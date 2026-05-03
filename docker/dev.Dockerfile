FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

ARG LOCAL_UID=1000
ARG LOCAL_GID=1000

RUN apt-get update && apt-get install -y --no-install-recommends \
    bash \
    build-essential \
    ca-certificates \
    clang-format \
    cmake \
    git \
    libbz2-dev \
    libgflags-dev \
    libgrpc++-dev \
    liblz4-dev \
    libprotobuf-dev \
    libsnappy-dev \
    libyaml-cpp-dev \
    libzstd-dev \
    make \
    ninja-build \
    python3 \
    curl \
    protobuf-compiler-grpc \
    pkg-config \
    protobuf-compiler \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 --branch v11.1.1 https://github.com/facebook/rocksdb.git /tmp/rocksdb \
    && cd /tmp/rocksdb \
    && DEBUG_LEVEL=0 PORTABLE=1 make -j2 shared_lib \
    && make install-shared \
    && rm -rf /tmp/rocksdb

RUN groupadd -f -o -g "${LOCAL_GID}" bytetaper \
    && useradd -m -o -u "${LOCAL_UID}" -g "${LOCAL_GID}" -s /bin/bash bytetaper

WORKDIR /workspace
USER bytetaper
