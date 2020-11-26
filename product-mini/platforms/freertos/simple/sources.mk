# TODO: set WAMR root dir
WAMR_ROOT := ../../../../

override PROJECT_CFLAGS := $(PROJECT_CFLAGS) -Wno-unused-parameter -Wno-pedantic

override PROJECT_CFLAGS := $(PROJECT_CFLAGS) -I$(PROJECTS_SRC_ROOT)/include

override PROJECT_CFLAGS := $(PROJECT_CFLAGS) \
                           -I$(WAMR_INC_ROOT)/core \
                           -I$(WAMR_INC_ROOT)/core/iwasm/include \
                           -I$(WAMR_INC_ROOT)/core/iwasm/common \
                           -I$(WAMR_INC_ROOT)/core/shared/utils \
                           -I$(WAMR_INC_ROOT)/core/shared/mem-alloc \
                           -I$(WAMR_INC_ROOT)/core/shared/platform/include \
                           -I$(WAMR_INC_ROOT)/core/shared/platform/freertos \
                           -I$(WAMR_INC_ROOT)/core/iwasm/interpreter

override PROJECT_CFLAGS := $(PROJECT_CFLAGS) \
                           -DBH_PLATFORM_FREERTOS \
                           -DBH_MALLOC=wasm_runtime_malloc \
                           -DBH_FREE=wasm_runtime_free \
                           -DBUILD_TARGET_X86_32 \
                           -DWASM_ENABLE_INTERP=1 \
                           -DWASM_ENABLE_FAST_INTERP=0 \
                           -DWASM_ENABLE_LIBC_BUILTIN=1

override PROJECT_CSRC := $(PROJECT_CSRC) \
                         $(WAMR_SRC_ROOT)/core/shared/platform/freertos/freertos_platform.c \
                         $(WAMR_SRC_ROOT)/core/shared/platform/freertos/freertos_thread.c \
                         $(WAMR_SRC_ROOT)/core/shared/platform/freertos/freertos_time.c \
                         $(WAMR_SRC_ROOT)/core/shared/platform/common/math/math.c \
                         $(WAMR_SRC_ROOT)/core/shared/mem-alloc/mem_alloc.c \
                         $(WAMR_SRC_ROOT)/core/shared/mem-alloc/ems/ems_kfc.c \
                         $(WAMR_SRC_ROOT)/core/shared/mem-alloc/ems/ems_hmu.c \
                         $(WAMR_SRC_ROOT)/core/shared/mem-alloc/ems/ems_alloc.c \
                         $(WAMR_SRC_ROOT)/core/shared/utils/bh_assert.c \
                         $(WAMR_SRC_ROOT)/core/shared/utils/bh_common.c \
                         $(WAMR_SRC_ROOT)/core/shared/utils/bh_hashmap.c \
                         $(WAMR_SRC_ROOT)/core/shared/utils/bh_list.c \
                         $(WAMR_SRC_ROOT)/core/shared/utils/bh_log.c \
                         $(WAMR_SRC_ROOT)/core/shared/utils/bh_vector.c \
                         $(WAMR_SRC_ROOT)/core/iwasm/libraries/libc-builtin/libc_builtin_wrapper.c \
                         $(WAMR_SRC_ROOT)/core/iwasm/common/wasm_runtime_common.c \
                         $(WAMR_SRC_ROOT)/core/iwasm/common/wasm_exec_env.c \
                         $(WAMR_SRC_ROOT)/core/iwasm/common/wasm_native.c \
                         $(WAMR_SRC_ROOT)/core/iwasm/common/wasm_memory.c \
                         $(WAMR_SRC_ROOT)/core/iwasm/common/wasm_shared_memory.c \
                         $(WAMR_SRC_ROOT)/core/iwasm/common/wasm_c_api.c \
                         $(WAMR_SRC_ROOT)/core/iwasm/common/arch/invokeNative_ia32.s \
                         $(WAMR_SRC_ROOT)/core/iwasm/interpreter/wasm_interp_classic.c \
                         $(WAMR_SRC_ROOT)/core/iwasm/interpreter/wasm_loader.c \
                         $(WAMR_SRC_ROOT)/core/iwasm/interpreter/wasm_runtime.c \
                         $(WAMR_SRC_ROOT)/product-mini/platforms/freertos/simple/iwasm_main.c \
                         $(WAMR_SRC_ROOT)/product-mini/platforms/freertos/simple/main.c
