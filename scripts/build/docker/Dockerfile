# aarch64 QNX & HGY based toolchain

FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

USER root

COPY sources_aliyun.list  /etc/apt
COPY requirements.txt /tmp

# Force bash for everything
ENV SHELL /bin/bash

# Install basic packages
RUN ln -fs /bin/bash /bin/sh \
    && mv /etc/apt/sources.list /etc/apt/sources_bak.list \
    && mv /etc/apt/sources_aliyun.list /etc/apt/sources.list \
    && apt-get update && apt install -y rsync wget curl vim \
    cpio unzip zip lsof bc pixz pigz tree gawk  \
    build-essential pkg-config libtool autoconf automake debhelper \
    bison flex gdb gdb-multiarch strace ltrace tzdata locales \
    chrpath texinfo binutils ftp lftp telnet \
    apt-transport-https ca-certificates gnupg software-properties-common \
    openssh-client git git-lfs diffstat \
    && rm -rf /var/cache/apk/* && rm -rf /var/lib/apt/lists/*

# Install QNN Linux dependency
RUN apt-get update && apt install -y libncurses5 libgl1 libgtk-3-dev libasound2-dev \
    libnss3 nodejs npm libgbm-dev desktop-file-utils \
    && add-apt-repository 'deb http://cz.archive.ubuntu.com/ubuntu jammy main universe' \
    && apt-get update && apt install -y \
    flatbuffers-compiler libflatbuffers-dev rename \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
    && ln -sf /usr/aarch64-linux-gnu/lib/ld-2.31.so /usr/lib/ld-linux-aarch64.so.1 \
    && export npm_config_cache=/tmp/npm \
    && npm cache clean -f && npm install -g n && n 14.17.4 \
    && npm i -g npm@6.14.14 \
    && rm -rf /var/cache/apt/archives/* \
    && rm -rf /var/lib/apt/lists/*

# Install clang-9
RUN wget https://github.com/llvm/llvm-project/releases/download/llvmorg-9.0.1/clang+llvm-9.0.1-aarch64-linux-gnu.tar.xz     -P /tmp --no-check-certificate && cd /tmp \
    && tar -xf clang+llvm-9.0.1-aarch64-linux-gnu.tar.xz -C /usr/lib \
    && mv /usr/lib/clang+llvm-9.0.1-aarch64-linux-gnu /usr/lib/clang-9 \
    && ln -sf /usr/lib/llvm-9/bin/clang /usr/bin/clang \
    && ln -sf /usr/lib/llvm-9/bin/clang++ /usr/bin/clang++ \
    && rm -rf /tmp/clang+llvm-9.0.1-aarch64-linux-gnu.tar.xz

# Install cmake
RUN wget https://github.com/Kitware/CMake/releases/download/v3.20.0/cmake-3.20.0.tar.gz -P /tmp --no-check-certificate \
    && cd /tmp && tar -zxf cmake-3.20.0.tar.gz && cd /tmp/cmake-3.20.0 \
    && ./bootstrap && make && make install \
    && cd /tmp && rm -rf /tmp/cmake-3.20.0*

# Install lconv
RUN wget https://github.com/linux-test-project/lcov/releases/download/v1.16/lcov-1.16.tar.gz -P /tmp --no-check-certificate \
    && cd /tmp && tar -zxf lcov-1.16.tar.gz && cd lcov-1.16 \
    && make -j8 && make install \
    && cd /tmp && rm -rf /tmp/lcov-1.16*

# Install python packages
RUN apt-get update && add-apt-repository ppa:deadsnakes/ppa \
    &&apt-get install -y python3 python3-dev python3-pip \
    python3-distutils python3-setuptools python3-venv \
    && rm -rf /var/cache/apt/archives/* \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf ~/.cache/pip \
    && rm -rf /tmp/*

# Final config
RUN dpkg-reconfigure locales && \
    locale-gen en_US.UTF-8 && \
    update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8

ENV LC_ALL   en_US.UTF-8
ENV LANG     en_US.UTF-8
ENV LANGUAGE en_US.UTF-8
ENV TZ America/Los_Angeles

# Setup entry point
ENTRYPOINT [ "/bin/toolchain-entry" ]
CMD /bin/bash
