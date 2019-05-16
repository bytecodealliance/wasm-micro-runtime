Introduction
==============
This project aims to demonstrate wasm app management and programming model of WAMR.

Build all binaries
==============
Execute the build.sh script then all binaries including wasm application files would be generated in 'out' directory.
`./build.sh`

Out directory structure
------------------------------
 <pre>
out/
├── host_tool
├── simple
└── wasm-apps
    ├── event_publisher.wasm
    ├── event_subscriber.wasm
    ├── request_handler.wasm
    ├── request_sender.wasm
    ├── sensor.wasm
    └── timer.wasm
 </pre>

- host_tool:
  A small testing tool to interact with WAMR. See the usage of this tool by executing "./host_tool -h".
  `./host_tool -h`

- simple:
  The simple application with WAMR runtime built in. See the usage of this application by executing "./simple -h".
  `./simple -h`
>Note: The connection between simple and host_tool is TCP by default and this guide uses default connection. You can also use the UART mode. To achieve this you have to uncomment the below line in CMakeLists.txt and rebuild. You have to set up a UART hardware connection between 2 machines one of which runs the host_tool and the other runs the simple application. See the help of host_tool and the simple application to know how to specify UART device parameters.
`#add_definitions (-DCONNECTION_UART)`

- wasm-apps:
  Sample wasm applications that demonstrate all APIs of the WAMR programming model. The source codes are in the wasm-apps directory under the root of this project.
    + event_publisher.wasm
    This application shows the sub/sub programming model. The pub application publishes the event "alert/overheat" by calling api_publish_event() API. The subscriber could be host_tool or other wasm application.
    + event_subscriber.wasm
    This application shows the sub/pub programming model. The sub application subscribes the "alert/overheat" event by calling api_subscribe_event() API so that it is able to receive the event once generated and published by the pub application. To make the process clear to interpret, the sub application dumps the event when receiving it.
    + request_handler.wasm
    This application shows the request/response programming model. The request handler application registers 2 resources(/url1 and /url2) by calling api_register_resource_handler() API. The request sender could be host_tool or other wasm application.
    + request_sender.wasm
    This application shows the request/response programming model. The sender application sends 2 requests, one is "/app/request_handler/url1" and the other is "url1". The former is an accurate request which explicitly specifies the name of request handler application in the middle of the URL and the later is a general request.
    + sensor.wasm
    This application shows the sensor programming model. It opens a test sensor and configures the sensor event generating interval to 1 second. To make the process clear to interpret, the application dumps the sensor event when receiving it.
    + timer.wasm
    This application shows the timer programming model. It creates a periodic timer that prints the current expiry number in every second.

Run
==========================
- Enter the out directory
  `cd ./out/`

- Startup the 'simple' process works in TCP server mode
  `./simple -s`

  You would see "App Manager started." is printed.
  `App Manager started.`

- Query all installed applications
  `./host_tool -q`

- Install the request handler wasm application
  `./host_tool -i request_handler -f ./wasm-apps/request_handler.wasm`

- Send request to specific wasm application
  `./host_tool -r /app/request_handler/url1 -A GET`

- Send a general request (not specify target application name)
  `./host_tool -r /url1 -A GET`

- Install the event publisher wasm application
  `./host_tool -i pub -f ./wasm-apps/event_publisher.wasm`

- Subscribe event by host_tool
  `./host_tool -s /alert/overheat -a 3000`

- Install the event subscriber wasm application
  `./host_tool -i sub -f ./wasm-apps/event_subscriber.wasm`

- Uninstall the wasm app
  `./host_tool -u request_handler`
  `./host_tool -u pub`
  `./host_tool -u sub`

  >Note: You have to manually kill the simple process by Ctrl+C after use.

Output example
---------------------------------

