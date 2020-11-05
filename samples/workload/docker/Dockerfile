FROM ubuntu:18.04 as builder

#
# install clang and llvm
COPY llvm.sh /tmp
RUN apt update \
    && apt install -y lsb-release wget software-properties-common build-essential \
    && cd /tmp \
    && chmod a+x llvm.sh \
    && ./llvm.sh 11

ARG WASI_SDK_VER=11.0
ARG WABT_VER=1.0.19
ARG CMAKE_VER=3.16.2
ARG BINARYEN_VER=version_97

#
# install wasi-sdk
ARG WASI_SDK_FILE="wasi-sdk-${WASI_SDK_VER}-linux.tar.gz"
COPY ${WASI_SDK_FILE} /opt
RUN cd /opt \
    && tar zxf ${WASI_SDK_FILE} \
    && rm ${WASI_SDK_FILE} \
    && ln -sf /opt/wasi-sdk-${WASI_SDK_VER} /opt/wasi-sdk \
    && ln -sf /opt/wasi-sdk/lib/clang/10.0.0/lib/wasi/ /usr/lib/llvm-11/lib/clang/11.0.0/lib/

#
# install wabt
ARG WABT_FILE="wabt-${WABT_VER}-ubuntu.tar.gz"
COPY ${WABT_FILE} /opt
RUN cd /opt \
    && tar zxf ${WABT_FILE} \
    && rm ${WABT_FILE} \
    && ln -sf /opt/wabt-${WABT_VER} /opt/wabt

#
# install cmake
ARG CMAKE_FILE="cmake-${CMAKE_VER}-Linux-x86_64.sh"
COPY ${CMAKE_FILE} /tmp
RUN cd /tmp \
    && chmod a+x ${CMAKE_FILE} \
    && mkdir /opt/cmake \
    && ./${CMAKE_FILE} --prefix=/opt/cmake --skip-license  \
    && ln -sf /opt/cmake/bin/cmake /usr/local/bin/cmake

#
# install tools
RUN apt install -y git tree

#
# install emsdk
RUN cd /opt \
    && git clone https://github.com/emscripten-core/emsdk.git \
    && cd emsdk \
    && git pull \
    && ./emsdk install latest \
    && ./emsdk activate latest \
    && echo "source /opt/emsdk/emsdk_env.sh" >> /root/.bashrc

#
# install binaryen
ARG BINARYEN_FILE="binaryen-${BINARYEN_VER}-x86_64-linux.tar.gz"
COPY ${BINARYEN_FILE} /opt
RUN cd /opt \
    && tar zxf ${BINARYEN_FILE} \
    && rm ${BINARYEN_FILE}  \
    && ln -sf /opt/binaryen-${BINARYEN_VER} /opt/binaryen

#
# Clean up
RUN apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf /tmp/*

VOLUME /data
WORKDIR /data
