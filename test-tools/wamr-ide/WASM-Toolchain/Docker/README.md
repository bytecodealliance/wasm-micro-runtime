### Build Docker Image

-   Linux

    ```shell
    chmod +x resource/*
    ./build_docker_image.sh
    ```

-   Windows

    ```shell
    ./build_docker_image.bat
    ```

### Resource Details

-   `Dockerflie` is the source file to build `wasm-debug-server` docker image.
-   `resource/build_wasm.sh` is the script to compile the wasm app with `wasi-sdk`.