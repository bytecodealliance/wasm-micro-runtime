#ifndef _AOT_GC_OBJECT_WRAPPER_H_
#define _AOT_GC_OBJECT_WRAPPER_H_

#include "../aot_compiler.h"
#include "stdbool.h"
#include "../aot/aot_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
aot_call_wasm_create_func_obj(AOTCompContext *comp_ctx,
                              AOTFuncContext *func_ctx, LLVMValueRef func_idx,
                              LLVMValueRef *p_gc_obj);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _AOT_GC_OBJECT_WRAPPER_H_ */
