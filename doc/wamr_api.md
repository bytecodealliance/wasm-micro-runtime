
WAMR application framework
========================



## Application system callbacks

```

```



## Base App library



The base library of application framework supports the essential API for WASM applications, such as inter-app communication, timers, etc. Other application framework components rely on the base library.



When building the WAMR SDK, once application framework is enabled, the base library will automatially enabled.



### Timer






### Micro-service model (request/response)
The microservice model is also known as request and response model. One WASM application acts as the server which provides a specific service. Other WASM applications or host/cloud applications request that service and get the response.
<img src="./pics/request.PNG" width="60%" height="60%">

Below is the reference implementation of the server application. It provides room temperature measurement service.

``` C
void on_init()
{
    api_register_resource_handler("/room_temp", room_temp_handler);
}

void on_destroy() 
{
}

void room_temp_handler(request_t *request)
{
    response_t response[1];
    attr_container_t *payload;
    payload = attr_container_create("room_temp payload");
    if (payload == NULL)
        return;

    attr_container_set_string(&payload, "temp unit", "centigrade");
    attr_container_set_int(&payload, "value", 26);

    make_response_for_request(request, response);
    set_response(response,
                 CONTENT_2_05,
                 FMT_ATTR_CONTAINER,
                 payload,
                 attr_container_get_serialize_length(payload));

    api_response_send(response);
    attr_container_destroy(payload);
}
```


### Pub/sub model
One WASM application acts as the event publisher. It publishes events to notify WASM applications or host/cloud applications which subscribe to the events.

<img src="./pics/sub.PNG" width="60%" height="60%">

Below is the reference implementation of the pub application. It utilizes a timer to repeatedly publish an overheat alert event to the subscriber applications. Then the subscriber applications receive the events immediately.

``` C
/* Timer callback */
void timer_update(user_timer_t timer
{
    attr_container_t *event;

    event = attr_container_create("event");
    attr_container_set_string(&event,
                              "warning",
                              "temperature is over high");

    api_publish_event("alert/overheat",
                      FMT_ATTR_CONTAINER,
                      event,
                      attr_container_get_serialize_length(event));

    attr_container_destroy(event);
}

void on_init()
{
    user_timer_t timer;
    timer = api_timer_create(1000, true, true, timer_update);
}

void on_destroy()
{
}
```

Below is the reference implementation of the sub application.
``` C
void overheat_handler(request_t *event)
{
    printf("Event: %s\n", event->url);

    if (event->payload != NULL && event->fmt == FMT_ATTR_CONTAINER)
       attr_container_dump((attr_container_t *) event->payload);
}

void on_init(
{
    api_subscribe_event ("alert/overheat", overheat_handler);
}

void on_destroy()
{
}
```
**Note:** You can also subscribe this event from host side by using host tool. Please refer `samples/simple` project for deail usage.



## Sensor API

The API set is defined in the header file ```core/app-framework/sensor/app/wa-inc/sensor.h``` :

``` C

```

## Connection API: 

The API set is defined in the header file `core/app-framework/connection/app/wa-inc/connection.h

``` C

```

## GUI API

The API's is list in header file ```core/app-framework/wgl/app/wa-inc/wgl.h``` which is implemented based open soure 2D graphic library [LittlevGL](https://docs.littlevgl.com/en/html/index.html). 



Currently supported widgets include button, label, list and check box and more wigdet would be provided in future.