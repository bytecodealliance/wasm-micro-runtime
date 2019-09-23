Introduction
==============
This sample demonstrates that a graphic user interface application in WebAssembly programming with WAMR graphic library(WGL) extension. WGL defined a WASM application API set for programming the UI applications. 

WGL implemention is based on [LittlevGL](https://github.com/littlevgl/), an open-source embedded 2d graphic library. Comparing the building the LittlevGL into WASM bytecode in the [littlevgl](../littlevgl) sample, WGL compiled LittlevGL source code into the WAMR runtime and defined a wrapper API for exporting to Webassembly application. These extension API's are listed in: `<wamr_root>/core/iwasm/lib/app-libs/extension/gui/wgl.h`. Currently only a small set of API's  are provided and that would be extended in future.


The runtime component supports building target for Linux and Zephyr/STM Nucleo board. The beauty of this sample is the WebAssembly application can have identical display and behavior when running from both runtime environments. That implies we can do majority of application validation from desktop environment as long as two runtime distributions support the same set of application interface.


Below pictures show the WASM application is running on an STM board with an LCD touch panel. When users click the blue button, the WASM application increases the counter, and the latest counter value is displayed on the top banner of the touch panel. The number on top will plus one each second, and the number on the bottom will plus one when clicked.


![WAMR UI SAMPLE](../../doc/pics/vgl_demo2.png "WAMR UI DEMO")

Configure 32 bit or 64 bit build
==============
On 64 bit operating system, there is an option to build 32 bit or 64 bit binaries. In file `./lvgl-native-ui-app/CMakeLists.txt` and/or `./wasm-runtime-wgl/linux-build/CMakeLists.txt` , modify the line:
`set (BUILD_AS_64BIT_SUPPORT "YES")`
 where `YES` means 64 bit build while `NO` means 32 bit build.

Install required SDK and libraries
==============
- 32 bit SDL(simple directmedia layer) (Note: only necessary when `BUILD_AS_64BIT_SUPPORT` is set to `NO`)
Use apt-get:
    `sudo apt-get install libsdl2-dev:i386`
Or download source from www.libsdl.org:
```
./configure C_FLAGS=-m32 CXX_FLAGS=-m32 LD_FLAGS=-m32
make
sudo make install
```
- 64 bit SDL(simple directmedia layer) (Note: only necessary when `BUILD_AS_64BIT_SUPPORT` is set to `YES`)
Use apt-get:
    `sudo apt-get install libsdl2-dev`
Or download source from www.libsdl.org:
```
./configure
make
sudo make install
```

- Install EMSDK
```
    https://emscripten.org/docs/tools_reference/emsdk.html
```

Build and Run
==============

Linux
--------------------------------
- Build</br>
`./build.sh`</br>
    All binaries are in "out", which contains "host_tool", "lvgl_native_ui_app", "ui_app.wasm", "ui_app_lvgl_compatible.wasm" and "wasm_runtime_wgl".
- Run native Linux application</br>
`./lvgl_native_ui_app`</br>

- Run WASM VM Linux applicaton & install WASM APP</br>
 First start wasm_runtime_wgl in server mode.</br>
`./wasm_runtime_wgl -s`</br>
 Then install wasm APP use host tool.</br>
`./host_tool -i ui_app -f ui_app.wasm`</br>
`./host_tool -i ui_app -f ui_app_lvgl_compatible.wasm`</br>

Zephyr
--------------------------------
WASM VM and native extension method can be built into Zephyr, Then we can install wasm app into STM32.</br>
- Build WASM VM into Zephyr system</br>
 a. clone zephyr source code</br>
Refer to Zephyr getting started.</br>
https://docs.zephyrproject.org/latest/getting_started/index.html</br>
`west init zephyrproject`</br>
`cd zephyrproject`</br>
`west update`</br>
 b. copy samples</br>
    `cd zephyr/samples/`</br>
    `cp -a <wamr_root>samples/gui/wasm-runtime-wgl wasm-runtime-wgl`</br>
    `cd wasm-runtime-wgl/zephyr_build`</br>
 c. create a link to wamr core</br>
   ` ln -s <wamr_root>/core core`</br>
 d. build source code</br>
    `mkdir build && cd build`</br>
    `source ../../../../zephyr-env.sh`</br>
    `cmake -GNinja -DBOARD=nucleo_f746zg ..`</br>
   ` ninja flash`</br>

- Test on STM32 NUCLEO_F767ZI with ILI9341 Display with XPT2046 touch</br>
Hardware Connections

```
+-------------------+-+------------------+
|NUCLEO-F767ZI       | ILI9341  Display  |
+-------------------+-+------------------+
| CN7.10             |         CLK       |
+-------------------+-+------------------+
| CN7.12             |         MISO      |
+-------------------+-+------------------+
| CN7.14             |         MOSI      |
+-------------------+-+------------------+
| CN11.1             | CS1 for ILI9341   |
+-------------------+-+------------------+
| CN11.2             |         D/C       |
+-------------------+-+------------------+
| CN11.3             |         RESET     |
+-------------------+-+------------------+
| CN9.25             |    PEN interrupt  |
+-------------------+-+------------------+
| CN9.27             |  CS2 for XPT2046  |
+-------------------+-+------------------+
| CN10.14            |    PC UART RX     |
+-------------------+-+------------------+
| CN11.16            |    PC UART RX     |
+-------------------+-+------------------+
```


- Install WASM application to Zephyr using host_tool</br>
First, connect PC and STM32 with UART. Then install to use host_tool.</br>
`./host_tool -D /dev/ttyUSBXXX -i ui_app -f ui_app.wasm`

