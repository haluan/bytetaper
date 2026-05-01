FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    bash \
    build-essential \
    ca-certificates \
    clang-format \
    cmake \
    make \
    ninja-build \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
