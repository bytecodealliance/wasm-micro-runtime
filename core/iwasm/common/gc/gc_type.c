/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gc_type.h"

void
wasm_dump_value_type(uint8 type, const WASMRefType *ref_type)
{
    switch (type) {
        case VALUE_TYPE_I32:
            os_printf("i32");
            break;
        case VALUE_TYPE_I64:
            os_printf("i64");
            break;
        case VALUE_TYPE_F32:
            os_printf("f32");
            break;
        case VALUE_TYPE_F64:
            os_printf("f64");
            break;
        case VALUE_TYPE_V128:
            os_printf("v128");
            break;
        case PACKED_TYPE_I8:
            os_printf("i8");
            break;
        case PACKED_TYPE_I16:
            os_printf("i16");
            break;
        case REF_TYPE_FUNCREF:
            os_printf("funcref");
            break;
        case REF_TYPE_EXTERNREF:
            os_printf("externref");
            break;
        case REF_TYPE_ANYREF:
            os_printf("anyref");
            break;
        case REF_TYPE_EQREF:
            os_printf("eqref");
            break;
        case REF_TYPE_HT_NULLABLE:
        case REF_TYPE_HT_NON_NULLABLE:
        {
            os_printf("(ref ");
            if (ref_type->ref_ht_common.nullable)
                os_printf("null ");
            if (wasm_is_refheaptype_common(&ref_type->ref_ht_common)) {
                switch (ref_type->ref_ht_common.heap_type) {
                    case HEAP_TYPE_FUNC:
                        os_printf("func");
                        break;
                    case HEAP_TYPE_EXTERN:
                        os_printf("extern");
                        break;
                    case HEAP_TYPE_ANY:
                        os_printf("any");
                        break;
                    case HEAP_TYPE_EQ:
                        os_printf("eq");
                        break;
                    case HEAP_TYPE_I31:
                        os_printf("i31");
                        break;
                    case HEAP_TYPE_DATA:
                        os_printf("data");
                        break;
                }
            }
            else if (wasm_is_refheaptype_typeidx(&ref_type->ref_ht_common)) {
                os_printf("%d", ref_type->ref_ht_typeidx.type_idx);
            }
            else if (wasm_is_refheaptype_rttn(&ref_type->ref_ht_common)) {
                os_printf("(rtt %d %d)", ref_type->ref_ht_rttn.n,
                          ref_type->ref_ht_rttn.type_idx);
            }
            else if (wasm_is_refheaptype_rtt(&ref_type->ref_ht_common)) {
                os_printf("(rtt %d)", ref_type->ref_ht_rtt.type_idx);
            }
            else {
                bh_assert(0);
            }
            os_printf(")");
            break;
        }
        /* (rtt n i) and (rtt i) have been converted into
           (ref (rtt n i)) and (ref (rtt i)) in wasm/aot loader,
           we don't handle them again. */
#if 0
        case REF_TYPE_RTTN:
            os_printf("(rtt %d %d)", ref_type->ref_rttn.n,
                      ref_type->ref_rttn.type_idx);
            break;
        case REF_TYPE_RTT:
            os_printf("(rtt %d)", ref_type->ref_rttn.type_idx);
            break;
#endif
        case REF_TYPE_I31REF:
            os_printf("i31ref");
            break;
        case REF_TYPE_DATAREF:
            os_printf("dataref");
            break;
        default:
            bh_assert(0);
    }
}

void
wasm_dump_func_type(const WASMFuncType *type)
{
    uint32 i, j = 0;
    const WASMRefType *ref_type = NULL;

    os_printf("func [");

    for (i = 0; i < type->param_count; i++) {
        if (wasm_is_type_multi_byte_type(type->types[i])) {
            bh_assert(j < type->ref_type_map_count);
            bh_assert(i == type->ref_type_maps[j].index);
            ref_type = type->ref_type_maps[j++].ref_type;
        }
        else
            ref_type = NULL;
        wasm_dump_value_type(type->types[i], ref_type);
        if (i < (uint32)type->param_count - 1)
            os_printf(" ");
    }

    os_printf("] -> [");

    for (; i < type->param_count + type->result_count; i++) {
        if (wasm_is_type_multi_byte_type(type->types[i])) {
            bh_assert(j < type->ref_type_map_count);
            bh_assert(i == type->ref_type_maps[j].index);
            ref_type = type->ref_type_maps[j++].ref_type;
        }
        else
            ref_type = NULL;
        wasm_dump_value_type(type->types[i], ref_type);
        if (i < (uint32)type->param_count + type->result_count - 1)
            os_printf(" ");
    }

    os_printf("]\n");
}

void
wasm_dump_struct_type(const WASMStructType *type)
{
    uint32 i, j = 0;
    const WASMRefType *ref_type = NULL;

    os_printf("struct");

    for (i = 0; i < type->field_count; i++) {
        os_printf(" (field ");
        if (type->fields[i].field_flags & 1)
            os_printf("(mut ");
        if (wasm_is_type_multi_byte_type(type->fields[i].field_type)) {
            bh_assert(j < type->ref_type_map_count);
            bh_assert(i == type->ref_type_maps[j].index);
            ref_type = type->ref_type_maps[j++].ref_type;
        }
        else
            ref_type = NULL;
        wasm_dump_value_type(type->fields[i].field_type, ref_type);
        if (type->fields[i].field_flags & 1)
            os_printf(")");
        os_printf(")");
    }

    os_printf("\n");
}

