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

#include "wamr_interp_frame.h"
#include "wamr.h"
#include "wamr_branch_block.h"
#include <wasm_opcode.h>
#include <memory>
#include <wasm_loader.h>
extern WAMRInstance *wamr;
void
WAMRInterpFrame::dump_impl(WASMInterpFrame *env)
{
    if (env->function) {
        wamr->set_func(env->function->u.func);

        if (env->ip)
            ip = env->ip
                 - env->function->u.func->code; // here we need to get the
                                                // offset from the code start.
#if WASM_ENABLE_FAST_INTERP == 0
        if (env->sp) {
            sp = reinterpret_cast<uint8 *>(env->sp)
                 - ((uint8 *)wamr->get_exec_env()
                        ->wasm_stack.bottom); // offset to the wasm_stack_top
        }
#endif
        if (env->function->u.func->field_name)
            function_name = env->function->u.func->field_name;
        else
            function_name = env->function->u.func_import->field_name;
    }
}
std::vector<std::unique_ptr<WAMRBranchBlock>>
wasm_replay_csp_bytecode(WASMExecEnv *exec_env, WASMInterpFrame *frame,
                         const uint8 *target_addr);
void
WAMRInterpFrame::restore_impl(WASMInterpFrame *env)
{
    auto module_inst = (WASMModuleInstance *)wamr->get_exec_env()->module_inst;
    if (0 < function_index
        && function_index < module_inst->e->function_count) {
        // LOGV(INFO) << fmt::format("function_index {} restored",
        // function_index);
        env->function = &module_inst->e->functions[function_index];
        if (env->function->is_import_func) {
            LOG_DEBUG("is_import_func");
            exit(-1);
        }
    }
    else {
        auto target_module = wamr->get_module_instance()->e;
        for (uint32 i = 0; i < target_module->function_count; i++) {
            auto cur_func = &target_module->functions[i];
            if (!cur_func->is_import_func) {
                if (!strcmp(cur_func->u.func->field_name,
                            function_name.c_str())) {
                    function_index = i;
                    env->function = cur_func;
                    break;
                }
            }
            else {
                if (!strcmp(cur_func->u.func_import->field_name,
                            function_name.c_str())) {
                    function_index = i;
                    env->function = cur_func;
                    break;
                }
            }
        }
    }
    wamr->set_func(env->function->u.func);
    auto cur_func = env->function;
    WASMFunction *cur_wasm_func = cur_func->u.func;

    LOG_DEBUG("ip_offset {} sp_offset {}, code start {}", ip, sp,
              (void *)wasm_get_func_code(env->function));
    env->ip = wasm_get_func_code(env->function) + ip;
    memcpy(env->lp, stack_frame.data(), stack_frame.size() * sizeof(uint32));
#if WASM_ENABLE_FAST_INTERP == 0
    env->sp_bottom =
        env->lp + cur_func->param_cell_num + cur_func->local_cell_num;
    env->sp = env->lp + sp;
    env->sp_boundary = env->sp_bottom + cur_wasm_func->max_stack_cell_num;

    // print_csps(csp);
    LOG_DEBUG("wasm_replay_csp_bytecode {} {} {}", (void *)wamr->get_exec_env(),
              (void *)env, (void *)env->ip);
    env->csp_bottom = (WASMBranchBlock *)env->sp_boundary;

    if (env->function->u.func && !env->function->is_import_func
        && env->sp_bottom) {
        auto csp = wasm_replay_csp_bytecode(wamr->get_exec_env(), env, env->ip);
        std::reverse(csp.begin(), csp.end());

        int i = 0;
        for (auto &&csp_item : csp) {
            restore(csp_item.get(), env->csp_bottom + i);
            LOG_DEBUG("csp_bottom {}",
                      ((uint8 *)env->csp_bottom + i)
                          - wamr->get_exec_env()->wasm_stack.bottom);
            i++;
        }

        env->csp = env->csp_bottom + csp.size();
        env->csp_boundary =
            env->csp_bottom + env->function->u.func->max_block_num;
    }
#endif
    LOG_DEBUG("func_idx %d ip %p sp %p stack bottom %p", function_index,
              (void *)env->ip, (void *)env->sp,
              (void *)wamr->get_exec_env()->wasm_stack.bottom);
}

#if WASM_ENABLE_AOT != 0
void
WAMRInterpFrame::dump_impl(AOTFrame *env)
{
    function_index = env->func_index;
    ip = env->ip_offset;
    sp = env->sp - env->lp; // offset to the wasm_stack_top

    LOG_DEBUG("function_index %d ip_offset %d lp %p sp %p sp_offset %lu",
              env->func_index, ip, (void *)env->lp, (void *)env->sp, sp);

    stack_frame = std::vector(env->lp, env->sp);

    for (auto x : stack_frame) {
        std::cout << x << " ";
    }
    std::cout << std::endl;
}
void
WAMRInterpFrame::restore_impl(AOTFrame *env)
{
    LOG_DEBUG("not impl");
    exit(-1);
}

