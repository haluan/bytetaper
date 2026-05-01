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
    libgrpc++-dev \
    libprotobuf-dev \
    make \
    ninja-build \
    protobuf-compiler-grpc \
    pkg-config \
    protobuf-compiler \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd -f -o -g "${LOCAL_GID}" bytetaper \
    && useradd -m -o -u "${LOCAL_UID}" -g "${LOCAL_GID}" -s /bin/bash bytetaper

WORKDIR /workspace
USER bytetaper
