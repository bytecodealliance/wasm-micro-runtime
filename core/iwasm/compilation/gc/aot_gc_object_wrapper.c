#include "aot_gc_object_wrapper.h"

#if WASM_ENABLE_GC != 0

#define BUILD_ICMP(op, left, right, res, name)                                \
    do {                                                                      \
        if (!(res =                                                           \
                  LLVMBuildICmp(comp_ctx->builder, op, left, right, name))) { \
            aot_set_last_error("llvm build icmp failed.");                    \
            goto fail;                                                        \
        }                                                                     \
    } while (0)

#define ADD_BASIC_BLOCK(block, name)                                          \
    do {                                                                      \
        if (!(block = LLVMAppendBasicBlockInContext(comp_ctx->context,        \
                                                    func_ctx->func, name))) { \
            aot_set_last_error("llvm add basic block failed.");               \
            goto fail;                                                        \
        }                                                                     \
    } while (0)

bool
aot_call_wasm_create_func_obj(AOTCompContext *comp_ctx,
                              AOTFuncContext *func_ctx, LLVMValueRef func_idx,
                              LLVMValueRef *p_gc_obj)
{
    LLVMValueRef gc_obj, cmp_gc_obj, param_values[5], func, value;
    LLVMTypeRef param_types[5], ret_type, func_type, func_ptr_type;
    AOTFuncType *aot_func_type = func_ctx->aot_func->func_type;
    LLVMBasicBlockRef block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMBasicBlockRef init_gc_obj_fail, init_gc_obj_succ;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = INT8_TYPE;
    param_types[3] = INT8_PTR_TYPE;
    param_types[4] = I32_TYPE;
    ret_type = INT8_TYPE;

    /* TODO: do we need modify gc APIs to support AOTModule? */
    GET_AOT_FUNCTION(wasm_create_func_obj, 5);

    /* Call function wasm_create_func_obj() */
    param_values[0] = func_ctx->aot_inst;
    param_values[1] = func_idx;
    param_values[2] = I8_CONST(1);
    param_values[3] = I8_PTR_NULL;
    param_values[4] = I32_ZERO;
    if (!(gc_obj = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                  param_values, 5, "call"))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    BUILD_ICMP(LLVMIntNE, gc_obj, OBJ_REF_NULL, cmp_gc_obj,
               "result whether gc_obj is null");

    ADD_BASIC_BLOCK(init_gc_obj_fail, "init_gc_obj_fail");
    ADD_BASIC_BLOCK(init_gc_obj_succ, "init_gc_obj_success");

    LLVMMoveBasicBlockAfter(init_gc_obj_fail, block_curr);
    LLVMMoveBasicBlockAfter(init_gc_obj_succ, block_curr);

    if (!LLVMBuildCondBr(comp_ctx->builder, cmp_gc_obj, init_gc_obj_succ,
                         init_gc_obj_fail)) {
        aot_set_last_error("llvm build cond br failed.");
        goto fail;
    }

    /* If init gc_obj failed, return this function
       so the runtime can catch the exception */
    LLVMPositionBuilderAtEnd(comp_ctx->builder, init_gc_obj_fail);
    if (!aot_build_zero_function_ret(comp_ctx, func_ctx, aot_func_type)) {
        goto fail;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, init_gc_obj_succ);
    *p_gc_obj = gc_obj;

    return true;
fail:
    return false;
}

#endif