void
wasm_dump_array_type(const WASMArrayType *type)
{
    os_printf("array ");

    if (type->elem_flags & 1)
        os_printf("(mut ");
    wasm_dump_value_type(type->elem_type, type->elem_ref_type);
    if (type->elem_flags & 1)
        os_printf(")");
    os_printf("\n");
}

bool
wasm_func_type_equal(const WASMFuncType *type1, const WASMFuncType *type2)
{
    uint32 i, j = 0;

    if (type1 == type2)
        return true;

    if (type1->param_count != type2->param_count
        || type1->result_count != type2->result_count
        || type1->ref_type_map_count != type2->ref_type_map_count)
        return false;

    for (i = 0; i < type1->param_count + type1->result_count; i++) {
        if (type1->types[i] != type2->types[i])
            return false;
        if (wasm_is_type_multi_byte_type(type1->types[i])) {
            const WASMRefType *ref_type1, *ref_type2;

            bh_assert(j < type1->ref_type_map_count);
            bh_assert(i == type1->ref_type_maps[j].index
                      && i == type2->ref_type_maps[j].index);

            ref_type1 = type1->ref_type_maps[j].ref_type;
            ref_type2 = type2->ref_type_maps[j].ref_type;
            if (!wasm_reftype_equal(ref_type1->ref_type, ref_type1,
                                    ref_type2->ref_type, ref_type2))
                return false;

            j++;
        }
    }

    return true;
}

bool
wasm_struct_type_equal(const WASMStructType *type1, const WASMStructType *type2)
{
    uint32 i, j = 0;

    if (type1 == type2)
        return true;

    if (type1->field_count != type2->field_count
        || type1->ref_type_map_count != type2->ref_type_map_count)
        return false;

    for (i = 0; i < type1->field_count; i++) {
        if (type1->fields[i].field_type != type2->fields[i].field_type
            || type1->fields[i].field_flags != type2->fields[i].field_flags)
            return false;

        if (wasm_is_type_multi_byte_type(type1->fields[i].field_type)) {
            const WASMRefType *ref_type1, *ref_type2;

            bh_assert(j < type1->ref_type_map_count);
            bh_assert(i == type1->ref_type_maps[j].index
                      && i == type2->ref_type_maps[j].index);

            ref_type1 = type1->ref_type_maps[j].ref_type;
            ref_type2 = type2->ref_type_maps[j].ref_type;
            if (!wasm_reftype_equal(ref_type1->ref_type, ref_type1,
                                    ref_type2->ref_type, ref_type2))
                return false;

            j++;
        }
    }

    return true;
}

bool
wasm_array_type_equal(const WASMArrayType *type1, const WASMArrayType *type2)
{
    if (type1 == type2)
        return true;

    if (type1->elem_flags != type2->elem_flags)
        return false;

    if (!wasm_reftype_equal(type1->elem_type, type1->elem_ref_type,
                            type2->elem_type, type2->elem_ref_type))
        return false;

    return true;
}

bool
wasm_type_equal(const WASMType *type1, const WASMType *type2)
{
    if (type1 == type2)
        return true;

    if (type1->type_flag != type2->type_flag)
        return false;

    if (wasm_type_is_func_type(type1))
        return wasm_func_type_equal((WASMFuncType *)type1,
                                    (WASMFuncType *)type2);
    else if (wasm_type_is_struct_type(type1))
        return wasm_struct_type_equal((WASMStructType *)type1,
                                      (WASMStructType *)type2);
    else if (wasm_type_is_array_type(type1))
        return wasm_array_type_equal((WASMArrayType *)type1,
                                     (WASMArrayType *)type2);

    bh_assert(0);
    return false;
}

bool
wasm_func_type_is_subtype_of(const WASMFuncType *type1,
                             const WASMFuncType *type2, const WASMType **types,
                             uint32 type_count)
{
    /**
     * The GC proposal overview defines:
     * A function type is a supertype of another function type if
     * they have the same number of parameters and results, and:
     *   For each parameter, the supertype's parameter type is a subtype of
     *     the subtype's parameter type (contravariance).
     *   For each result, the supertype's parameter type is a supertype of
     *     the subtype's parameter type (covariance).
     * But from our testing, the spec's interpreter doesn't support it yet,
     * we just check whether the two function types are the same or not.
     */
    return wasm_func_type_equal(type1, type2);
}

