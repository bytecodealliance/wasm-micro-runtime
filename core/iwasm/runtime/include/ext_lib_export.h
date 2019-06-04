/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

