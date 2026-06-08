// OpenSSL API compatibility shim for SRS on Windows.
// Maps OpenSSL 1.1 APIs to OpenSSL 3.x/4.x equivalents.
#ifndef SRS_COMPAT_OPENSSL_COMPAT_H
#define SRS_COMPAT_OPENSSL_COMPAT_H

// TLSv1_2_method was removed in OpenSSL 3.0+. Use TLS_client_method with
// min protocol version set to TLS 1.2 instead.
#ifndef TLSv1_2_method
#define TLSv1_2_method() TLS_client_method()
#endif

// DTLSv1_method was removed in OpenSSL 3.0+. Map to DTLS_client/server_method.
#ifndef DTLSv1_client_method
#define DTLSv1_client_method() DTLS_client_method()
#endif
#ifndef DTLSv1_server_method
#define DTLSv1_server_method() DTLS_server_method()
#endif

// SRTP_MASTER_KEY_LEN is now provided by libsrtp2's srtp.h (vcpkg or bundled).
// Do not define here to avoid redefinition warnings.

#endif