bool
wasm_struct_type_is_subtype_of(const WASMStructType *type1,
                               const WASMStructType *type2,
                               const WASMType **types, uint32 type_count)
{
    const WASMRefType *ref_type1 = NULL, *ref_type2 = NULL;
    uint32 i, j1 = 0, j2 = 0;

    /**
     * A structure type is a supertype of another structure type if
     *   its field list is a prefix of the other (width subtyping).
     * A structure type also is a supertype of another structure type
     *   if they have the same fields and for each field type:
     *     The field is mutable in both types and the storage types
     *       are the same.
     *     The field is immutable in both types and their storage types
     *       are in (covariant) subtype relation (depth subtyping).
     */

    if (type1 == type2)
        return true;

    if (type1->field_count > type2->field_count) {
        /* Check whether type1's field list is a prefix of type2 */
        for (i = 0; i < type2->field_count; i++) {
            if (type1->fields[i].field_flags != type2->fields[i].field_flags
                || type1->fields[i].field_type != type2->fields[i].field_type)
                return false;
            if (wasm_is_type_multi_byte_type(type1->fields[i].field_type)) {
                bh_assert(j1 < type1->ref_type_map_count);
                bh_assert(j2 < type2->ref_type_map_count);
                ref_type1 = type1->ref_type_maps[j1++].ref_type;
                ref_type2 = type2->ref_type_maps[j2++].ref_type;
                if (!wasm_reftype_equal(ref_type1->ref_type, ref_type1,
                                        ref_type2->ref_type, ref_type2))
                    return false;
            }
        }
        return true;
    }
    else if (type1->field_count == type2->field_count) {
        /* Check each field's flag and type */
        for (i = 0; i < type1->field_count; i++) {
            if (type1->fields[i].field_flags != type2->fields[i].field_flags)
                return false;

            if (type1->fields[i].field_flags & 1) {
                /* The field is mutable in both types: the storage types
                   must be the same */
                if (type1->fields[i].field_type != type2->fields[i].field_type)
                    return false;
                if (wasm_is_type_multi_byte_type(type1->fields[i].field_type)) {
                    bh_assert(j1 < type1->ref_type_map_count);
                    bh_assert(j2 < type2->ref_type_map_count);
                    ref_type1 = type1->ref_type_maps[j1++].ref_type;
                    ref_type2 = type2->ref_type_maps[j2++].ref_type;
                    if (!wasm_reftype_equal(ref_type1->ref_type, ref_type1,
                                            ref_type2->ref_type, ref_type2))
                        return false;
                }
            }
            else {
                /* The field is immutable in both types: their storage types
                   must be in (covariant) subtype relation (depth subtyping) */
                if (wasm_is_type_multi_byte_type(type1->fields[i].field_type)) {
                    bh_assert(j1 < type1->ref_type_map_count);
                    ref_type1 = type1->ref_type_maps[j1++].ref_type;
                }
                if (wasm_is_type_multi_byte_type(type2->fields[i].field_type)) {
                    bh_assert(j2 < type2->ref_type_map_count);
                    ref_type2 = type2->ref_type_maps[j2++].ref_type;
                }
                if (!wasm_reftype_is_subtype_of(type1->fields[i].field_type,
                                                ref_type1,
                                                type2->fields[i].field_type,
                                                ref_type2, types, type_count))
                    return false;
            }
        }
        return true;
    }

    return false;
}

bool
wasm_array_type_is_subtype_of(const WASMArrayType *type1,
                              const WASMArrayType *type2,
                              const WASMType **types, uint32 type_count)
{
    /**
     * An array type is a supertype of another array type if:
     *   Both element types are mutable and the storage types are the same.
     *   Both element types are immutable and their storage types are in
     *     (covariant) subtype relation (depth subtyping).
     */

    if (type1->elem_flags != type2->elem_flags)
        return false;

    if (type1->elem_flags & 1) {
        /* The elem is mutable in both types: the storage types
           must be the same */
        return wasm_reftype_equal(type1->elem_type, type1->elem_ref_type,
                                  type2->elem_type, type2->elem_ref_type);
    }
    else {
        /* The elem is immutable in both types: their storage types
           must be in (covariant) subtype relation (depth subtyping) */
        return wasm_reftype_is_subtype_of(
            type1->elem_type, type1->elem_ref_type, type2->elem_type,
            type2->elem_ref_type, types, type_count);
    }
    return false;
}

bool
wasm_type_is_subtype_of(const WASMType *type1, const WASMType *type2,
                        const WASMType **types, uint32 type_count)
{
    if (type1 == type2)
        return true;

    if (type1->type_flag != type2->type_flag)
        return false;

    if (wasm_type_is_func_type(type1))
        return wasm_func_type_is_subtype_of(
            (WASMFuncType *)type1, (WASMFuncType *)type2, types, type_count);
    else if (wasm_type_is_struct_type(type1))
        return wasm_struct_type_is_subtype_of((WASMStructType *)type1,
                                              (WASMStructType *)type2, types,
                                              type_count);
    else if (wasm_type_is_array_type(type1))
        return wasm_array_type_is_subtype_of(
            (WASMArrayType *)type1, (WASMArrayType *)type2, types, type_count);

    bh_assert(0);
    return false;
}

uint32
wasm_reftype_size(uint8 type)
{
    if (type == VALUE_TYPE_I32 || type == VALUE_TYPE_F32)
        return 4;
    else if (type == VALUE_TYPE_I64 || type == VALUE_TYPE_F64)
        return 8;
    else if (type >= (uint8)REF_TYPE_DATAREF && type <= (uint8)REF_TYPE_FUNCREF)
        return sizeof(uintptr_t);
    else if (type == PACKED_TYPE_I8)
        return 1;
    else if (type == PACKED_TYPE_I16)
        return 2;
    else if (type == VALUE_TYPE_V128)
        return 16;
    else {
        bh_assert(0);
        return 0;
    }

    return 0;
}

