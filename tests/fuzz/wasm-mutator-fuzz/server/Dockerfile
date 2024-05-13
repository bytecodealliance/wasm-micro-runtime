FROM ubuntu:20.04
WORKDIR /wamr-internal-test/tests/fuzz/wasm_mutator_fuzz/server
ADD tests/fuzz/wasm_mutator_fuzz/server/requirements.txt /requirements.txt

ARG proxy=""

RUN if [ "$proxy" != "" ]; \
    then export http_proxy="$proxy" && export https_proxy="$proxy"; \
    else echo Do not set proxy; \
    fi

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asian/Shanghai

RUN apt -o Acquire::http::proxy="$proxy" update
RUN apt -o Acquire::http::proxy="$proxy" install \
 curl clang rustc cargo python3 python3-pip git gcc build-essential cmake g++-multilib libunwind-dev wget -y
RUN pip install -U -r /requirements.txt --proxy=$proxy
ADD ./tests/fuzz /wamr-internal-test/tests/fuzz
RUN git config --global http.proxy $proxy
RUN git config --global https.proxy $proxy

WORKDIR /wamr-internal-test/tests/fuzz/wasm_mutator_fuzz
#RUN git clone https://github.com/bytecodealliance/wasm-tools
#WORKDIR /wamr-internal-test/tests/fuzz/wasm_mutator_fuzz/wasm-tools
#RUN cargo install wasm-tools

RUN wget -e "https_proxy=$proxy" https://github.com/bytecodealliance/wasm-tools/releases/download/v1.201.0/wasm-tools-1.201.0-x86_64-linux.tar.gz
RUN tar -xzf wasm-tools-1.201.0-x86_64-linux.tar.gz
RUN mv wasm-tools-1.201.0-x86_64-linux wasm-tools
ENV PATH="/wamr-internal-test/tests/fuzz/wasm_mutator_fuzz/wasm-tools:$PATH"

WORKDIR /wamr-internal-test/tests/fuzz/wasm_mutator_fuzz/server/app
CMD nohup sh -c 'python3 main.py'
