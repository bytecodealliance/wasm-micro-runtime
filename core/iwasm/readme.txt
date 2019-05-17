
1. How to build iwasm in Linux

  cd products/linux/
  mkdir build && cd build
  cmake .. && make

2. How to build iwasm in Zephyr system
  (1) clone zephyr source code

    git clone https://github.com/zephyrproject-rtos/zephyr.git

  (2) copy <iwasm_root_dir>/products/zephyr/simple directory to zephry/samples/

    cd zephyr/samples/
    cp -a <iwasm_root_dir>/products/zephyr/simple simple
    cd simple

  (3) create a link to <iwasm_root_dir> and rename it to iwasm

    ln -s <iwasm_root_dir> iwasm

  (4) create a link to <shared-lib_root_dir> and rename it to shared-lib

    ln -s <shared-lib_root_dir> shared-lib

  (5) build source code

    mkdir build && cd build
    source ../../../zephyr-env.sh
    cmake -GNinja -DBOARD=qemu_x86 ..
    ninja

  (6) run simple

    ninja run

3. How to build iwasm in AliOS-Things platform
  (1) clone AliOS-Things source code

    git clone https://github.com/alibaba/AliOS-Things.git

  (2) copy <iwasm_root_dir>/products/alios-things directory to AliOS-Things/middleware,
      and rename it as iwasm

    cp -a <iwasm_root_dir>/products/alios-things middleware/iwasm

  (3) create a link to <iwasm_root_dir> in middleware/iwasm/ and rename it to iwasm

    ln -s <iwasm_root_dir> middleware/iwasm/iwasm

  (4) create a link to <shared-lib_root_dir> in middleware/iwasm/ and rename it to shared-lib

    ln -s <shared-lib_root_dir> middle/iwasm/shared-lib

  (5) modify sample source code of AliOS-Things to call iwasm_init() function

    modify file app/example/helloworld/helloworld.c, in the beginning of
    function application_start(), add:

      bool iwasm_init(void);
      iwasm_init();

    modify file app/example/helloworld/helloworld.mk, change
      $(NAME)_COMPONENTS := yloop cli
    to
      $(NAME)_COMPONENTS := yloop cli iwasm

  (6) build source code

    aos make helloworld@linuxhost

  (7) run source code
    ./out/helloworld@linuxhost/binary/helloworld@linuxhost.elf