uint32
wasm_reftype_struct_size(const WASMRefType *ref_type)
{
    if (wasm_is_reftype_htref_nullable(ref_type->ref_type)
        || wasm_is_reftype_htref_non_nullable(ref_type->ref_type)) {
        if (wasm_is_refheaptype_rttn(&ref_type->ref_ht_common)) {
            return (uint32)sizeof(RefHeapType_RttN);
        }
        else if (wasm_is_refheaptype_rtt(&ref_type->ref_ht_common)) {
            return (uint32)sizeof(RefHeapType_Rtt);
        }
        else if (wasm_is_refheaptype_common(&ref_type->ref_ht_common)
                 || wasm_is_refheaptype_typeidx(&ref_type->ref_ht_common)) {
            return (uint32)sizeof(RefHeapType_Common);
        }
    }
    else if (wasm_is_reftype_rttnref(ref_type->ref_type)) {
        return (uint32)sizeof(RefRttNType);
    }
    else if (wasm_is_reftype_rttref(ref_type->ref_type)) {
        return (uint32)sizeof(RefRttType);
    }

    bh_assert(0);
    return 0;
}

bool
wasm_refheaptype_equal(const RefHeapType_Common *ref_heap_type1,
                       const RefHeapType_Common *ref_heap_type2)
{
    RefHeapType_RttN *ht_rttn1, *ht_rttn2;
    RefHeapType_Rtt *ht_rtt1, *ht_rtt2;

    if (ref_heap_type1 == ref_heap_type2)
        return true;

    if (ref_heap_type1->ref_type != ref_heap_type2->ref_type)
        return false;

    if (ref_heap_type1->heap_type != ref_heap_type2->heap_type)
        return false;

    if (wasm_is_refheaptype_common(ref_heap_type1)
        || wasm_is_refheaptype_typeidx(ref_heap_type1))
        /* No need to extra info for common types and (type i) */
        return true;

    if (wasm_is_refheaptype_rttn(ref_heap_type1)) {
        ht_rttn1 = (RefHeapType_RttN *)ref_heap_type1;
        ht_rttn2 = (RefHeapType_RttN *)ref_heap_type2;
        /* Check n and i of (rtt n i) */
        return (ht_rttn1->n == ht_rttn2->n
                && ht_rttn1->type_idx == ht_rttn2->type_idx)
                   ? true
                   : false;
    }

    if (wasm_is_refheaptype_rtt(ref_heap_type1)) {
        ht_rtt1 = (RefHeapType_Rtt *)ref_heap_type1;
        ht_rtt2 = (RefHeapType_Rtt *)ref_heap_type2;
        /* Check i of (rtt i) */
        return (ht_rtt1->type_idx == ht_rtt2->type_idx) ? true : false;
    }

    bh_assert(0);
    return false;
}

bool
wasm_refrttntype_equal(const RefRttNType *ref_rttn_type1,
                       const RefRttNType *ref_rttn_type2)
{
    return (ref_rttn_type1->n == ref_rttn_type2->n
            && ref_rttn_type1->type_idx == ref_rttn_type2->type_idx)
               ? true
               : false;
}

bool
wasm_refrtttype_equal(const RefRttType *ref_rtt_type1,
                      const RefRttType *ref_rtt_type2)
{
    return (ref_rtt_type1->type_idx == ref_rtt_type2->type_idx) ? true : false;
}

bool
wasm_reftype_equal(uint8 type1, const WASMRefType *reftype1, uint8 type2,
                   const WASMRefType *reftype2)
{
    /* For (ref null func/extern/any/eq/i31/data), they are same as
       funcref/externref/anyref/eqref/i31ref/dataref and have been
       converted into to the related one-byte type when loading, so here
       we don't consider the situations again:
         one is (ref null func/extern/any/eq/i31/data),
         the other is funcref/externref/anyref/eqref/i31ref/dataref. */
    if (type1 != type2)
        return false;

    if (!wasm_is_type_multi_byte_type(type1))
        /* one byte type */
        return true;

    if (type1 == (uint8)REF_TYPE_HT_NULLABLE
        || type1 == (uint8)REF_TYPE_HT_NON_NULLABLE) {
        /* (ref null ht) or (ref ht) */
        return wasm_refheaptype_equal((RefHeapType_Common *)reftype1,
                                      (RefHeapType_Common *)reftype2);
    }
    /* (rtt n i) and (rtt i) have been converted into
       (ref (rtt n i)) and (ref (rtt i)) in wasm/aot loader,
       we don't handle them again. */
#if 0
    else if (type1 == (uint8)REF_TYPE_RTTN) {
        /* (rtt n i) */
        return wasm_refrttntype_equal((RefRttNType *)reftype1,
                                      (RefRttNType *)reftype2);
    }
    else if (type1 == (uint8)REF_TYPE_RTT) {
        /* (rtt i) */
        return wasm_refrtttype_equal((RefRttType *)reftype1,
                                     (RefRttType *)reftype2);
    }
#endif

    bh_assert(0);
    return false;
}

inline static bool
wasm_is_reftype_supers_of_eq(uint8 type)
{
    return (type == REF_TYPE_EQREF || type == REF_TYPE_ANYREF) ? true : false;
}

inline static bool
wasm_is_reftype_supers_of_func(uint8 type)
{
    return (type == REF_TYPE_FUNCREF || type == REF_TYPE_ANYREF) ? true : false;
}

inline static bool
wasm_is_reftype_supers_of_extern(uint8 type)
{
    return (type == REF_TYPE_EXTERNREF || type == REF_TYPE_ANYREF) ? true
                                                                   : false;
}

