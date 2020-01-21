/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _EXT_LIB_EXPORT_H_
#define _EXT_LIB_EXPORT_H_

#include "lib_export.h"

#ifdef __cplusplus
extern "C" {
#endif

int
get_ext_lib_export_apis(NativeSymbol **p_ext_lib_apis)
{
    *p_ext_lib_apis = extended_native_symbol_defs;
    return sizeof(extended_native_symbol_defs) / sizeof(NativeSymbol);
}

#ifdef __cplusplus
}
#endif

#endif /* end of _EXT_LIB_EXPORT_H_ */

