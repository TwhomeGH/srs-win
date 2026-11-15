//
// Copyright (c) 2013-2025 The SRS Authors
//
// SPDX-License-Identifier: MIT
//

#include <srs_app_console_auth.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include <srs_app_config.hpp>
#include <srs_kernel_error.hpp>
#include <srs_kernel_log.hpp>
#include <srs_kernel_utility.hpp>

using namespace std;

namespace {

const char *kBase64UrlAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

string base64url_encode(const string &input)
{
    string out;
    size_t i = 0;
    const unsigned char *data = (const unsigned char *)input.data();
    while (i + 2 < input.size()) {
        unsigned int n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out += kBase64UrlAlphabet[(n >> 18) & 0x3f];
        out += kBase64UrlAlphabet[(n >> 12) & 0x3f];
        out += kBase64UrlAlphabet[(n >> 6) & 0x3f];
        out += kBase64UrlAlphabet[n & 0x3f];
        i += 3;
    }
    if (i < input.size()) {
        unsigned int n = data[i] << 16;
        if (i + 1 < input.size()) {
            n |= data[i + 1] << 8;
        }
        out += kBase64UrlAlphabet[(n >> 18) & 0x3f];
        out += kBase64UrlAlphabet[(n >> 12) & 0x3f];
        if (i + 1 < input.size()) {
            out += kBase64UrlAlphabet[(n >> 6) & 0x3f];
        }
    }
    return out;
}

int base64url_value(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

string base64url_decode(const string &input)
{
    string out;
    unsigned int buffer = 0;
    int bits = 0;
    for (size_t i = 0; i < input.size(); i++) {
        int value = base64url_value(input[i]);
        if (value < 0) {
            return string();
        }
        buffer = (buffer << 6) | (unsigned int)value;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out += (char)((buffer >> bits) & 0xff);
        }
    }
    return out;
}

string hmac_hex(const string &key, const string &data)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_size = 0;
    if (!HMAC(EVP_sha256(), key.data(), (int)key.size(),
              (const unsigned char *)data.data(), data.size(), digest, &digest_size)) {
        return string();
    }

    static const char *hex = "0123456789abcdef";
    string out;
    for (unsigned int i = 0; i < digest_size; i++) {
        out += hex[(digest[i] >> 4) & 0xf];
        out += hex[digest[i] & 0xf];
    }
    return out;
}

string token_password()
{
    return srs_console_auth_password();
}

string token_create(const string &username, int64_t expires_at)
{
    char expires_buf[32];
    snprintf(expires_buf, sizeof(expires_buf), "%lld", (long long)expires_at);
    string payload = username + "|" + expires_buf;
    string encoded = base64url_encode(payload);
    return encoded + "." + hmac_hex(token_password(), payload);
}

bool token_parse(const string &token, string &username, int64_t &expires_at)
{
    size_t dot = token.rfind('.');
    if (dot == string::npos || dot == 0 || dot + 1 >= token.size()) {
        return false;
    }

    const string encoded = token.substr(0, dot);
    const string signature = token.substr(dot + 1);
    const string payload = base64url_decode(encoded);
    if (payload.empty()) {
        return false;
    }

    size_t sep = payload.rfind('|');
    if (sep == string::npos) {
        return false;
    }

    username = payload.substr(0, sep);
    const string expires = payload.substr(sep + 1);
    if (username.empty() || expires.empty()) {
        return false;
    }

    char *end = NULL;
    long long value = strtoll(expires.c_str(), &end, 10);
    if (!end || *end != '\0' || value <= 0) {
        return false;
    }
    expires_at = (int64_t)value;

    const string expected = hmac_hex(token_password(), payload);
    if (expected.size() != signature.size()) {
        return false;
    }
    // Constant-time comparison; the token is compared on every API request.
    return CRYPTO_memcmp(expected.data(), signature.data(), expected.size()) == 0;
}

} // namespace

static string g_console_username;
static string g_console_password;
static bool g_console_auth_bootstrapped = false;

static string auth_file_path()
{
    const char *custom = getenv("SRS_CONSOLE_AUTH_FILE");
    if (custom && *custom) {
        return string(custom);
    }

    string dir = _srs_config->get_work_dir();
    if (dir.empty()) {
        dir = "./";
    }
    if (dir[dir.size() - 1] == '/' || dir[dir.size() - 1] == '\\') {
        return dir + "objs/console-auth.txt";
    }
    return dir + "/objs/console-auth.txt";
}

static string random_password()
{
    unsigned char bytes[24];
    if (RAND_bytes(bytes, sizeof(bytes)) != 1) {
        return string();
    }

    static const char *hex = "0123456789abcdef";
    string out;
    for (size_t i = 0; i < sizeof(bytes); i++) {
        out += hex[(bytes[i] >> 4) & 0xf];
        out += hex[bytes[i] & 0xf];
    }
    return out;
}

static bool load_auth_file(const string &path, string &username, string &password)
{
    ifstream file(path.c_str());
    if (!file.is_open()) {
        return false;
    }

    string line;
    while (getline(file, line)) {
        if (line.compare(0, 9, "username=") == 0) {
            username = line.substr(9);
        } else if (line.compare(0, 9, "password=") == 0) {
            password = line.substr(9);
        }
    }
    return !username.empty() && !password.empty();
}