inline static bool
wasm_is_reftype_supers_of_i31(uint8 type)
{
    return (type == REF_TYPE_I31REF || wasm_is_reftype_supers_of_eq(type))
               ? true
               : false;
}

inline static bool
wasm_is_reftype_supers_of_data(uint8 type)
{
    return (type == REF_TYPE_DATAREF || wasm_is_reftype_supers_of_eq(type))
               ? true
               : false;
}

inline static bool
wasm_is_reftype_supers_of_rtt_nullable(uint8 type_super,
                                       const WASMRefType *ref_type_super,
                                       uint32 type_idx_of_sub)
{
    /* (ref null (rtt i)) <: [ (ref null (rtt i), eqref, anyref ] */

    /* super type is (ref null (rtt i)) */
    if (type_super == REF_TYPE_HT_NULLABLE
        && wasm_is_refheaptype_rtt(&ref_type_super->ref_ht_common)
        && ref_type_super->ref_ht_rtt.type_idx == type_idx_of_sub)
        return true;

    /* super type is eqref or anyref */
    return wasm_is_reftype_supers_of_eq(type_super);
}

inline static bool
wasm_is_reftype_supers_of_rttn_nullable(uint8 type_super,
                                        const WASMRefType *ref_type_super,
                                        uint32 n_of_sub, uint32 type_idx_of_sub)
{
    /* (ref null (rtt n i)) <: [ (ref null (rtt n i), (ref null (rtt i),
                                 eqref, anyref ] */

    /* super type is (ref null (rtt n i) */
    if (type_super == REF_TYPE_HT_NULLABLE
        && wasm_is_refheaptype_rttn(&ref_type_super->ref_ht_common)
        && ref_type_super->ref_ht_rttn.n <= n_of_sub
        && ref_type_super->ref_ht_rttn.type_idx == type_idx_of_sub)
        return true;

    /* super type is (ref null (rtt i), eqref or anyref */
    return wasm_is_reftype_supers_of_rtt_nullable(type_super, ref_type_super,
                                                  type_idx_of_sub);
}

inline static bool
wasm_is_reftype_supers_of_rtt_non_nullable(uint8 type_super,
                                           const WASMRefType *ref_type_super,
                                           uint32 type_idx_of_sub)
{
    /* (ref (rtt i)) <: [ (ref (rtt i), (ref null (rtt i), eqref, anyref ] */

    /* super type is (ref (rtt i) */
    if (type_super == REF_TYPE_HT_NON_NULLABLE
        && wasm_is_refheaptype_rtt(&ref_type_super->ref_ht_common)
        && ref_type_super->ref_ht_rtt.type_idx == type_idx_of_sub)
        return true;

    /* super type is (ref null (rtt i)), eqref or anyref */
    return wasm_is_reftype_supers_of_rtt_nullable(type_super, ref_type_super,
                                                  type_idx_of_sub);
}

inline static bool
wasm_is_reftype_supers_of_rttn_non_nullable(uint8 type_super,
                                            const WASMRefType *ref_type_super,
                                            uint32 n_of_sub,
                                            uint32 type_idx_of_sub)
{
    /* (ref (rtt n i)) <: [ (ref (rtt n i), (ref (rtt i)),
     *                      (ref null (rtt n i)), (ref null (rtt i),
     *                      eqref, anyref ] */

    if (type_super == REF_TYPE_HT_NON_NULLABLE) {
        /* super type is (ref (rtt n i)) */
        if (wasm_is_refheaptype_rttn(&ref_type_super->ref_ht_common)
            && ref_type_super->ref_ht_rttn.n <= n_of_sub
            && ref_type_super->ref_ht_rttn.type_idx == type_idx_of_sub)
            return true;
        /* super type is (ref (rtt i)) */
        else if (wasm_is_refheaptype_rtt(&ref_type_super->ref_ht_common)
                 && ref_type_super->ref_ht_rtt.type_idx == type_idx_of_sub)
            return true;
    }

    /* super type is (ref null (rtt n i), eqref or anyref */
    return wasm_is_reftype_supers_of_rttn_nullable(type_super, ref_type_super,
                                                   n_of_sub, type_idx_of_sub);
}

