# How to use WAMR with Zephyr

## Build with Docker(recommend approach)

To have a quicker start, a Docker container of the Zephyr setup can be generated. The current docker image would be considerably large(~15GB), it would take some time to build it and enough disk space to store it.

### Build Docker images

```shell
docker build -t wamr-zephyr .
```

> PS: currently, the esp32 custom linker script only works with a lower version of Zephyr, if you want to use an esp32 board, you can build the Dockerfile with a lower version of Zephyr, Zephyr SDE, ESP-IDF. The old version of Docker image can also build other targets, but probably it's a better choice to use the new Dockerfile for other boards

```shell
# If you want to build on esp32 platform
docker build -f Dockerfile.old -t wamr-zephyr .
```

### Run Docker images

Adopt the device or remove if not needed.

```shell
docker run -ti --device=/dev/ttyUSB0 wamr-zephyr
```

And then inside the docker container:

```shell
# copy the corresponding board conf file to current directory
cp boards/qemu_x86_nommu.conf prj.conf
# then build
./build_and_run.sh x86
```

> PS: for boards esp32, need to configure some environment first

```shell
# configure zephyr with espressif
export ZEPHYR_TOOLCHAIN_VARIANT="espressif"
export ESPRESSIF_TOOLCHAIN_PATH="/root/.espressif/tools/xtensa-esp32-elf/esp-2019r2-8.2.0/xtensa-esp32-elf/"
export ESP_IDF_PATH="/root/esp/esp-idf"
# copy the corresponding board conf file to current directory
cp boards/esp32.conf prj.conf
# then build
./build_and_run.sh esp32
```

## Build on local environment

### Dependencies installation

Following the Zephyr and Espressif official document:

1. Zephyr installation:

   <https://docs.zephyrproject.org/latest/develop/getting_started/index.html>

2. ESP32 installation:

   <https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html>

And setup the Zephyr for esp32:

<https://wiki.amarulasolutions.com/zephyr/esp32/esp32-setup.html>

Then Installing QEMU, for example, on Linux:

```shell
sudo apt-get install qemu
```

### Run the build script

Make sure you have the environment variable ready, you can use the command `env` to check:

```shell
env
```

```shell
# export ZEPHYR_BASE if it's not present
export ZEPHYR_BASE=~/zephyrproject/zephyr
# and if you install zephyr in virtual environment rather than global
source ~/zephyrproject/.venv/bin/activate
```

For boards esp32, need to configure some extra environment first, check the following env variable whether in the env list, if not, add them like:

> Noted: The esp32 custom linker script doesn't work with the recent version of Zephyr, if you want to use it in the local environment, please install Zephyr 2.3.0 with the corresponding SDK, and ESP-IDF 4.0

```shell
export ZEPHYR_TOOLCHAIN_VARIANT="espressif"
export ESPRESSIF_TOOLCHAIN_PATH="~/.espressif/tools/xtensa-esp32-elf/esp-{the version you installed}/xtensa-esp32-elf/"
export ESP_IDF_PATH="~/esp/esp-idf"
```

Then you can run the build script:

```shell
# copy the corresponding board conf file to current directory
cp boards/qemu_x86_nommu.conf prj.conf
# then build
./build_and_run.sh x86
```
