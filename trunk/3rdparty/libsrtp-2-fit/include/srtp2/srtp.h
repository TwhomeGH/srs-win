// Forwarding header: maps #include <srtp2/srtp.h> to the bundled srtp.h
// When vcpkg libsrtp is installed, the real header takes priority.
#pragma once
#include "../srtp.h"
