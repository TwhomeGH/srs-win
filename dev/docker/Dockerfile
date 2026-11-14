# Dev-only Linux lane: build the upstream SRS from this repository's trunk/.
# Keep this image separate from the Windows port; it exists for parity testing.
#
# Ubuntu 24.04 ships modern system libraries, so this lane deliberately does
# not build the vendored (and aging) 3rdparty copies:
#   OpenSSL 3.x, libsrtp 2.5+, SRT 1.5+, FFmpeg 6.1+
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        pkg-config \
        libssl-dev \
        libsrtp2-dev \
        libsrt-openssl-dev \
        libopus-dev \
        libavcodec-dev \
        libavformat-dev \
        libavutil-dev \
        libswscale-dev \
        libswresample-dev \
        unzip \
        automake \
        libtool \
        patch \
        git \
        tcl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /srs
COPY trunk /srs/trunk

# A Windows checkout keeps CRLF in some vendored helper scripts. Normalize the
# text scripts we might execute; dos2unix refuses files it guesses are binary,
# so use sed directly.
RUN find /srs/trunk -type f \( -name configure -o -name '*.sh' -o -name bootstrap -o -name 'config.guess' -o -name 'config.sub' \) \
        -exec sed -i 's/\r$//' {} + \
    && find /srs/trunk -type f \( -name configure -o -name '*.sh' -o -name bootstrap \) -exec chmod +x {} +

WORKDIR /srs/trunk

# Use the distribution's modern libraries instead of the vendored copies.
RUN ./configure --jobs="$(nproc)" \
        --use-sys-srtp=on \
        --use-sys-srt=on \
        --use-sys-ffmpeg=on \
    && make -j"$(nproc)"

EXPOSE 1935 1985 8088

CMD ["./objs/srs", "-c", "conf/dev.conf"]
