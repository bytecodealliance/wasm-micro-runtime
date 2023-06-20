/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <llvm-c/DebugInfo.h>
#include <llvm-c/Types.h>

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/MDBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>

#include "./dpgo_internal.h"

using namespace llvm;

/* return true if broken */
bool
verify_module_and_debug_info(LLVMModuleRef module)
{
    bool broken_debug_info = false;
    Module *M = reinterpret_cast<Module *>(module);
    bool broken_module = verifyModule(*M, &dbgs(), &broken_debug_info);

    if (broken_debug_info || broken_module)
        LOG_ERROR("broken_module=%d, broken_debug_info=%d", broken_module,
                  broken_debug_info);

    return broken_debug_info || broken_module;
}

/* append !{!"function_entry_count", i64 NN} */
void
wasm_dpgo_set_function_entry_count(LLVMValueRef function, uint32 count)
{
    Function *F = unwrap<Function>(function);
    if (F->hasProfileData()) {
        uint64 current_count = F->getEntryCount().getValue().getCount();
        if (current_count == count)
            return;
    }

    F->setEntryCount(count, Function::PCT_Real);
    F->addFnAttr(Attribute::Hot);
}

/* append !{!"branch_weights", i32 NN, i32 MM} */
void
wasm_dpgo_set_branch_weights(LLVMContextRef context, LLVMValueRef instruction,
                             uint32 *counts, uint32 counts_size)
{
    MDBuilder MDB(*reinterpret_cast<LLVMContext *>(context));
    SmallVector<unsigned, 4> Weights;

    for (unsigned i = 0; i < counts_size; i++) {
        Weights.push_back(counts[i]);
    }

    // FIXME: add more counters when meeting switch
    LOG_DEBUG("%s --> branch_weights {%u, %u}",
              LLVMPrintValueToString(instruction), counts[0], counts[1]);

    bh_assert(LLVMGetValueKind(instruction) == LLVMInstructionValueKind);
    Instruction *I = unwrap<Instruction>(instruction);
    I->setMetadata(LLVMContext::MD_prof, MDB.createBranchWeights(Weights));
}

void
wasm_dpgo_unlike_true_branch(LLVMContextRef context, LLVMValueRef cond_br)
{
    bh_assert(LLVMIsABranchInst(cond_br));
    bh_assert(LLVMIsConditional(cond_br));

    uint32 weights[2] = { 1, 1000 };
    return wasm_dpgo_set_branch_weights(context, cond_br, weights, 2);
}

void
wasm_dpgo_unlike_false_branch(LLVMContextRef context, LLVMValueRef cond_br)
{
    bh_assert(LLVMIsABranchInst(cond_br));
    bh_assert(LLVMIsConditional(cond_br));

    uint32 weights[2] = { 1000, 1 };
    return wasm_dpgo_set_branch_weights(context, cond_br, weights, 2);
}

/* append !{!"loop_header_weight", i64 NN} */
void
wasm_dpgo_set_irr_loop(LLVMContextRef context, LLVMValueRef instruction,
                       uint32 count)
{
    MDBuilder MDB(*reinterpret_cast<LLVMContext *>(context));

    bh_assert(LLVMGetValueKind(instruction) == LLVMInstructionValueKind);
    Instruction *I = unwrap<Instruction>(instruction);
    I->setMetadata(llvm::LLVMContext::MD_irr_loop,
                   MDB.createIrrLoopHeaderWeight(count));
}

void
wasm_dpgo_set_vp(LLVMModuleRef module, LLVMValueRef instruction, uint32 *counts,
                 uint32 counts_size)
{
    /*
    TODO:
    using
    void annotateValueSite(Module &M, Instruction &Inst,
                           const InstrProfRecord &InstrProfR,
                           InstrProfValueKind ValueKind, uint32 SiteIndx,
                           uint32 MaxMDCount = 3);

    Valuekind is 0(IPVK_IndirectCallTarget),
    MaxMDCount needs to be defined
    */
}

