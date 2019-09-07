Introduction
==============
This project builds out both host tools running on the host side, and an application running on the device side. The device application consists of iwasm, application library, application manager, timers and sensors support. The device runs on Linux OS and interacts with host tools.

It demonstrates an end to end scenario, the wasm applications life cycle management and communication programming models.

Directory structure
------------------------------
```
simple/
├── build.sh
├── CMakeLists.txt
├── README.md
├── src
│   ├── ext_lib_export.c
│   ├── iwasm_main.c
│   └── main.c
└── wasm-apps
    ├── connection.c
    ├── event_publisher.c
    ├── event_subscriber.c
    ├── gui.c
    ├── request_handler.c
    ├── request_sender.c
    ├── sensor.c
    └── timer.c
```

- build.sh<br/>
  The script to build all binaries.
- CMakeLists.txt<br/>
  CMake file used to build the simple application.
- README.md<br/>
  The file you are reading currently.
- src/ext_lib_export.c<br/>
  This file is used to export native APIs. See the `The mechanism of exporting Native API to WASM application` section in WAMR README.md for detail.
- src/iwam_main.c<br/>
  This file is the implementation by platform integrator. It implements the interfaces that enable the application manager communicating with the host side. See `{WAMR_ROOT}/core/app-mgr/app-mgr-shared/app_manager_export.h` for the definition of the host interface.
```
/* Interfaces of host communication */
typedef struct host_interface {
    host_init_func init;
    host_send_fun send;
    host_destroy_fun destroy;
} host_interface;
```
```
host_interface interface = {
    .init = host_init,
    .send = host_send,
    .destroy = host_destroy
};
```
This interface is passed to application manager by calling
```
app_manager_startup(&interface);
```

The `host_init_func` is called when the application manager starts up. And `host_send_fun` is called by the application manager to send data to the host.
>**Note:** Currently application manager keeps running and never exit, `host_destroy_fun` has no chance to get executed. So you can leave this API implementation empty.

- src/main.c<br/>
  The main file.
- wasm-apps<br/>
  Source files of sample wasm applications.

Configure 32 bit or 64 bit build
==============
On 64 bit operating system, there is an option to build 32 bit or 64 bit binaries. In file `CMakeLists.txt`, modify the line:
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

Build all binaries
==============
Execute the build.sh script then all binaries including wasm application files would be generated in 'out' directory.
`./build.sh`

Out directory structure
------------------------------
```
out/
├── host_tool
├── simple
└── wasm-apps
    ├── connection.wasm
    ├── event_publisher.wasm
    ├── event_subscriber.wasm
    ├── gui.wasm
    ├── request_handler.wasm
    ├── request_sender.wasm
    ├── sensor.wasm
    └── timer.wasm
```

- host_tool:
  A small testing tool to interact with WAMR. See the usage of this tool by executing "./host_tool -h".
  `./host_tool -h`

- simple:
  A simple testing tool running on the host side that interact with WAMR. It is used to install, uninstall and query WASM applications in WAMR, and send request or subscribe event, etc. See the usage of this application by executing "./simple -h".
  `./simple -h`
>****Note:**** The connection between simple and host_tool is TCP by default and is what this guide uses. The simple application works as a server and the host_tool works as a client. You can also use UART connection. To achieve this you have to uncomment the below line in CMakeLists.txt and rebuild. You have to set up a UART hardware connection between 2 machines one of which runs the host_tool and the other runs the simple application. See the help of host_tool and the simple application to know how to specify UART device parameters.<br/>
`#add_definitions (-DCONNECTION_UART)`

- wasm-apps:
  Sample wasm applications that demonstrate all APIs of the WAMR programming model. The source codes are in the wasm-apps directory under the root of this project.
    + connection.wasm<br/>
    This application shows the connection programming model. It connects to a TCP server on 127.0.0.1:7777 and periodically sends message to it.
    + event_publisher.wasm<br/>
    This application shows the sub/pub programming model. The pub application publishes the event "alert/overheat" by calling api_publish_event() API. The subscriber could be host_tool or other wasm application.
    + event_subscriber.wasm<br/>
    This application shows the sub/pub programming model. The sub application subscribes the "alert/overheat" event by calling api_subscribe_event() API so that it is able to receive the event once generated and published by the pub application. To make the process clear to interpret, the sub application dumps the event when receiving it.
    + gui.wasm<br/>
    This application shows the built-in 2D graphical user interface API with which various widgets could be created.
    + request_handler.wasm<br/>
    This application shows the request/response programming model. The request handler application registers 2 resources(/url1 and /url2) by calling api_register_resource_handler() API. The request sender could be host_tool or other wasm application.
    + request_sender.wasm<br/>
    This application shows the request/response programming model. The sender application sends 2 requests, one is "/app/request_handler/url1" and the other is "url1". The former is an accurate request which explicitly specifies the name of request handler application in the middle of the URL and the later is a general request.
    + sensor.wasm<br/>
    This application shows the sensor programming model. It opens a test sensor and configures the sensor event generating interval to 1 second. To make the process clear to interpret, the application dumps the sensor event when receiving it.
    + timer.wasm<br/>
    This application shows the timer programming model. It creates a periodic timer that prints the current expiry number in every second.

