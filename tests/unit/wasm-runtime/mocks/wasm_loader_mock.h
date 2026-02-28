#ifndef WASM_LOADER_MOCK_H
#define WASM_LOADER_MOCK_H

#include <stdint.h>
#include <stdbool.h>
#include "wasm_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Mock function declarations - only compiled when WAMR_BUILD_TEST is defined */
#ifdef WAMR_BUILD_TEST

WASMModule *
wasm_loader_load(uint8 *buf, uint32 size,
#if WASM_ENABLE_MULTI_MODULE != 0
                 bool main_module,
#endif
                 const LoadArgs *args, char *error_buf, uint32 error_buf_size);

WASMModule *
wasm_loader_load_from_sections(WASMSection *section_list, char *error_buf,
                               uint32 error_buf_size);

#endif /* WAMR_BUILD_TEST */

#ifdef __cplusplus
}
#endif

#endif /* WASM_LOADER_MOCK_H */