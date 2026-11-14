//
// Regression test for srs_path_rename: POSIX rename replaces an existing
// destination, while the Windows CRT rename fails with EEXIST. HLS refreshes
// the same m3u8 path on every segment, so this must work on Windows too.
//

#include <srs_kernel_file_rename.hpp>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

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

int main()
{
    char *base = tmpnam(NULL);
    if (!base) {
        fprintf(stderr, "tmpnam failed\n");
        return 1;
    }

    std::string source = std::string(base) + ".from";
    std::string target = std::string(base) + ".to";

    // Replacing an existing destination is the HLS playlist refresh case.
    if (!write_file(source, "new-content") || !write_file(target, "old-content")) {
        fprintf(stderr, "write fixture failed: %s\n", strerror(errno));
        return 1;
    }

    if (srs_path_rename(source, target) != 0) {
        fprintf(stderr, "replace rename failed: %s\n", strerror(errno));
        return 1;
    }
    if (read_file(target) != "new-content") {
        fprintf(stderr, "destination was not replaced\n");
        return 1;
    }
    if (read_file(source) != "") {
        fprintf(stderr, "source still exists after rename\n");
        return 1;
    }

    // A missing source must fail rather than silently succeeding.
    if (srs_path_rename(source, target) != -1) {
        fprintf(stderr, "missing source did not fail\n");
        return 1;
    }

    remove(target.c_str());
    printf("path-rename-test: PASS\n");
    return 0;
}