static bool
check_buf(const uint8 *buf, const uint8 *buf_end, uint32 length,
          char *error_buf, uint32 error_buf_size)
{
    if ((uintptr_t)buf + length < (uintptr_t)buf
        || (uintptr_t)buf + length > (uintptr_t)buf_end) {
        return false;
    }
    return true;
}

#define CHECK_BUF(buf, buf_end, length)                                    \
    do {                                                                   \
        if (!check_buf(buf, buf_end, length, error_buf, error_buf_size)) { \
            goto fail;                                                     \
        }                                                                  \
    } while (0)

#define CHECK_BUF1(buf, buf_end, length)                                    \
    do {                                                                    \
        if (!check_buf1(buf, buf_end, length, error_buf, error_buf_size)) { \
            goto fail;                                                      \
        }                                                                   \
    } while (0)

#define TEMPLATE_READ_VALUE(Type, p) \
    ((p) += sizeof(Type), *(Type *)((p) - sizeof(Type)))

static bool
read_leb(uint8 **p_buf, const uint8 *buf_end, uint32 maxbits, bool sign,
         uint64 *p_result, char *error_buf, uint32 error_buf_size)
{
    const uint8 *buf = *p_buf;
    uint64 result = 0;
    uint32 shift = 0;
    uint32 offset = 0, bcnt = 0;
    uint64 byte;

    while (true) {
        /* uN or SN must not exceed ceil(N/7) bytes */
        if (bcnt + 1 > (maxbits + 6) / 7) {
            return false;
        }

        CHECK_BUF(buf, buf_end, offset + 1);
        byte = buf[offset];
        offset += 1;
        result |= ((byte & 0x7f) << shift);
        shift += 7;
        bcnt += 1;
        if ((byte & 0x80) == 0) {
            break;
        }
    }

    if (!sign && maxbits == 32 && shift >= maxbits) {
        /* The top bits set represent values > 32 bits */
        if (((uint8)byte) & 0xf0)
            goto fail_integer_too_large;
    }
    else if (sign && maxbits == 32) {
        if (shift < maxbits) {
            /* Sign extend, second highest bit is the sign bit */
            if ((uint8)byte & 0x40)
                result |= (~((uint64)0)) << shift;
        }
        else {
            /* The top bits should be a sign-extension of the sign bit */
            bool sign_bit_set = ((uint8)byte) & 0x8;
            int top_bits = ((uint8)byte) & 0xf0;
            if ((sign_bit_set && top_bits != 0x70)
                || (!sign_bit_set && top_bits != 0))
                goto fail_integer_too_large;
        }
    }
    else if (sign && maxbits == 64) {
        if (shift < maxbits) {
            /* Sign extend, second highest bit is the sign bit */
            if ((uint8)byte & 0x40)
                result |= (~((uint64)0)) << shift;
        }
        else {
            /* The top bits should be a sign-extension of the sign bit */
            bool sign_bit_set = ((uint8)byte) & 0x1;
            int top_bits = ((uint8)byte) & 0xfe;

            if ((sign_bit_set && top_bits != 0x7e)
                || (!sign_bit_set && top_bits != 0))
                goto fail_integer_too_large;
        }
    }

    *p_buf += offset;
    *p_result = result;
    return true;

fail_integer_too_large:
fail:
    return false;
}
#define read_uint8(p) TEMPLATE_READ_VALUE(uint8, p)
#define read_uint32(p) TEMPLATE_READ_VALUE(uint32, p)
#define read_bool(p) TEMPLATE_READ_VALUE(bool, p)

#define skip_leb(p) while (*p++ & 0x80)
#define skip_leb_int64(p, p_end) skip_leb(p)
#define skip_leb_uint32(p, p_end) skip_leb(p)
#define skip_leb_int32(p, p_end) skip_leb(p)
#define read_leb_int64(p, p_end, res)                                   \
    do {                                                                \
        uint64 res64;                                                   \
        if (!read_leb((uint8 **)&p, p_end, 64, true, &res64, error_buf, \
                      error_buf_size))                                  \
            goto fail;                                                  \
        (res) = (int64)res64;                                           \
    } while (0)

#define read_leb_uint32(p, p_end, res)                                   \
    do {                                                                 \
        uint64 res64;                                                    \
        if (!read_leb((uint8 **)&p, p_end, 32, false, &res64, error_buf, \
                      error_buf_size))                                   \
            goto fail;                                                   \
        (res) = (uint32)res64;                                           \
    } while (0)

#define read_leb_int32(p, p_end, res)                                   \
    do {                                                                \
        uint64 res64;                                                   \
        if (!read_leb((uint8 **)&p, p_end, 32, true, &res64, error_buf, \
                      error_buf_size))                                  \
            goto fail;                                                  \
        (res) = (int32)res64;                                           \
    } while (0)

