# Prerequisites for your host environment

## Ubuntu

First installed needed packages and libraries

```sh
apt-get update \
  && apt-get install -y apt-transport-https apt-utils build-essential \
  ca-certificates curl g++-multilib git gnupg \
  libgcc-9-dev lib32gcc-9-dev lsb-release \
  ninja-build ocaml ocamlbuild python2.7 \
  software-properties-common tree tzdata \
  unzip valgrind vim wget zip --no-install-recommen
```

Then install CMake and wasi-sdk-16.0

```sh
wget --progress=dot:giga -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg > /dev/null \
  && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ bionic main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null \
  && apt-get update \
  && rm /usr/share/keyrings/kitware-archive-keyring.gpg \
  && apt-get install -y kitware-archive-keyring --no-install-recommends \
  && apt-get install -y cmake --no-install-recommends 

wget -c --progress=dot:giga https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-16/wasi-sdk-16.0-linux.tar.gz -P /opt \
  && tar xf /opt/wasi-sdk-16.0-linux.tar.gz -C /opt \
  && ln -fs /opt/wasi-sdk-16.0 /opt/wasi-sdk \
  && rm /opt/wasi-sdk-16.0-linux.tar.gz
```

This should be sufficient to build WAMR and run our hello world program.
<!-- 
TODO:

## MacOS

## Windows
 -->
