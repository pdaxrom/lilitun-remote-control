# syntax=docker/dockerfile:experimental
FROM ubuntu:18.04 AS osxcross
LABEL stage=osxcross

ENV DEBIAN_FRONTEND noninteractive

COPY gcc-disable-multilib.patch /root/

COPY MacOSX10.9.sdk.tar.gz /root/

COPY bomutils /root/bomutils

WORKDIR root

RUN apt update && apt install -y gcc g++ zlib1g-dev libmpc-dev libmpfr-dev libgmp-dev clang cmake git libssl-dev lzma-dev libxml2-dev patch python wget && \
    git clone https://github.com/tpoechtrager/osxcross.git && \
    mv -f /root/MacOSX10.9.sdk.tar.gz osxcross/tarballs/ && \
    cd osxcross && \
    patch -p1 < /root/gcc-disable-multilib.patch

WORKDIR /root/osxcross

RUN yes | ./build.sh

RUN yes | ./build_gcc.sh

RUN strip target/bin/* || true && strip osxcross/target/libexec/gcc/x86_64-apple-darwin13/10.2.0/* || true

RUN make -C /root/bomutils PREFIX=/root/osxcross/target && make -C /root/bomutils PREFIX=/root/osxcross/target install

FROM ubuntu:18.04
LABEL stage=builder

ENV DEBIAN_FRONTEND noninteractive

RUN apt update && apt install -y git ccache pkg-config m4 build-essential cmake zlib1g-dev libpng-dev libjpeg-dev \
    liblzo2-dev libssl-dev automake autoconf libtool chrpath libx11-dev libxft-dev libgl1-mesa-dev \
    libglu1-mesa-dev libxinerama-dev libxcursor-dev libxcb1-dev libxcb-shm0-dev libxcb-xtest0-dev \
    libxcb-keysyms1-dev imagemagick debhelper mingw-w64 wget libfuse2 clang lzma-dev libxml2-dev \
    zlib1g-dev libmpc-dev libmpfr-dev libgmp-dev nasm zip cpio && apt clean

COPY --from=osxcross /root/osxcross/target /root/osxcross/target

ENV PATH="/root/osxcross/target/bin:${PATH}"

ENV INSIDE_DOCKER="yes"

ENV INSIDE_DOCKER_SRC_DIR="/root/src"

WORKDIR /root/src

CMD ./build.sh