std::vector<std::unique_ptr<WAMRBranchBlock>>
wasm_replay_csp_bytecode(WASMExecEnv *exec_env, WASMInterpFrame *frame,
                         const uint8 *target_addr)
{
    WASMFunctionInstance *cur_func = frame->function;
    const uint8 *frame_ip = wasm_get_func_code(cur_func),
                *frame_ip_end = wasm_get_func_code_end(cur_func);

    if (!(frame_ip <= target_addr && target_addr <= frame_ip_end)) {
        LOG_DEBUG("target_addr invalid");
        exit(-1);
    }

    char error_buf[128];
    uint32 count;
    uint32 error_buf_size = sizeof(error_buf);
    uint8 opcode, u8;

    uint32 param_cell_num, cell_num = 0;
    uint32 local_idx, local_offset;
    uint8 local_type;
    std::vector<std::unique_ptr<WAMRBranchBlock>> csp;
    uint32 *frame_lp = frame->lp;
    uint32 *frame_sp =
        frame_lp + cur_func->param_cell_num + cur_func->local_cell_num;
    uint8 value_type;
    uint8 *else_addr, *end_addr;

#define PUSH_CSP(_label_type, param_cell_num, cell_num, _target_addr)        \
    {                                                                        \
        auto e = std::make_unique<WAMRBranchBlock>();                        \
        e->cell_num = cell_num;                                              \
        e->begin_addr = frame_ip - cur_func->u.func->code;                   \
        e->target_addr = (_target_addr)-cur_func->u.func->code;              \
        e->frame_sp = reinterpret_cast<uint8 *>(frame_sp - (param_cell_num)) \
                      - exec_env->wasm_stack.bottom;                         \
        csp.emplace_back(std::move(e));                                      \
    }

#define POP_CSP_CHECK_OVERFLOW(n)  \
    do {                           \
        assert(csp.size() >= (n)); \
    } while (0)

#define POP_CSP()                  \
    do {                           \
        POP_CSP_CHECK_OVERFLOW(1); \
        csp.pop_back();            \
    } while (0)

#define POP_CSP_N(n)                                                        \
    do {                                                                    \
        uint32 *frame_sp_old = frame_sp;                                    \
        uint32 cell_num_to_copy;                                            \
        POP_CSP_CHECK_OVERFLOW((n) + 1);                                    \
        csp.resize(csp.size() - (n));                                       \
        frame_ip = cur_func->u.func->code + csp.back()->target_addr;        \
        /* copy arity values of block */                                    \
        frame_sp = reinterpret_cast<uint32 *>(exec_env->wasm_stack.s.bottom \
                                              + csp.back()->frame_sp);      \
        cell_num_to_copy = csp.back()->cell_num;                            \
        frame_sp += cell_num_to_copy;                                       \
    } while (0)

#define GET_LOCAL_INDEX_TYPE_AND_OFFSET()                                \
    do {                                                                 \
        uint32 param_count = cur_func->param_count;                      \
        read_leb_uint32(frame_ip, frame_ip_end, local_idx);              \
        local_offset = cur_func->local_offsets[local_idx];               \
        if (local_idx < param_count)                                     \
            local_type = cur_func->param_types[local_idx];               \
        else                                                             \
            local_type = cur_func->local_types[local_idx - param_count]; \
    } while (0)

#define PUSH_I32() frame_sp++;
#define POP_I32() frame_sp--;
#define PUSH_I64() frame_sp += 2;
#define POP_I64() frame_sp -= 2;

    WASMFunction *cur_wasm_func = cur_func->u.func;
    WASMType *func_type;
    func_type = cur_wasm_func->func_type;
    cell_num = func_type->ret_cell_num;
    PUSH_CSP(LABEL_TYPE_FUNCTION, 0, cell_num, frame_ip_end - 1);

    while (frame_ip < target_addr) {
        opcode = *frame_ip++;
#if WASM_ENABLE_DEBUG_INTERP != 0
    op_break_retry:
#endif
        switch (opcode) {
            case WASM_OP_UNREACHABLE:
            case WASM_OP_NOP:
                break;

            case WASM_OP_BLOCK:
                value_type = *frame_ip++;
                param_cell_num = 0;
                cell_num = wasm_value_type_cell_num(value_type);
                if (!wasm_loader_find_block_addr(
                        exec_env, (BlockAddr *)exec_env->block_addr_cache,
                        frame_ip, (uint8 *)-1, LABEL_TYPE_BLOCK, &else_addr,
                        &end_addr)) {
                    LOG_DEBUG("wasm_loader_find_block_addr");
                    exit(-1);
                }
                PUSH_CSP(LABEL_TYPE_BLOCK, param_cell_num, cell_num, end_addr);
                break;
            case WASM_OP_LOOP:
                value_type = *frame_ip++;
                param_cell_num = 0;
                cell_num = 0;
                PUSH_CSP(LABEL_TYPE_LOOP, param_cell_num, cell_num, frame_ip);
                break;
            case WASM_OP_IF:
                /* block result type: 0x40/0x7F/0x7E/0x7D/0x7C */
                value_type = *frame_ip++;
                param_cell_num = 0;
                cell_num = wasm_value_type_cell_num(value_type);
                if (!wasm_loader_find_block_addr(
                        exec_env, (BlockAddr *)exec_env->block_addr_cache,
                        frame_ip, (uint8 *)-1, LABEL_TYPE_IF, &else_addr,
                        &end_addr)) {
                    LOG_DEBUG("FAULT");
                    exit(-1);
                }

                if (target_addr <= else_addr) {
                    // cond is true
                    PUSH_CSP(LABEL_TYPE_IF, param_cell_num, cell_num, end_addr);
                }
                else {
                    if (else_addr == nullptr) {
                        frame_ip = end_addr + 1;
                    }
                    /* if there is an else branch, go to the else addr */
                    else {
                        PUSH_CSP(LABEL_TYPE_IF, param_cell_num, cell_num,
                                 end_addr);
                        frame_ip = else_addr + 1;
                    }
                }
                break;

            case EXT_OP_BLOCK:
            case EXT_OP_LOOP:
            case EXT_OP_IF:
                POP_I32();
                LOG_DEBUG("FAULT");
                exit(-1);
                break;

            case WASM_OP_ELSE:
                frame_ip = cur_func->u.func->code + csp.back()->target_addr;
                break;

            case WASM_OP_END:
                if (csp.size() > 1) {
                    POP_CSP();
                }
                else {
                    LOG_DEBUG("FAULT");
                    exit(-1);
                }
                break;

            case WASM_OP_BR:
                skip_leb_uint32(frame_ip, p_end); /* labelidx */
                break;

            case WASM_OP_BR_IF:
                skip_leb_uint32(frame_ip, p_end); /* labelidx */
                POP_I32();
                break;

            case WASM_OP_BR_TABLE:
                read_leb_uint32(frame_ip, frame_ip_end, count); /* lable num */
                POP_I32();
                break;

            case EXT_OP_BR_TABLE_CACHE:
                LOG_DEBUG("FAULT");
                exit(-1);
                break;

            case WASM_OP_RETURN:
                LOG_DEBUG("FAULT");
                exit(-1);
                break;

            case WASM_OP_CALL:
#if WASM_ENABLE_TAIL_CALL != 0
            case WASM_OP_RETURN_CALL:
#endif
                skip_leb_uint32(frame_ip, p_end); /* funcidx */
                break;

            case WASM_OP_CALL_INDIRECT:
#if WASM_ENABLE_TAIL_CALL != 0
            case WASM_OP_RETURN_CALL_INDIRECT:
#endif
                skip_leb_uint32(frame_ip, p_end); /* typeidx */
                CHECK_BUF(frame_ip, frame_ip_end, 1);
                u8 = read_uint8(frame_ip); /* 0x00 */
                POP_I32();
                break;

            case WASM_OP_DROP:
                frame_sp--;
                break;
            case WASM_OP_SELECT:
                POP_I32();
                frame_sp--;
                break;
            case WASM_OP_DROP_64:
                frame_sp -= 2;
                break;
            case WASM_OP_SELECT_64:
                POP_I32();
                frame_sp -= 2;
                break;

#if WASM_ENABLE_REF_TYPES != 0
            case WASM_OP_SELECT_T:
            case WASM_OP_TABLE_GET:
            case WASM_OP_TABLE_SET:
            case WASM_OP_REF_NULL:
            case WASM_OP_REF_IS_NULL:
            case WASM_OP_REF_FUNC:
                LOG_DEBUG("FAULT");
                exit(-1);
                break;
#endif /* WASM_ENABLE_REF_TYPES */
            case WASM_OP_GET_LOCAL:
                GET_LOCAL_INDEX_TYPE_AND_OFFSET();

                switch (local_type) {
                    case VALUE_TYPE_I32:
                    case VALUE_TYPE_F32:
                        PUSH_I32();
                        break;
                    case VALUE_TYPE_I64:
                    case VALUE_TYPE_F64:
                        PUSH_I64();
                        break;
                    default:
                        LOG_DEBUG("FAULT");
                        exit(-1);
                }
                break;
            case WASM_OP_SET_LOCAL:
                GET_LOCAL_INDEX_TYPE_AND_OFFSET();

                switch (local_type) {
                    case VALUE_TYPE_I32:
                    case VALUE_TYPE_F32:
#if WASM_ENABLE_REF_TYPES != 0
                    case VALUE_TYPE_FUNCREF:
                    case VALUE_TYPE_EXTERNREF:
#endif
                        POP_I32();
                        break;
                    case VALUE_TYPE_I64:
                    case VALUE_TYPE_F64:
                        POP_I64()
                        break;
                    default:
                        LOG_DEBUG("FAULT");
                        exit(-1);
                }
                break;
            case WASM_OP_TEE_LOCAL:
                skip_leb_uint32(frame_ip, frame_ip_end);
                break;
            case WASM_OP_GET_GLOBAL:
                skip_leb_uint32(frame_ip, frame_ip_end);
                PUSH_I32();
                break;
            case WASM_OP_SET_GLOBAL:
                skip_leb_uint32(frame_ip, frame_ip_end);
                POP_I32();
                break;
            case WASM_OP_GET_GLOBAL_64:
                skip_leb_uint32(frame_ip, frame_ip_end);
                PUSH_I64();
                break;
            case WASM_OP_SET_GLOBAL_64:
                skip_leb_uint32(frame_ip, frame_ip_end);
                POP_I64();
                break;
            case WASM_OP_SET_GLOBAL_AUX_STACK:
                skip_leb_uint32(frame_ip, p_end); /* local index */
                frame_sp--;
                break;

            case EXT_OP_GET_LOCAL_FAST:
                local_offset = *frame_ip++;
                if (local_offset & 0x80) {
                    PUSH_I64();
                }
                else {
                    PUSH_I32();
                }
                break;
            case EXT_OP_SET_LOCAL_FAST:
                local_offset = *frame_ip++;
                if (local_offset & 0x80) {
                    POP_I64();
                }
                else {
                    POP_I32();
                }
                break;
            case EXT_OP_TEE_LOCAL_FAST:
                local_offset = *frame_ip++;
                break;

            case WASM_OP_I32_LOAD:
            case WASM_OP_F32_LOAD:
            case WASM_OP_I32_LOAD8_S:
            case WASM_OP_I32_LOAD8_U:
            case WASM_OP_I32_LOAD16_S:
            case WASM_OP_I32_LOAD16_U:
                POP_I32();
                PUSH_I32();
                skip_leb_uint32(frame_ip, p_end); /* align */
                skip_leb_uint32(frame_ip, p_end); /* offset */
                break;
            case WASM_OP_I64_LOAD:
            case WASM_OP_F64_LOAD:
            case WASM_OP_I64_LOAD8_S:
            case WASM_OP_I64_LOAD8_U:
            case WASM_OP_I64_LOAD16_S:
            case WASM_OP_I64_LOAD16_U:
            case WASM_OP_I64_LOAD32_S:
            case WASM_OP_I64_LOAD32_U:
                POP_I32();
                PUSH_I64();
                skip_leb_uint32(frame_ip, p_end); /* align */
                skip_leb_uint32(frame_ip, p_end); /* offset */
                break;
            case WASM_OP_I32_STORE:
            case WASM_OP_F32_STORE:
            case WASM_OP_I32_STORE8:
            case WASM_OP_I32_STORE16:
                POP_I32();
                POP_I32();
                skip_leb_uint32(frame_ip, p_end); /* align */
                skip_leb_uint32(frame_ip, p_end); /* offset */
                break;
            case WASM_OP_I64_STORE:
            case WASM_OP_F64_STORE:
            case WASM_OP_I64_STORE8:
            case WASM_OP_I64_STORE16:
            case WASM_OP_I64_STORE32:
                POP_I32();
                frame_sp -= 2;
                skip_leb_uint32(frame_ip, p_end); /* align */
                skip_leb_uint32(frame_ip, p_end); /* offset */
                break;

            case WASM_OP_MEMORY_SIZE:
            case WASM_OP_MEMORY_GROW:
                skip_leb_uint32(frame_ip, p_end); /* 0x00 */
                PUSH_I32();
                break;

            case WASM_OP_I32_CONST:
                skip_leb_int32(frame_ip, p_end);
                PUSH_I32();
                break;
            case WASM_OP_I64_CONST:
                PUSH_I32();
                PUSH_I32();
                skip_leb_int64(frame_ip, p_end);
                break;
            case WASM_OP_F32_CONST:
                frame_ip += sizeof(float32);
                PUSH_I32();
                break;
            case WASM_OP_F64_CONST:
                PUSH_I32();
                PUSH_I32();
                frame_ip += sizeof(float64);
                break;

            case WASM_OP_I32_EQZ:
                POP_I32();
                PUSH_I32();
                break;
            case WASM_OP_I32_EQ:
            case WASM_OP_I32_NE:
            case WASM_OP_I32_LT_S:
            case WASM_OP_I32_LT_U:
            case WASM_OP_I32_GT_S:
            case WASM_OP_I32_GT_U:
            case WASM_OP_I32_LE_S:
            case WASM_OP_I32_LE_U:
            case WASM_OP_I32_GE_S:
            case WASM_OP_I32_GE_U:
            case WASM_OP_F32_EQ:
            case WASM_OP_F32_NE:
            case WASM_OP_F32_LT:
            case WASM_OP_F32_GT:
            case WASM_OP_F32_LE:
            case WASM_OP_F32_GE:
                POP_I32();
                POP_I32();
                PUSH_I32();
                break;
            case WASM_OP_I64_EQZ:
                POP_I64();
                PUSH_I32();
                break;
            case WASM_OP_I64_EQ:
            case WASM_OP_I64_NE:
            case WASM_OP_I64_LT_S:
            case WASM_OP_I64_LT_U:
            case WASM_OP_I64_GT_S:
            case WASM_OP_I64_GT_U:
            case WASM_OP_I64_LE_S:
            case WASM_OP_I64_LE_U:
            case WASM_OP_I64_GE_S:
            case WASM_OP_I64_GE_U:
            case WASM_OP_F64_EQ:
            case WASM_OP_F64_NE:
            case WASM_OP_F64_LT:
            case WASM_OP_F64_GT:
            case WASM_OP_F64_LE:
            case WASM_OP_F64_GE:
                POP_I64();
                POP_I64();
                PUSH_I32();
                break;

            case WASM_OP_I32_CLZ:
            case WASM_OP_I32_CTZ:
            case WASM_OP_I32_POPCNT:
                break;
            case WASM_OP_I32_ADD:
            case WASM_OP_I32_SUB:
            case WASM_OP_I32_MUL:
            case WASM_OP_I32_DIV_S:
            case WASM_OP_I32_DIV_U:
            case WASM_OP_I32_REM_S:
            case WASM_OP_I32_REM_U:
            case WASM_OP_I32_AND:
            case WASM_OP_I32_OR:
            case WASM_OP_I32_XOR:
            case WASM_OP_I32_SHL:
            case WASM_OP_I32_SHR_S:
            case WASM_OP_I32_SHR_U:
            case WASM_OP_I32_ROTL:
            case WASM_OP_I32_ROTR:
                POP_I32();
                break;
            case WASM_OP_I64_CLZ:
            case WASM_OP_I64_CTZ:
            case WASM_OP_I64_POPCNT:
                break;
            case WASM_OP_I64_ADD:
            case WASM_OP_I64_SUB:
            case WASM_OP_I64_MUL:
            case WASM_OP_I64_DIV_S:
            case WASM_OP_I64_DIV_U:
            case WASM_OP_I64_REM_S:
            case WASM_OP_I64_REM_U:
            case WASM_OP_I64_AND:
            case WASM_OP_I64_OR:
            case WASM_OP_I64_XOR:
            case WASM_OP_I64_SHL:
            case WASM_OP_I64_SHR_S:
            case WASM_OP_I64_SHR_U:
            case WASM_OP_I64_ROTL:
            case WASM_OP_I64_ROTR:
                POP_I64();
                break;
            case WASM_OP_F32_ABS:
            case WASM_OP_F32_NEG:
            case WASM_OP_F32_CEIL:
            case WASM_OP_F32_FLOOR:
            case WASM_OP_F32_TRUNC:
            case WASM_OP_F32_NEAREST:
            case WASM_OP_F32_SQRT:
                break;
            case WASM_OP_F32_ADD:
            case WASM_OP_F32_SUB:
            case WASM_OP_F32_MUL:
            case WASM_OP_F32_DIV:
            case WASM_OP_F32_MIN:
            case WASM_OP_F32_MAX:
            case WASM_OP_F32_COPYSIGN:
                POP_I32();
                break;
            case WASM_OP_F64_ABS:
            case WASM_OP_F64_NEG:
            case WASM_OP_F64_CEIL:
            case WASM_OP_F64_FLOOR:
            case WASM_OP_F64_TRUNC:
            case WASM_OP_F64_NEAREST:
            case WASM_OP_F64_SQRT:
                break;
            case WASM_OP_F64_ADD:
            case WASM_OP_F64_SUB:
            case WASM_OP_F64_MUL:
            case WASM_OP_F64_DIV:
            case WASM_OP_F64_MIN:
            case WASM_OP_F64_MAX:
            case WASM_OP_F64_COPYSIGN:
                POP_I64();
                break;
            case WASM_OP_I32_WRAP_I64:
                POP_I64();
                PUSH_I32();
                break;
            case WASM_OP_I32_TRUNC_S_F32:
            case WASM_OP_I32_TRUNC_U_F32:
                break;
            case WASM_OP_I32_TRUNC_S_F64:
            case WASM_OP_I32_TRUNC_U_F64:
                POP_I64();
                PUSH_I32();
                break;
            case WASM_OP_I64_EXTEND_S_I32:
            case WASM_OP_I64_EXTEND_U_I32:
            case WASM_OP_I64_TRUNC_S_F32:
            case WASM_OP_I64_TRUNC_U_F32:
                POP_I32();
                PUSH_I64();
                break;
            case WASM_OP_I64_TRUNC_S_F64:
            case WASM_OP_I64_TRUNC_U_F64:
            case WASM_OP_F32_CONVERT_S_I32:
            case WASM_OP_F32_CONVERT_U_I32:
                break;
            case WASM_OP_F32_CONVERT_S_I64:
            case WASM_OP_F32_CONVERT_U_I64:
            case WASM_OP_F32_DEMOTE_F64:
                POP_I64();
                PUSH_I32();
                break;
            case WASM_OP_F64_CONVERT_S_I32:
            case WASM_OP_F64_CONVERT_U_I32:
                POP_I32();
                PUSH_I64();
                break;
            case WASM_OP_F64_CONVERT_S_I64:
            case WASM_OP_F64_CONVERT_U_I64:
                break;
            case WASM_OP_F64_PROMOTE_F32:
                POP_I32();
                PUSH_I64();
                break;
            case WASM_OP_I32_REINTERPRET_F32:
            case WASM_OP_I64_REINTERPRET_F64:
            case WASM_OP_F32_REINTERPRET_I32:
            case WASM_OP_F64_REINTERPRET_I64:
                break;
            case WASM_OP_I32_EXTEND8_S:
            case WASM_OP_I32_EXTEND16_S:
            case WASM_OP_I64_EXTEND8_S:
            case WASM_OP_I64_EXTEND16_S:
            case WASM_OP_I64_EXTEND32_S:
                // TODO
                LOG_DEBUG("FAULT");
                exit(-1);
                break;
            case WASM_OP_MISC_PREFIX:
            {
                uint32 opcode1;

                read_leb_uint32(frame_ip, frame_ip_end, opcode1);

                switch (opcode1) {
                    case WASM_OP_I32_TRUNC_SAT_S_F32:
                    case WASM_OP_I32_TRUNC_SAT_U_F32:
                    case WASM_OP_I32_TRUNC_SAT_S_F64:
                    case WASM_OP_I32_TRUNC_SAT_U_F64:
                    case WASM_OP_I64_TRUNC_SAT_S_F32:
                    case WASM_OP_I64_TRUNC_SAT_U_F32:
                    case WASM_OP_I64_TRUNC_SAT_S_F64:
                    case WASM_OP_I64_TRUNC_SAT_U_F64:
                        break;
#if WASM_ENABLE_BULK_MEMORY != 0
                    case WASM_OP_MEMORY_INIT:
                        skip_leb_uint32(frame_ip, p_end);
                        /* skip memory idx */
                        frame_ip++;
                        break;
                    case WASM_OP_DATA_DROP:
                        skip_leb_uint32(frame_ip, p_end);
                        break;
                    case WASM_OP_MEMORY_COPY:
                        /* skip two memory idx */
                        frame_ip += 2;
                        break;
                    case WASM_OP_MEMORY_FILL:
                        /* skip memory idx */
                        frame_ip++;
                        break;
#endif /* WASM_ENABLE_BULK_MEMORY */
#if WASM_ENABLE_REF_TYPES != 0
                    case WASM_OP_TABLE_INIT:
                    case WASM_OP_TABLE_COPY:
                        /* tableidx */
                        skip_leb_uint32(p, p_end);
                        /* elemidx */
                        skip_leb_uint32(p, p_end);
                        break;
                    case WASM_OP_ELEM_DROP:
                        /* elemidx */
                        skip_leb_uint32(p, p_end);
                        break;
                    case WASM_OP_TABLE_SIZE:
                    case WASM_OP_TABLE_GROW:
                    case WASM_OP_TABLE_FILL:
                        skip_leb_uint32(p, p_end); /* table idx */
                        break;
#endif /* WASM_ENABLE_REF_TYPES */
                    default:
                        LOG_DEBUG("FAULT");
                        exit(-1);
                }
                break;
            }

#if WASM_ENABLE_SIMD != 0
#if (WASM_ENABLE_WAMR_COMPILER != 0) || (WASM_ENABLE_JIT != 0)
            case WASM_OP_SIMD_PREFIX:
            {
                LOG_DEBUG("FAULT");
                exit(-1);
                /* TODO: shall we ceate a table to be friendly to branch
                 * prediction */
                opcode = read_uint8(p);
                /* follow the order of enum WASMSimdEXTOpcode in
                 * wasm_opcode.h
                 */
                switch (opcode) {
                    case SIMD_v128_load:
                    case SIMD_v128_load8x8_s:
                    case SIMD_v128_load8x8_u:
                    case SIMD_v128_load16x4_s:
                    case SIMD_v128_load16x4_u:
                    case SIMD_v128_load32x2_s:
                    case SIMD_v128_load32x2_u:
                    case SIMD_v128_load8_splat:
                    case SIMD_v128_load16_splat:
                    case SIMD_v128_load32_splat:
                    case SIMD_v128_load64_splat:
                    case SIMD_v128_store:
                        /* memarg align */
                        skip_leb_uint32(p, p_end);
                        /* memarg offset*/
                        skip_leb_uint32(p, p_end);
                        break;

                    case SIMD_v128_const:
                    case SIMD_v8x16_shuffle:
                        /* immByte[16] immLaneId[16] */
                        CHECK_BUF1(p, p_end, 16);
                        p += 16;
                        break;

                    case SIMD_i8x16_extract_lane_s:
                    case SIMD_i8x16_extract_lane_u:
                    case SIMD_i8x16_replace_lane:
                    case SIMD_i16x8_extract_lane_s:
                    case SIMD_i16x8_extract_lane_u:
                    case SIMD_i16x8_replace_lane:
                    case SIMD_i32x4_extract_lane:
                    case SIMD_i32x4_replace_lane:
                    case SIMD_i64x2_extract_lane:
                    case SIMD_i64x2_replace_lane:
                    case SIMD_f32x4_extract_lane:
                    case SIMD_f32x4_replace_lane:
                    case SIMD_f64x2_extract_lane:
                    case SIMD_f64x2_replace_lane:
                        /* ImmLaneId */
                        CHECK_BUF(p, p_end, 1);
                        p++;
                        break;

                    case SIMD_v128_load8_lane:
                    case SIMD_v128_load16_lane:
                    case SIMD_v128_load32_lane:
                    case SIMD_v128_load64_lane:
                    case SIMD_v128_store8_lane:
                    case SIMD_v128_store16_lane:
                    case SIMD_v128_store32_lane:
                    case SIMD_v128_store64_lane:
                        /* memarg align */
                        skip_leb_uint32(p, p_end);
                        /* memarg offset*/
                        skip_leb_uint32(p, p_end);
                        /* ImmLaneId */
                        CHECK_BUF(p, p_end, 1);
                        p++;
                        break;

                    case SIMD_v128_load32_zero:
                    case SIMD_v128_load64_zero:
                        /* memarg align */
                        skip_leb_uint32(p, p_end);
                        /* memarg offset*/
                        skip_leb_uint32(p, p_end);
                        break;

                    default:
                        /*
                         * since latest SIMD specific used almost every
                         * value from 0x00 to 0xff, the default branch will
                         * present all opcodes without imm
                         * https://github.com/WebAssembly/simd/blob/main/proposals/simd/NewOpcodes.md
                         */
                        break;
                }
                break;
            }
#endif /* end of (WASM_ENABLE_WAMR_COMPILER != 0) || (WASM_ENABLE_JIT != 0) */
#endif /* end of WASM_ENABLE_SIMD */

#if WASM_ENABLE_SHARED_MEMORY != 0
            case WASM_OP_ATOMIC_PREFIX:
            {
                LOG_DEBUG("FAULT");
                exit(-1);
                /* atomic_op (1 u8) + memarg (2 u32_leb) */
                opcode = read_uint8(frame_ip);
                if (opcode != WASM_OP_ATOMIC_FENCE) {
                    skip_leb_uint32(frame_ip, p_end); /* align */
                    skip_leb_uint32(frame_ip, p_end); /* offset */
                }
                else {
                    /* atomic.fence doesn't have memarg */
                    frame_ip++;
                }
                break;
            }
#if WASM_ENABLE_DEBUG_INTERP != 0
            case DEBUG_OP_BREAK:
            {
                WASMDebugInstance *debug_instance =
                    wasm_exec_env_get_instance(exec_env);
                char orignal_opcode[1];
                uint64 size = 1;
                WASMModuleInstance *module_inst =
                    (WASMModuleInstance *)exec_env->module_inst;
                uint64 offset = (p - 1) >= module_inst->module->load_addr
                                    ? (p - 1) - module_inst->module->load_addr
                                    : ~0;
                if (debug_instance) {
                    if (wasm_debug_instance_get_obj_mem(debug_instance, offset,
                                                        orignal_opcode, &size)
                        && size == 1) {
                        LOG_VERBOSE("WASM loader find OP_BREAK , recover it "
                                    "with  %02x: ",
                                    orignal_opcode[0]);
                        opcode = orignal_opcode[0];
                        goto op_break_retry;
                    }
                }
                break;
            }
#endif

            default:
                LOG_DEBUG("FAULT");
                exit(-1);
        }
#endif
    }

    (void)u8;
    (void)exec_env;
    std::reverse(csp.begin(), csp.end());
    return csp;
fail:
    LOG_DEBUG("FAULT");
    exit(-1);
}
#endif

#if !__has_include(<expected>) || __cplusplus <= 202002L
void
dump(WAMRInterpFrame *t, WASMInterpFrame *env)
{
    t->dump_impl(env);
};
void
restore(WAMRInterpFrame *t, WASMInterpFrame *env)
{
    t->restore_impl(env);
};
void
dump(WAMRInterpFrame *t, AOTFrame *env)
{
    t->dump_impl(env);
};
void
restore(WAMRInterpFrame *t, AOTFrame *env)
{
    t->restore_impl(env);
};
#endif