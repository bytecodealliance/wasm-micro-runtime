/*
 * The WebAssembly Live Migration Project
 *
 *  By: Aibo Hu
 *      Yiwei Yang
 *      Brian Zhao
 *      Andrew Quinn
 *
 *  Copyright 2024 Regents of the Univeristy of California
 *  UC Santa Cruz Sluglab.
 */

#ifndef MVVM_WAMR_TYPE_H
#define MVVM_WAMR_TYPE_H
#include "wasm_runtime.h"
#include "wamr_serializer.h"

struct WAMRType {
    uint16 param_count;
    uint16 result_count;
    uint16 param_cell_num;
    uint16 ret_cell_num;
    uint16 ref_count;
    /* types of params and results */
    void dump_impl(WASMType *env)
    {
        param_count = env->param_count;
        result_count = env->result_count;
        param_cell_num = env->param_cell_num;
        ret_cell_num = env->ret_cell_num;
        ref_count = env->ref_count;
    };
    bool equal_impl(WASMType *type) const
    {
        return param_count == type->param_count
               && result_count == type->result_count
               && param_cell_num == type->param_cell_num
               && ret_cell_num == type->ret_cell_num
               && ref_count == type->ref_count;
    };
};
#if __has_include(<expected>) && __cplusplus > 202002L
template<CheckerTrait<WASMType *> T>
void
dump(T t, WASMType *env)
{
    t->dump_impl(env);
}
template<CheckerTrait<WASMType *> T>
bool
equal(T t, WASMType *env)
{
    t->equal_impl(env);
}
#else
void
dump(WAMRType *t, WASMType *env)
{
    t->dump_impl(env);
};
void
equal(WAMRType *t, WASMType *env)
{
    return t->equal_impl(env);
};
#endif
#endif // MVVM_WAMR_TYPE_H
