//
// Copyright (c) 2013-2025 The SRS Authors
//
// SPDX-License-Identifier: MIT
//

#ifndef SRS_APP_CONSOLE_AUTH_HPP
#define SRS_APP_CONSOLE_AUTH_HPP

#include <srs_core.hpp>

#include <ctime>
#include <string>

// Console session tokens are stateless HMAC tokens derived from the configured
// http_api.auth password. They survive an SRS restart, expire after 7 days, and
// can be refreshed without asking the user to re-enter the password.
#define SRS_CONSOLE_TOKEN_TTL_SECONDS (7 * 24 * 60 * 60)

// Create a token for a valid username/password pair.
// @param token output token, only set on success.
// @param expires_at output Unix timestamp in seconds.
extern srs_error_t srs_console_auth_login(const std::string &username, const std::string &password,
                                          std::string &token, int64_t &expires_at);

// Bootstrap effective credentials. If auth is enabled without username/password,
// generate a random password once and persist it to work_dir/objs/console-auth.txt.
// Explicit config or SRS_HTTP_API_AUTH_USERNAME/PASSWORD always win.
extern srs_error_t srs_console_auth_bootstrap();

// Effective credentials used by the HTTP auth mux and token signer.
extern std::string srs_console_auth_username();
extern std::string srs_console_auth_password();

// Verify a bearer token and return its username.
// @note The token is stateless; changing the configured password invalidates it.
extern bool srs_console_auth_verify(const std::string &token, std::string &username);

// Refresh a still-valid token, issuing a new 7-day expiry.
extern srs_error_t srs_console_auth_refresh(const std::string &token, std::string &new_token, int64_t &expires_at);

#endif
