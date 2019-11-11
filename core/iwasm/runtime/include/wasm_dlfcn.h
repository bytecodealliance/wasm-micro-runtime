/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_DLFCN_H
#define _WASM_DLFCN_H

#ifdef __cplusplus
extern "C" {
#endif

void *
wasm_dlsym(void *handle, const char *symbol);

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_DLFCN_H */