static srs_error_t write_auth_file(const string &path, const string &username, const string &password)
{
    string dir = srs_path_filepath_dir(path);
    if (!dir.empty()) {
        srs_error_t err = srs_os_mkdir_all(dir);
        if (err != srs_success) {
            return srs_error_wrap(err, "create console auth dir %s", dir.c_str());
        }
    }

    ofstream file(path.c_str(), ios::out | ios::trunc);
    if (!file.is_open()) {
        return srs_error_new(ERROR_SYSTEM_CONFIG_INVALID, "open console auth file %s", path.c_str());
    }
    file << "username=" << username << "\n";
    file << "password=" << password << "\n";
    if (!file.good()) {
        return srs_error_new(ERROR_SYSTEM_CONFIG_INVALID, "write console auth file %s", path.c_str());
    }
    file.close();
#ifndef _WIN32
    chmod(path.c_str(), 0600);
#endif
    return srs_success;
}

srs_error_t srs_console_auth_bootstrap()
{
    if (g_console_auth_bootstrapped) {
        return srs_success;
    }

    if (!_srs_config->get_http_api_auth_enabled()) {
        g_console_auth_bootstrapped = true;
        return srs_success;
    }

    // Explicit config or environment values are authoritative.
    g_console_username = _srs_config->get_http_api_auth_username();
    g_console_password = _srs_config->get_http_api_auth_password();
    if (!g_console_username.empty() && !g_console_password.empty()) {
        g_console_auth_bootstrapped = true;
        return srs_success;
    }

    // An environment password with no username still means "use the default
    // username", not "generate and overwrite the password".
    if (!g_console_password.empty()) {
        if (g_console_username.empty()) {
            g_console_username = "admin";
        }
        g_console_auth_bootstrapped = true;
        return srs_success;
    }

    const string path = auth_file_path();
    string file_user;
    string file_password;
    if (load_auth_file(path, file_user, file_password)) {
        g_console_username = file_user;
        g_console_password = file_password;
        srs_trace("http_api.auth loaded generated credentials from %s", path.c_str());
        g_console_auth_bootstrapped = true;
        return srs_success;
    }

    if (g_console_username.empty()) {
        g_console_username = "admin";
    }
    g_console_password = random_password();
    if (g_console_password.empty()) {
        return srs_error_new(ERROR_SYSTEM_CONFIG_INVALID, "generate console auth password");
    }

    srs_error_t err = write_auth_file(path, g_console_username, g_console_password);
    if (err != srs_success) {
        return srs_error_wrap(err, "persist generated console auth");
    }

    // Do not put the password in the log; the operator can read the file.
    srs_warn("http_api.auth generated initial credentials; read %s", path.c_str());
    g_console_auth_bootstrapped = true;
    return srs_success;
}

string srs_console_auth_username()
{
    return g_console_username.empty() ? _srs_config->get_http_api_auth_username() : g_console_username;
}

string srs_console_auth_password()
{
    return g_console_password.empty() ? _srs_config->get_http_api_auth_password() : g_console_password;
}

srs_error_t srs_console_auth_login(const string &username, const string &password,
                                   string &token, int64_t &expires_at)
{
    if (!_srs_config->get_http_api_auth_enabled()) {
        return srs_error_new(ERROR_SYSTEM_CONFIG_INVALID, "http_api.auth is disabled");
    }

    const string expected_user = srs_console_auth_username();
    const string expected_pass = srs_console_auth_password();
    if (username != expected_user || password != expected_pass) {
        return srs_error_new(ERROR_SYSTEM_CONFIG_INVALID, "invalid console credentials");
    }

    expires_at = (int64_t)time(NULL) + SRS_CONSOLE_TOKEN_TTL_SECONDS;
    token = token_create(username, expires_at);
    if (token.empty()) {
        return srs_error_new(ERROR_SYSTEM_CONFIG_INVALID, "create console token");
    }

    return srs_success;
}

bool srs_console_auth_verify(const string &token, string &username)
{
    if (!_srs_config->get_http_api_auth_enabled()) {
        return false;
    }

    int64_t expires_at = 0;
    if (!token_parse(token, username, expires_at)) {
        return false;
    }

    if (username != srs_console_auth_username()) {
        return false;
    }

    return expires_at > (int64_t)time(NULL);
}

srs_error_t srs_console_auth_refresh(const string &token, string &new_token, int64_t &expires_at)
{
    string username;
    if (!srs_console_auth_verify(token, username)) {
        return srs_error_new(ERROR_SYSTEM_CONFIG_INVALID, "invalid or expired console token");
    }

    expires_at = (int64_t)time(NULL) + SRS_CONSOLE_TOKEN_TTL_SECONDS;
    new_token = token_create(username, expires_at);
    if (new_token.empty()) {
        return srs_error_new(ERROR_SYSTEM_CONFIG_INVALID, "refresh console token");
    }

    return srs_success;
}
