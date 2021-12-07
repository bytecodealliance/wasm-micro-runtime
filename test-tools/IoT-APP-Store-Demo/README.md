# IoT Application Store
Wasm application management portal for WAMR

## Start the server

### Using docker
1. install docker and docker-compose
    ``` bash
    sudo apt install docker.io docker-compose
    ```

2. start
    ```
    docker-compose up
    ```
### Using commands
> Note: must use python3.5. If you don't have python3.5 on your machine, had better using docker
1. install the required package
    ```
    pip3 install django
    ```

2. Start device server
    ```
    cd wasm_django/server
    python3 wasm_server.py
    ```

3. Start IoT application management web portal
    ```
    cd wasm_django
    python3 manage.py runserver 0.0.0.0:80
    ```

## Start the runtime
1. Download WAMR runtime from [help](http://localhost/help/) page
    > NOTE: You need to start the server before accessing this link!

5. Start a WAMR runtime from localhost
    ```
    ./simple
    ```
    or from other computers
    ```
    ./simple -a [your.server.ip.address]
    ```

## Online demo
    http://82.156.57.236/