**Output of simple**
```
$ ./simple -s
App Manager started.
connection established!
sent 137 bytes to host
Query Applets success!
Attribute container dump:
Tag: Applets Info
Attribute list:
  key: num, type: int, value: 0x0

connection lost, and waiting for client to reconnect...
connection established!
Install WASM app success!
sent 16 bytes to host
WASM app 'request_handler' started
connection lost, and waiting for client to reconnect...
connection established!
Send request to applet: request_handler
Send request to app request_handler success.
App request_handler got request, url url1, action 1
[resp] ### user resource 1 handler called
sent 150 bytes to host
Wasm app process request success.
connection lost, and waiting for client to reconnect...
connection established!
Send request to app request_handler success.
App request_handler got request, url /url1, action 1
[resp] ### user resource 1 handler called
sent 150 bytes to host
Wasm app process request success.
connection lost, and waiting for client to reconnect...
connection established!
sent 137 bytes to host
Query Applets success!
Attribute container dump:
Tag: Applets Info
Attribute list:
  key: num, type: int, value: 0x1
  key: applet1, type: string, value: request_handler
  key: heap1, type: int, value: 0xc000

connection lost, and waiting for client to reconnect...
connection established!
Install WASM app success!
sent 16 bytes to host
WASM app 'pub' started
connection lost, and waiting for client to reconnect...
connection established!
Install WASM app success!
sent 16 bytes to host
WASM app 'sub' started
am_register_event adding url:(alert/overheat)
client: 3 registered event (alert/overheat)
connection lost, and waiting for client to reconnect...
Send request to app sub success.
App sub got request, url alert/overheat, action 6
### user over heat event handler called
Attribute container dump:
Tag: 
Attribute list:
  key: warning, type: string, value: temperature is over high

Wasm app process request success.
Send request to app sub success.
App sub got request, url alert/overheat, action 6
### user over heat event handler called
Attribute container dump:
Tag: 
Attribute list:
  key: warning, type: string, value: temperature is over high

Wasm app process request success.
connection established!
am_register_event adding url:(alert/overheat)
client: -3 registered event (alert/overheat)
sent 16 bytes to host
sent 142 bytes to host
Send request to app sub success.
App sub got request, url alert/overheat, action 6
### user over heat event handler called
Attribute container dump:
Tag: 
Attribute list:
  key: warning, type: string, value: temperature is over high

Wasm app process request success.
Send request to app sub success.
App sub got request, url alert/overheat, action 6
### user over heat event handler called
Attribute container dump:
Tag: 
Attribute list:
  key: warning, type: string, value: temperature is over high

Wasm app process request success.
connection established!
sent 266 bytes to host
Query Applets success!
Attribute container dump:
Tag: Applets Info
Attribute list:
  key: num, type: int, value: 0x3
  key: applet1, type: string, value: sub
  key: heap1, type: int, value: 0xc000
  key: applet2, type: string, value: pub
  key: heap2, type: int, value: 0xc000
  key: applet3, type: string, value: request_handler
  key: heap3, type: int, value: 0xc000

connection lost, and waiting for client to reconnect...
Send request to app sub success.
App sub got request, url alert/overheat, action 6
### user over heat event handler called
Attribute container dump:
Tag: 
Attribute list:
  key: warning, type: string, value: temperature is over high

Wasm app process request success.
Send request to app sub success.
App sub got request, url alert/overheat, action 6
### user over heat event handler called
Attribute container dump:
Tag: 
Attribute list:
  key: warning, type: string, value: temperature is over high

Wasm app process request success.
connection established!
App instance main thread exit.
client: 3 deregistered event (alert/overheat)
Uninstall WASM app successful!
sent 16 bytes to host
connection lost, and waiting for client to reconnect...
connection established!
App instance main thread exit.
Uninstall WASM app successful!
sent 16 bytes to host
connection lost, and waiting for client to reconnect...
connection established!
App instance main thread exit.
Uninstall WASM app successful!
sent 16 bytes to host
connection lost, and waiting for client to reconnect...
connection established!
sent 137 bytes to host
Query Applets success!
Attribute container dump:
Tag: Applets Info
Attribute list:
  key: num, type: int, value: 0x0

connection lost, and waiting for client to reconnect...
^C

```

**Output of host_tool**
```
$ ./host_tool -q

response status 69
{
	"num":	1,
	"applet1":	"request_handler",
	"heap1":	49152
}
$ ./host_tool -i pub -f ./wasm-apps/event_publisher.wasm

response status 65
$ ./host_tool -i sub -f ./wasm-apps/event_subscriber.wasm

response status 65
$ ./host_tool -s /alert/overheat -a 3000

response status 69

received an event alert/overheat
{
	"warning":	"temperature is over high"
}
received an event alert/overheat
{
	"warning":	"temperature is over high"
}
received an event alert/overheat
{
	"warning":	"temperature is over high"
}
received an event alert/overheat
{
	"warning":	"temperature is over high"
$ ./host_tool -q

response status 69
{
	"num":	3,
	"applet1":	"sub",
	"heap1":	49152,
	"applet2":	"pub",
	"heap2":	49152,
	"applet3":	"request_handler",
	"heap3":	49152
$ ./host_tool -u sub

response status 66
$ ./host_tool -u pub

response status 66
$ ./host_tool -u request_handler

response status 66
$ ./host_tool -q

response status 69
{
	"num":	0
}

```
