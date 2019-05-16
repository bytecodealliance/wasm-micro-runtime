#1.Overview
Littlevgl is an Open-source Embedded GUI Library. We definde an UI APP, and it can be running in 3 scenarios.
1. Native Linux. The App code built into Linux executables.
2. WASM VM for Different platform. WASM VM and native extension being built into Linux and Zephyr platforms. With WASM VM inside, many WASM APP can run on top of it.
3. WASM APP. This kind of binary can be run on WASM VM.
##Folder structure
├── build.sh   **build script, will create build and out folder.**
├── LICENCE.txt
├── UI.JPG    **UI appearence.**
├── user_guide.md **User Guide.**
### 1. vgl-native-ui-app
Littlevgl graphics app has being built into Linux application named "vgl_native_ui_app", which can directly run on Linux.
├── vgl-native-ui-app
│   ├── CMakeLists.txt
│   ├── lv_drivers             **Display and input device driver sources.**
│   └── main.c                  **UI app logic.**
### 2. vgl-wasm-runtime
Wasm micro-runtime and littlevgl native interface built into Linux application named "littlevgl", where wasm apps can run on it.
├── vgl-wasm-runtime
│   ├── CMakeLists.txt
│   ├── src **Cantains WASM VM initialization adn native extension sources.**
│   │   ├── display_indev.h **App called methods which are implemented in WASM native.**
│   │   ├── ext-lib-export.c **WASM exported function**
│   │   ├── platform   **Native method Implementation on different platform.**
│   │   │   ├── linux **Linux implementation**
│   │   │   │   ├── display_indev.c
│   │   │   │   ├── iwasm_main.c
│   │   │   │   ├── main.c
│   │   │   │   └── mouse.c
│   │   │   └── zephyr **Zephyr implementation**
│   │   │       ├── display.h
│   │   │       ├── display_ili9340_adafruit_1480.c
│   │   │       ├── display_ili9340.c
│   │   │       ├── display_ili9340.h
│   │   │       ├── display_indev.c
│   │   │       ├── LICENSE
│   │   │       ├── XPT2046.c
│   │   │       └── XPT2046.h
│   │   └── test_wasm.h
│   └── zephyr-build **Zephyr project files**
│       ├── CMakeLists.txt
│       └── prj.conf

###3. wasm-apps
A wasm app with littlevgl graphics.
└── wasm-apps
    ├── build_wasm_app.sh **Git clone lvgl and build app.**
    ├── Makefile_wasm_app **Makefile**
    └── src  **source code of UI app.**

#2.Install required SDK and libraries.
##1. 32 bit SDL(simple directmedia layer) 
###a. Use apt-get
sudo apt-get install libsdl2-dev:i386
###b. Install from source
www.libsdl.org
`./configure C_FLAGS=-m32 CXX_FLAGS=-m32 LD_FLAGS=-m32`
 ` ./make`
`./sudo make install`
##2. Install EMSDK
    https://emscripten.org/docs/tools_reference/emsdk.html
##3. Cmake
     CMAKE version above 3.13.1.
#3.Build & Run
##1. Build and run on Linux
###a. Build
`./build.sh`
    All binaries are in "out", which contains "host_tool", "vgl_native_ui_app", "TestApplet1.wasm" and "vgl_wasm_runtime".
###b.Run native Linux application
`./vgl_native_ui_app`
###c.Run WASM VM Linux applicaton & install WASM APP
####1. Start vgl_wasm_runtime in server mode
`./vgl_wasm_runtime -s`
####2. Install wasm APP
`./host_tool -i TestApplet1 -f TestApplet1.wasm`
##2. Build and run on zephyr
WASM VM and native extension method can be built into zephyr, Then we can install wasm app into STM32.
###1.Build wasm into Zephyr system
####a. clone zephyr source code
`git clone https://github.com/zephyrproject-rtos/zephyr.git`
####b. copy samples
    `cd zephyr/samples/`
    `cp -a <iwasm_root_dir>projects/littlevgl/vgl-wasm-runtime vgl-wasm-runtime`
    `cd vgl-wasm-runtime/zephyr_build`
####c. create a link to wamr core
   ` ln -s <iwasm_root_dir>/core core`
####d. build source code
We use nucleo_f767zi, which is almost the same as nucleo_f746zg, except SRAM size in DTS.
Duplicate zephyr board support of nucleo_f746zg, then change SRAM size to 512KB.
    `mkdir build && cd build`
    `source ../../../../zephyr-env.sh`
    `cmake -GNinja -DBOARD=nucleo_f767ZI ..`
   ` ninja flash`

###2. Test on STM32 NUCLEO_F767ZI with ILI9341 Display with XPT2046 touch.
####a. Hardware Connetions
+-------------------+-+------------------+
|NUCLEO-F767ZI || ILI9341  Display |
+-------------------+-+------------------+
| CN7.10               |         CLK             |
+-------------------+-+------------------+
| CN7.12               |         MISO           |
+-------------------+-+------------------+
| CN7.14               |         MOSI           |
+-------------------+-+------------------+
| CN11.1               | CS1 for ILI9341   |
+-------------------+-+------------------+
| CN11.2               |         D/C              |
+-------------------+-+------------------+
| CN11.3               |         RESET         |
+-------------------+-+------------------+
| CN9.25               |    PEN interrupt   |
+-------------------+-+------------------+
| CN9.27               | CS2 for XPT2046|
+-------------------+-+------------------+
| CN10.14             | PC UART RX        |
+-------------------+-+------------------+
| CN11.16             | PC UART RX        |
+-------------------+-+------------------+

####b. Install wasm app to zephyr using host_tool
First connect PC and STM32 with UART. Then install use host_tool.
`./host_tool -D /dev/ttyUSB0 -i TestApplet1 -f TestApplet1.wasm`

