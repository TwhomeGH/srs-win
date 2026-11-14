//
// Regression test for Windows file sharing during HLS playlist refresh.
// A reader must open with FILE_SHARE_DELETE so srs_path_rename can replace
// the m3u8 while an HTTP client still holds it open.
//

#include <srs_kernel_file_open.hpp>
#include <srs_kernel_file_rename.hpp>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

static bool write_file(const std::string &path, const char *content)
{
    FILE *fp = fopen(path.c_str(), "wb");
    if (!fp) {
        return false;
    }
    bool ok = fwrite(content, 1, strlen(content), fp) == strlen(content);
    fclose(fp);
    return ok;
}

static std::string read_file(const std::string &path)
{
    FILE *fp = fopen(path.c_str(), "rb");
    if (!fp) {
        return "";
    }
    char buf[64];
    size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);
    buf[n] = '\0';
    return std::string(buf, n);
}

static void close_fd(int fd)
{
#ifdef _WIN32
    _close(fd);
#else
    close(fd);
#endif
}

int main()
{
    char *base = tmpnam(NULL);
    if (!base) {
        fprintf(stderr, "tmpnam failed\n");
        return 1;
    }

    std::string source = std::string(base) + ".new";
    std::string target = std::string(base) + ".m3u8";

    if (!write_file(source, "new-playlist") || !write_file(target, "old-playlist")) {
        fprintf(stderr, "write fixture failed: %s\n", strerror(errno));
        return 1;
    }

    // Hold the destination open exactly like SrsHttpFileServer does.
#ifdef _WIN32
    int fd = srs_open_readonly(target.c_str(), O_RDONLY | O_BINARY);
#else
    int fd = srs_open_readonly(target.c_str(), O_RDONLY);
#endif
    if (fd < 0) {
        fprintf(stderr, "open destination failed: %s\n", strerror(errno));
        return 1;
    }

    if (srs_path_rename(source, target) != 0) {
        fprintf(stderr, "rename while reader open failed: %s\n", strerror(errno));
        close_fd(fd);
        return 1;
    }
    close_fd(fd);

    if (read_file(target) != "new-playlist") {
        fprintf(stderr, "destination was not replaced\n");
        return 1;
    }
    if (read_file(source) != "") {
        fprintf(stderr, "source still exists after rename\n");
        return 1;
    }

    remove(target.c_str());
    printf("file-share-test: PASS\n");
    return 0;
}