bool
wasm_reftype_is_subtype_of(uint8 type1, const WASMRefType *ref_type1,
                           uint8 type2, const WASMRefType *ref_type2,
                           const WASMType **types, uint32 type_count)
{
    /* (rtt n i) and (rtt i) have been converted into
       (ref (rtt n i)) and (ref (rtt i)) in wasm/aot loader,
       we don't handle them again. */
    bh_assert(type1 != (uint8)REF_TYPE_RTTN && type1 != (uint8)REF_TYPE_RTT
              && type2 != (uint8)REF_TYPE_RTTN && type2 != (uint8)REF_TYPE_RTT);

    if (type1 >= PACKED_TYPE_I16 && type1 <= VALUE_TYPE_I32) {
        /* Primitive types (I32/I64/F32/F64/V128/I8/I16) are not
           subtypes of each other */
        return type1 == type2 ? true : false;
    }

    /**
     * Check subtype relationship of two ref types, the ref type hierarchy can
     * be described as:
     *
     * anyref -> eqref -> (ref null (rtt i)) -> (ref (rtt i))
     *   |        |              |                       |
     *   |        |              -> (ref null (rtt n i)) -> (ref (rtt n i))
     *   |        |-> i31ref
     *   |        |-> dataref -> (ref null $t) -> (ref $t), $t is struct/array
     *   |
     *   |-> funcref -> (ref null $t) -> (ref $t), $t is func
     *   |
     *   |-> externref
     */

    if (type1 == REF_TYPE_ANYREF) {
        /* any <: any */
        return type2 == REF_TYPE_ANYREF ? true : false;
    }
    else if (type1 == REF_TYPE_EQREF) {
        /* eq <: [eq, any] */
        return wasm_is_reftype_supers_of_eq(type2);
    }
    else if (type1 == REF_TYPE_FUNCREF) {
        /* func <: [func, any] */
        return wasm_is_reftype_supers_of_func(type2);
    }
    else if (type1 == REF_TYPE_EXTERNREF) {
        /* extern <: [extern, any] */
        return wasm_is_reftype_supers_of_extern(type2);
    }
    else if (type1 == REF_TYPE_I31REF) {
        /* i31 <: [i31, eq, any] */
        return wasm_is_reftype_supers_of_i31(type2);
    }
    else if (type1 == REF_TYPE_DATAREF) {
        /* data <: [data, eq, any] */
        return wasm_is_reftype_supers_of_data(type2);
    }
    else if (type1 == REF_TYPE_HT_NULLABLE) {
        if (wasm_is_refheaptype_rtt(&ref_type1->ref_ht_common)) {
            /* reftype1 is (ref null (rtt i)), the super type can be:
                 (ref null (rtt i), eqref and anyref */
            return wasm_is_reftype_supers_of_rtt_nullable(
                type2, ref_type2, ref_type1->ref_ht_rtt.type_idx);
        }
        else if (wasm_is_refheaptype_rttn(&ref_type1->ref_ht_common)) {
            /* reftype1 is (ref null (rtt n i)), the super type can be:
                 (ref null (rtt n i), (ref null (rtt i)), eqref and anyref */
            return wasm_is_reftype_supers_of_rttn_nullable(
                type2, ref_type2, ref_type1->ref_ht_rttn.n,
                ref_type1->ref_ht_rttn.type_idx);
        }
        else if (wasm_is_refheaptype_typeidx(&ref_type1->ref_ht_common)) {
            /* reftype1 is (ref null $t), the super type can be:
                 (ref null $t), dataref, eqref, funcref and anyref */
            if (type2 == REF_TYPE_HT_NULLABLE
                && wasm_is_refheaptype_typeidx(&ref_type2->ref_ht_common)
                && ref_type1->ref_ht_typeidx.type_idx
                       == ref_type2->ref_ht_typeidx.type_idx)
                return true;
            else if (types[ref_type1->ref_ht_typeidx.type_idx]->type_flag
                     == WASM_TYPE_FUNC)
                return wasm_is_reftype_supers_of_func(type2);
            else
                return wasm_is_reftype_supers_of_data(type2);
        }
        else {
            /* (ref null (func/extern/any/eq/i31/data)) have been converted into
               funcref/externref/anyref/eqref/i31ref/dataref when loading */
            bh_assert(0);
        }
    }
    else if (type1 == REF_TYPE_HT_NON_NULLABLE) {
        if (wasm_is_refheaptype_rtt(&ref_type1->ref_ht_common)) {
            /* reftype1 is (ref (rtt i)), the super type can be:
                 (ref (rtt i), (ref null (rtt i)), eqref and anyref */
            return wasm_is_reftype_supers_of_rtt_non_nullable(
                type2, ref_type2, ref_type1->ref_ht_rtt.type_idx);
        }
        else if (wasm_is_refheaptype_rttn(&ref_type1->ref_ht_common)) {
            /* reftype1 is (ref (rtt n i)), the super type can be:
                 (ref (rtt n i), (ref (rtt i), (ref null (rtt n i ),
                 (ref null (rtt i)), eqref and data ref */
            return wasm_is_reftype_supers_of_rttn_non_nullable(
                type2, ref_type2, ref_type1->ref_ht_rttn.n,
                ref_type1->ref_ht_rttn.type_idx);
        }
        else if (wasm_is_refheaptype_typeidx(&ref_type1->ref_ht_common)) {
            /* reftype1 is (ref $t), the super type can be:
                 (ref $t), (ref null $t), dataref, eqref, funcref and anyref */
            if ((type2 == REF_TYPE_HT_NULLABLE
                 || type2 == REF_TYPE_HT_NON_NULLABLE)
                && wasm_is_refheaptype_typeidx(&ref_type2->ref_ht_common)
                && ref_type1->ref_ht_typeidx.type_idx
                       == ref_type2->ref_ht_typeidx.type_idx)
                return true;
            else if (types[ref_type1->ref_ht_typeidx.type_idx]->type_flag
                     == WASM_TYPE_FUNC)
                return wasm_is_reftype_supers_of_func(type2);
            else
                return wasm_is_reftype_supers_of_data(type2);
        }
        else if (wasm_is_refheaptype_common(&ref_type1->ref_ht_common)) {
            /* reftype1 is (ref func/extern/any/eq/i31/data), the super type
               can be itself, or supers of
               funcref/externref/anyref/eqref/i31ref/dataref */
            if (wasm_reftype_equal(type1, ref_type1, type2, ref_type2))
                return true;
            else {
                int32 heap_type = ref_type1->ref_ht_common.heap_type;
                if (heap_type == HEAP_TYPE_ANY) {
                    /* (ref any) <: anyref */
                    return type2 == REF_TYPE_ANYREF ? true : false;
                }
                else if (heap_type == HEAP_TYPE_EQ) {
                    /* (ref eq) <: [eqref, anyref] */
                    return wasm_is_reftype_supers_of_eq(type2);
                }
                else if (heap_type == HEAP_TYPE_FUNC) {
                    /* (ref func) <: [funcref, anyref] */
                    return wasm_is_reftype_supers_of_func(type2);
                }
                else if (heap_type == HEAP_TYPE_EXTERN) {
                    /* (ref extern) <: [externref, anyref] */
                    return wasm_is_reftype_supers_of_extern(type2);
                }
                else if (heap_type == HEAP_TYPE_I31) {
                    /* (ref i31) <: [i31ref, eqref, anyref] */
                    return wasm_is_reftype_supers_of_i31(type2);
                }
                else if (heap_type == HEAP_TYPE_DATA) {
                    /* (ref data) <: [dataref, eqref, anyref] */
                    return wasm_is_reftype_supers_of_data(type2);
                }
                else {
                    bh_assert(0);
                }
            }
        }
        else {
            /* unknown type detected */
            LOG_ERROR("unknown sub type 0x%02x", type1);
            bh_assert(0);
        }
    }
    else {
        /* (rtt n i) and (rtt i) have been converted into
           (ref (rtt n i)) and (ref (rtt i)) in wasm/aot loader,
           we don't handle them again. */
        bh_assert(0);
    }

    return false;
}

