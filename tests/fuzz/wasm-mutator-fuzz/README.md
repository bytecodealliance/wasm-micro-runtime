# WAMR fuzz test framework

## install wasm-tools

```bash
1.git clone https://github.com/bytecodealliance/wasm-tools
$ cd wasm-tools
2.This project can be installed and compiled from source with this Cargo command:
$ cargo install wasm-tools
3.Installation can be confirmed with:
$ wasm-tools --version
4.Subcommands can be explored with:
$ wasm-tools help
```

## Build

```bash
mkdir build && cd build
# Without custom mutator (libfuzzer modify the buffer randomly)
cmake ..
# With custom mutator (wasm-tools mutate)
cmake .. -DCUSTOM_MUTATOR=1
make -j$(nproc)
```

## Manually generate wasm file in build

```bash
# wasm-tools smith generate some valid wasm file
# The generated wasm file is in corpus_dir under build
# N - Number of files to be generated
./smith_wasm.sh N 

# running
``` bash
cd build
./wasm-mutate-fuzz CORPUS_DIR
 
```

## Fuzzing Server

```shell
1. Installation Dependent Environment
$ cd server
$ pip install -r requirements.txt

2. Database Migration
$ python3 app/manager.py db init
$ python3 app/manager.py db migrate  
$ python3 app/manager.py db upgrade  

3. Change localhost to your machine's IP address
$ cd ../portal 
$ vim .env   # Change localhost to your machine's IP address  # http://<ip>:16667

4. Run Server and Portal
$ cd ..   # Switch to the original directory
If you want to customize the front-end deployment port:  # defaut 9999
    $ vim .env # Please change the portal_port to the port you want to use 

The server is deployed on port 16667 by default, If you want to change the server deployment port:
    $ vim .env # Please change the server_port to the port you want to use 
    $ vim portal/.env # Please change the VITE_SERVER_URL to the port you want to use  # http://ip:<port>


If your network needs to set up a proxy
    $ vim .env # Change proxy to your proxy address

$ docker-compose up --build -d
Wait for completion, Access the port set by env
```
