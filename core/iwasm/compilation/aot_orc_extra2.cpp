// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/SmallVectorMemoryBuffer.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

#include "bh_log.h"

class MyCompiler : public llvm::orc::IRCompileLayer::IRCompiler
{
  public:
    MyCompiler(llvm::orc::JITTargetMachineBuilder JTMB);
    llvm::Expected<llvm::orc::SimpleCompiler::CompileResult> operator()(
        llvm::Module &M) override;

  private:
    llvm::orc::JITTargetMachineBuilder JTMB;
};

MyCompiler::MyCompiler(llvm::orc::JITTargetMachineBuilder JTMB)
  : IRCompiler(llvm::orc::irManglingOptionsFromTargetOptions(JTMB.getOptions()))
  , JTMB(std::move(JTMB))
{}

class PrintStackSizes : public llvm::MachineFunctionPass
{
  public:
    PrintStackSizes();
    bool runOnMachineFunction(llvm::MachineFunction &MF) override;
    static char ID;
};

PrintStackSizes::PrintStackSizes()
  : MachineFunctionPass(ID)
{}

char PrintStackSizes::ID = 0;

bool
PrintStackSizes::runOnMachineFunction(llvm::MachineFunction &MF)
{
    auto name = MF.getName();
    auto MFI = &MF.getFrameInfo();
    size_t sz = MFI->getStackSize();
    LOG_VERBOSE("func %.*s stack %zu", name.size(), name.data(), sz);
    return false;
}

class MyPassManager : public llvm::legacy::PassManager
{
  public:
    void add(llvm::Pass *P) override;
};

void
MyPassManager::add(llvm::Pass *P)
{
    // a hack to avoid having a copy of the whole addPassesToEmitMC.
    // we want to add PrintStackSizes before FreeMachineFunctionPass.
    if (P->getPassName() == "Free MachineFunction") {
        return;
    }
    llvm::legacy::PassManager::add(P);
}

// a modified copy from llvm/lib/ExecutionEngine/Orc/CompileUtils.cpp
llvm::Expected<llvm::orc::SimpleCompiler::CompileResult>
MyCompiler::operator()(llvm::Module &M)
{
    auto TM = cantFail(JTMB.createTargetMachine());
    llvm::SmallVector<char, 0> ObjBufferSV;

    {
        llvm::raw_svector_ostream ObjStream(ObjBufferSV);

        MyPassManager PM;
        llvm::MCContext *Ctx;
        if (TM->addPassesToEmitMC(PM, Ctx, ObjStream))
            return llvm::make_error<llvm::StringError>(
                "Target does not support MC emission",
                llvm::inconvertibleErrorCode());
        PM.add(new PrintStackSizes());
        dynamic_cast<llvm::legacy::PassManager *>(&PM)->add(
            llvm::createFreeMachineFunctionPass());
        PM.run(M);
    }

    auto ObjBuffer = std::make_unique<llvm::SmallVectorMemoryBuffer>(
        std::move(ObjBufferSV),
        M.getModuleIdentifier() + "-jitted-objectbuffer",
        /*RequiresNullTerminator=*/false);

    return std::move(ObjBuffer);
}

llvm::Expected<std::unique_ptr<llvm::orc::IRCompileLayer::IRCompiler>>
compiler_creator(llvm::orc::JITTargetMachineBuilder JTMB)
{
    LOG_VERBOSE("%s called", __func__);
    return std::make_unique<MyCompiler>(MyCompiler(std::move(JTMB)));
}
