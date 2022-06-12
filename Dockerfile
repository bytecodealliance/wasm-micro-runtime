FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build.essential \
    cmake \
    git \
    wget

RUN wget -q https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-14/wasi-sdk-14.0-linux.tar.gz && \
    tar xf wasi-sdk-*-linux.tar.gz -C /opt && rm -f wasi-sdk-*-linux.tar.gz && \
    mv /opt/wasi-sdk-$(WASI_SDK_VERSION) /opt/wasi-sdk