static uint32
reftype_hash(const void *key)
{
    WASMRefType *reftype = (WASMRefType *)key;

    switch (reftype->ref_type) {
        case (uint8)REF_TYPE_HT_NULLABLE:
        case (uint8)REF_TYPE_HT_NON_NULLABLE:
        {
            RefHeapType_Common *ref_heap_type = (RefHeapType_Common *)reftype;

            if (wasm_is_refheaptype_common(ref_heap_type)
                /* type indexes of defined type are same */
                || wasm_is_refheaptype_typeidx(ref_heap_type)) {
                return (uint32)reftype->ref_type
                       ^ (uint32)ref_heap_type->heap_type;
            }
            else if (wasm_is_refheaptype_rttn(ref_heap_type)) {
                RefHeapType_RttN *heap_type_rttn = (RefHeapType_RttN *)reftype;
                return (uint32)reftype->ref_type
                       ^ (uint32)heap_type_rttn->rtt_type ^ heap_type_rttn->n
                       ^ heap_type_rttn->type_idx;
            }
            else if (wasm_is_refheaptype_rtt(ref_heap_type)) {
                RefHeapType_Rtt *heap_type_rtt = (RefHeapType_Rtt *)reftype;
                return (uint32)reftype->ref_type
                       ^ (uint32)heap_type_rtt->rtt_type
                       ^ heap_type_rtt->type_idx;
            }

            break;
        }

        /* (rtt n i) and (rtt i) have been converted into
           (ref (rtt n i)) and (ref (rtt i)) in wasm/aot loader,
           we don't handle them again. */
#if 0
        case (uint8)REF_TYPE_RTTN:
        {
            RefRttNType *ref_rttn_type = (RefRttNType *)reftype;
            return (uint32)ref_rttn_type->ref_type ^ ref_rttn_type->n
                   ^ ref_rttn_type->type_idx;
        }

        case (uint8)REF_TYPE_RTT:
        {
            RefRttType *ref_rtt_type = (RefRttType *)reftype;
            return (uint32)ref_rtt_type->ref_type ^ ref_rtt_type->type_idx;
        }
#endif

        default:
            break;
    }

    bh_assert(0);
    return 0;
}

static bool
reftype_equal(void *type1, void *type2)
{
    WASMRefType *reftype1 = (WASMRefType *)type1;
    WASMRefType *reftype2 = (WASMRefType *)type2;

    return wasm_reftype_equal(reftype1->ref_type, reftype1, reftype2->ref_type,
                              reftype2);
}

