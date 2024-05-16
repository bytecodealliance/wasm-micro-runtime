CC = /home/user/wasi-sdk-21.0/bin/clang
AR = /home/user/wasi-sdk-21.0/bin/llvm-ar
WAMR_ROOT_DIR = /home/user/wasm-micro-runtime
SYSROOT = /home/user/wasi-sdk-21.0/share/wasi-sysroot
CFLAGS = -O2 --sysroot=$(SYSROOT) -Iinc/ # -nostdlib
LDFLAGS = --sysroot=$(SYSROOT) \
          -L. -lwasi_socket_ext \
          -Wl,-z,stack-size=8192 \
          -Wl,--initial-memory=65536 \
          -Wl,--no-entry -Wl,--strip-all \
          -Wl,--export=__heap_base -Wl,--export=__data_end \
          -Wl,--allow-undefined \
          -Wl,--export=main -Wl,--export=__main_argc_argv \
          -Wl,--export=malloc -Wl,--export=free \
          -nostdlib \

# other linker flags:
# --shared-memory
# -Wl,--export=__wasm_call_ctors \

SRC = http_get.c
OBJ = $(SRC:.c=.o)
LIB_SRC = inc/wasi_socket_ext.c
LIB_OBJ = $(LIB_SRC:.c=.o)
LIB = libwasi_socket_ext.a
EXEC = http_get.wasm

# Default target
all: $(LIB) $(EXEC)

# Link 
$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile
%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

# Compile library object
$(LIB_OBJ): $(LIB_SRC)
	$(CC) -o $@ -c $< $(CFLAGS)

# Create library
$(LIB): $(LIB_OBJ)
	$(AR) rcs $@ $^

# Clean
clean:
	rm -rf $(OBJ) $(LIB_OBJ) $(LIB) $(EXEC)