static bool
wasm_dpgo_get_offset_of_instr(WASMModule *wasm_module, LLVMValueRef instruction,
                              uint32 *line)
{
    /* always have some Debug Location */
    LLVMMetadataRef dbg_location = LLVMInstructionGetDebugLoc(instruction);
    bh_assert(dbg_location && "No Debug Location");
    bh_assert(LLVMGetMetadataKind(dbg_location) == LLVMDILocationMetadataKind
              && "Not a DILocation");
    if (!dbg_location)
        return false;

    *line = LLVMDILocationGetLine(dbg_location);
    LOG_DEBUG("  DILocation. LINE: %u", *line);
    return true;
}

static void
wasm_dpgo_visit_select(WASMModule *wasm_module, LLVMModuleRef module,
                       LLVMValueRef function, uint32 func_idx)
{
    for (LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(function);
         basic_block != NULL;
         basic_block = LLVMGetNextBasicBlock(basic_block)) {
        for (LLVMValueRef instruction = LLVMGetFirstInstruction(basic_block);
             instruction != NULL;
             instruction = LLVMGetNextInstruction(instruction)) {

            if (!LLVMIsASelectInst(instruction))
                continue;

            LOG_DEBUG("Select : %s  .", LLVMPrintValueToString(instruction));

            /* locate profiling counters */
            uint32 line;
            bool ret =
                wasm_dpgo_get_offset_of_instr(wasm_module, instruction, &line);
            if (!ret)
                return;

            uint32 weights[2] = { 0 };
            ret = wasm_dpgo_get_select_counts(wasm_module, func_idx, line,
                                              &weights[0], &weights[1]);
            if (!ret) {
                LOG_WARNING("Failed to get profiling counters for select %s",
                            LLVMPrintValueToString(instruction));
                return;
            }

            wasm_dpgo_set_branch_weights(LLVMGetModuleContext(module),
                                         instruction, weights, 2);
        }
    }
}

static void
wasm_dpgo_visit_terminator(WASMModule *wasm_module, LLVMModuleRef module,
                           LLVMValueRef function, uint32 func_idx)
{
    LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(function);
    while (basic_block) {
        LLVMValueRef terminator = LLVMGetBasicBlockTerminator(basic_block);
        basic_block = LLVMGetNextBasicBlock(basic_block);

        if (LLVMGetNumSuccessors(terminator) <= 1)
            continue;

        LOG_DEBUG("Terminator: %s  .", LLVMPrintValueToString(terminator));

        /* only take care condbr and switch */
        if (!LLVMIsABranchInst(terminator) && !LLVMIsASwitchInst(terminator))
            continue;

        /* bypass non-CondBr */
        if (LLVMIsABranchInst(terminator) && !LLVMIsConditional(terminator))
            continue;

        LLVMValueRef prof_md =
            LLVMGetMetadata(terminator, LLVMContext::MD_prof);
        if (prof_md) {
            LOG_DEBUG("  Already has Prof meta");
            continue;
        }

        /* locate profiling counters */
        uint32 line;
        bool ret =
            wasm_dpgo_get_offset_of_instr(wasm_module, terminator, &line);
        if (!ret)
            return;

        if (LLVMIsABranchInst(terminator)) {
            uint32 weights[2] = { 0 };
            ret = wasm_dpgo_get_cond_br_counts(wasm_module, func_idx, line,
                                               &weights[0], &weights[1]);
            if (!ret) {
                LOG_WARNING("Failed to get profiling counters for condbr %s",
                            LLVMPrintValueToString(terminator));
                return;
            }

            wasm_dpgo_set_branch_weights(LLVMGetModuleContext(module),
                                         terminator, weights, 2);
            continue;
        }

        // FIXME: implement switch
    }
}

void
wasm_dpgo_set_prof_meta(WASMModule *wasm_module, LLVMModuleRef module,
                        LLVMValueRef function, uint32 func_idx)
{
    bh_assert(LLVMGetValueKind(function) == LLVMFunctionValueKind);
    bh_assert(func_idx < wasm_module->import_function_count
                             + wasm_module->function_count);

    size_t size;
    LOG_DEBUG("  --> Add prof meta into %s",
              LLVMGetValueName2(function, &size));

    wasm_dpgo_visit_terminator(wasm_module, module, function, func_idx);
    wasm_dpgo_visit_select(wasm_module, module, function, func_idx);
}