Run the scenario
==========================
- Enter the out directory<br/>
```
$ cd ./out/
```

- Startup the 'simple' process works in TCP server mode and you would see "App Manager started." is printed.<br/>
```
$ ./simple -s
App Manager started.
```

- Query all installed applications<br/>
```
$ ./host_tool -q

response status 69
{
    "num":    0
}
```

The `69` stands for response status to this query request which means query success and a payload is attached with the response. See `{WAMR_ROOT}/core/iwasm/lib/app-libs/base/wasm_app.h` for the definitions of response codes. The payload is printed with JSON format where the `num` stands for application installations number and value `0` means currently no application is installed yet.

- Install the request handler wasm application<br/>
```
$ ./host_tool -i request_handler -f ./wasm-apps/request_handler.wasm

response status 65
```
The `65` stands for response status to this installation request which means success. 

Output of simple
```
Install WASM app success!
sent 16 bytes to host
WASM app 'request_handler' started
```

Now the request handler application is running and waiting for host or other wasm application to send a request.

- Query again<br/>
```
$ ./host_tool -q

response status 69
{
    "num":    1,
    "applet1":    "request_handler",
    "heap1":    49152
}
```
In the payload, we can see `num` is 1 which means 1 application is installed. `applet1`stands for the name of the 1st application. `heap1` stands for the heap size of the 1st application.

- Send request from host to specific wasm application<br/>
```
$ ./host_tool -r /app/request_handler/url1 -A GET

response status 69
{
    "key1":    "value1",
    "key2":    "value2"
}
```

We can see a response with status `69` and a payload is received.

Output of simple
```
connection established!
Send request to applet: request_handler
Send request to app request_handler success.
App request_handler got request, url url1, action 1
[resp] ### user resource 1 handler called
sent 150 bytes to host
Wasm app process request success.
```

- Send a general request from host (not specify target application name)<br/>
```
$ ./host_tool -r /url1 -A GET

response status 69
{
    "key1":    "value1",
    "key2":    "value2"
}
```

Output of simple
```
connection established!
Send request to app request_handler success.
App request_handler got request, url /url1, action 1
[resp] ### user resource 1 handler called
sent 150 bytes to host
Wasm app process request success.
```

- Install the event publisher wasm application<br/>
```
$ ./host_tool -i pub -f ./wasm-apps/event_publisher.wasm

response status 65
```

- Subscribe event by host_tool<br/>
```
$ ./host_tool -s /alert/overheat -a 3000

response status 69

received an event alert/overheat
{
    "warning":    "temperature is over high"
}
received an event alert/overheat
{
    "warning":    "temperature is over high"
}
received an event alert/overheat
{
    "warning":    "temperature is over high"
}
received an event alert/overheat
{
    "warning":    "temperature is over high"
}
```
We can see 4 `alert/overheat` events are received in 3 seconds which is published by the `pub` application.

Output of simple
```
connection established!
am_register_event adding url:(alert/overheat)
client: -3 registered event (alert/overheat)
sent 16 bytes to host
sent 142 bytes to host
sent 142 bytes to host
sent 142 bytes to host
sent 142 bytes to host
```
- Install the event subscriber wasm application<br/>
```
$ ./host_tool -i sub -f ./wasm-apps/event_subscriber.wasm

response status 65
```
The `sub` application is installed.

Output of simple
```
connection established!
Install WASM app success!
WASM app 'sub' started
am_register_event adding url:(alert/overheat)
client: 3 registered event (alert/overheat)
sent 16 bytes to host
Send request to app sub success.
App sub got request, url alert/overheat, action 6
### user over heat event handler called
Attribute container dump:
Tag: 
Attribute list:
  key: warning, type: string, value: temperature is over high

Wasm app process request success.
```

We can see the `sub` application receives the `alert/overheat` event and dumps it out.<br/>
At device side, the event is represented by an attribute container which contains key-value pairs like below:
```
Attribute container dump:
Tag:
Attribute list:
  key: warning, type: string, value: temperature is over high
```
`warning` is the key's name. `string` means this is a string value and `temperature is over high` is the value.

- Uninstall the wasm application<br/>
```
$ ./host_tool -u request_handler

response status 66

$ ./host_tool -u pub

response status 66

$ ./host_tool -u sub

response status 66
```

- Query again<br/>
```
$ ./host_tool -q

response status 69
{
    "num":    0
}
```

  >**Note:** Here we only installed part of the sample WASM applications. You can try others by yourself.

  >**Note:** You have to manually kill the simple process by Ctrl+C after use.
