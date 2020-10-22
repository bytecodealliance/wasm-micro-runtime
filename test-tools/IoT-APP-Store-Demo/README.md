# IoT Application Store
Wasm application management portal for WAMR

# Requirement
Install django with pip3  
```
pip3 install django
```

# Run 
1. Start wasm server  
    ```
    cd wasm_django/server
    python3 wasm_server.py
    ```

2. Start IoT application management web portal  
    ```
    cd wasm_django
    python3 manage.py runserver 0.0.0.0:80
    ```

3. Download WAMR runtime from [help](http://localhost/help/) page
    > NOTE: You need to start web server according to *step 2* before accessing this link!

4. Start a WAMR runtime from localhost  
    ```
    ./simple
    ```
    or from other computers
    ```
    ./simple -a [your.server.ip.address]
    ```

# Online demo
    http://39.106.110.7/