WASMRefType *
wasm_reftype_dup(const WASMRefType *ref_type)
{
    if (wasm_is_reftype_htref_nullable(ref_type->ref_type)
        || wasm_is_reftype_htref_non_nullable(ref_type->ref_type)) {
        if (wasm_is_refheaptype_rttn(&ref_type->ref_ht_common)) {
            RefHeapType_RttN *ht_rttn;
            if (!(ht_rttn = wasm_runtime_malloc(sizeof(RefHeapType_RttN))))
                return NULL;

            ht_rttn->ref_type = ref_type->ref_ht_rttn.ref_type;
            ht_rttn->nullable = ref_type->ref_ht_rttn.nullable;
            ht_rttn->rtt_type = ref_type->ref_ht_rttn.rtt_type;
            ht_rttn->n = ref_type->ref_ht_rttn.n;
            ht_rttn->type_idx = ref_type->ref_ht_rttn.type_idx;
            return (WASMRefType *)ht_rttn;
        }
        else if (wasm_is_refheaptype_rtt(&ref_type->ref_ht_common)) {
            RefHeapType_Rtt *ht_rtt;
            if (!(ht_rtt = wasm_runtime_malloc(sizeof(RefHeapType_Rtt))))
                return NULL;

            ht_rtt->ref_type = ref_type->ref_ht_rtt.ref_type;
            ht_rtt->nullable = ref_type->ref_ht_rtt.nullable;
            ht_rtt->rtt_type = ref_type->ref_ht_rtt.rtt_type;
            ht_rtt->type_idx = ref_type->ref_ht_rtt.type_idx;
            return (WASMRefType *)ht_rtt;
        }
        else if (wasm_is_refheaptype_common(&ref_type->ref_ht_common)
                 || wasm_is_refheaptype_typeidx(&ref_type->ref_ht_common)) {
            RefHeapType_Common *ht_common;
            if (!(ht_common = wasm_runtime_malloc(sizeof(RefHeapType_Common))))
                return NULL;

            ht_common->ref_type = ref_type->ref_ht_common.ref_type;
            ht_common->nullable = ref_type->ref_ht_common.nullable;
            ht_common->heap_type = ref_type->ref_ht_common.heap_type;
            return (WASMRefType *)ht_common;
        }
    }
    /* (rtt n i) and (rtt i) have been converted into
       (ref (rtt n i)) and (ref (rtt i)) in wasm/aot loader,
       we don't handle them again. */
#if 0
    else if (wasm_is_reftype_rttnref(ref_type->ref_type)) {
        RefRttNType *rttn;
        if (!(rttn = wasm_runtime_malloc(sizeof(RefRttNType))))
            return NULL;

        rttn->ref_type = ref_type->ref_rttn.ref_type;
        rttn->n = ref_type->ref_rttn.n;
        rttn->type_idx = ref_type->ref_rttn.type_idx;
        return (WASMRefType *)rttn;
    }
    else if (wasm_is_reftype_rttref(ref_type->ref_type)) {
        RefRttType *rtt;
        if (!(rtt = wasm_runtime_malloc(sizeof(RefRttType))))
            return NULL;

        rtt->ref_type = ref_type->ref_rtt.ref_type;
        rtt->type_idx = ref_type->ref_rtt.type_idx;
        return (WASMRefType *)rtt;
    }
#endif

    bh_assert(0);
    return NULL;
}

void
wasm_set_refheaptype_typeidx(RefHeapType_TypeIdx *ref_ht_typeidx, bool nullable,
                             int32 type_idx)
{
    ref_ht_typeidx->ref_type =
        nullable ? REF_TYPE_HT_NULLABLE : REF_TYPE_HT_NON_NULLABLE;
    ref_ht_typeidx->nullable = nullable;
    ref_ht_typeidx->type_idx = type_idx;
}

void
wasm_set_refheaptype_rttn(RefHeapType_RttN *ref_ht_rttn, bool nullable,
                          uint32 n, uint32 type_idx)
{
    ref_ht_rttn->ref_type =
        nullable ? REF_TYPE_HT_NULLABLE : REF_TYPE_HT_NON_NULLABLE;
    ref_ht_rttn->nullable = nullable;
    ref_ht_rttn->rtt_type = HEAP_TYPE_RTTN;
    ref_ht_rttn->n = n;
    ref_ht_rttn->type_idx = type_idx;
}

void
wasm_set_refheaptype_rtt(RefHeapType_Rtt *ref_ht_rtt, bool nullable,
                         uint32 type_idx)
{
    ref_ht_rtt->ref_type =
        nullable ? REF_TYPE_HT_NULLABLE : REF_TYPE_HT_NON_NULLABLE;
    ref_ht_rtt->nullable = nullable;
    ref_ht_rtt->rtt_type = HEAP_TYPE_RTT;
    ref_ht_rtt->type_idx = type_idx;
}

void
wasm_set_refheaptype_common(RefHeapType_Common *ref_ht_common, bool nullable,
                            int32 heap_type)
{
    ref_ht_common->ref_type =
        nullable ? REF_TYPE_HT_NULLABLE : REF_TYPE_HT_NON_NULLABLE;
    ref_ht_common->nullable = nullable;
    ref_ht_common->heap_type = heap_type;
}

WASMRefType *
wasm_reftype_map_find(WASMRefTypeMap *ref_type_maps, uint32 ref_type_map_count,
                      uint32 index_to_find)
{
    int low = 0, mid;
    int high = (int32)ref_type_map_count - 1;
    uint32 index;

    while (low <= high) {
        mid = (low + high) / 2;
        index = ref_type_maps[mid].index;
        if (index_to_find == index) {
            return ref_type_maps[mid].ref_type;
        }
        else if (index_to_find < index)
            high = mid - 1;
        else
            low = mid + 1;
    }

    return NULL;
}

HashMap *
wasm_reftype_set_create(uint32 size)
{
    HashMap *ref_type_set = bh_hash_map_create(
        32, false, reftype_hash, reftype_equal, NULL, wasm_runtime_free);

    return ref_type_set;
}

WASMRefType *
wasm_reftype_set_insert(HashMap *ref_type_set, const WASMRefType *ref_type)
{
    WASMRefType *ref_type_ret;

    if ((ref_type_ret = bh_hash_map_find(ref_type_set, (void *)ref_type)))
        return ref_type_ret;

    if (!(ref_type_ret = wasm_reftype_dup(ref_type)))
        return NULL;

    if (!bh_hash_map_insert(ref_type_set, ref_type_ret, ref_type_ret)) {
        wasm_runtime_free(ref_type_ret);
        return NULL;
    }

    return ref_type_ret;
}
