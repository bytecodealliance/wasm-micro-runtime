/*
 * Copyright (C) 2023 Liquid Reply GmbH. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

use std::{
    env,
    path::{Path, PathBuf},
};

#[derive(Debug)]
struct ParseLibPathError {
    #[allow(dead_code)]
    msg: String,
}

fn get_vmlib_path() -> Result<PathBuf, ParseLibPathError> {
    let lib_path = env::var("WAMR_VMLIB_PATH").map_err(|e| ParseLibPathError {
        msg: format!("'WAMR_VMLIB_PATH'. {}", e.to_string()),
    })?;

    Path::new(&(lib_path))
        .canonicalize()
        .map_err(|e| ParseLibPathError {
            msg: format!("'{}'. {}", lib_path, e.to_string()),
        })
        .map(|lib_path| lib_path)
}

fn get_llvm_lib_path() -> Result<PathBuf, ParseLibPathError> {
    let lib_path = env::var("WAMR_LLVMLIB_PATH").map_err(|e| ParseLibPathError {
        msg: format!("'WAMR_LLVMLIB_PATH'. {}", e.to_string()),
    })?;

    Path::new(&(lib_path))
        .canonicalize()
        .map_err(|e| ParseLibPathError {
            msg: format!("'{}'. {}", lib_path, e.to_string()),
        })
        .map(|lib_path| lib_path)
}

fn main() -> Result<(), ParseLibPathError> {
    let vmlib_path = get_vmlib_path()?;

    println!("cargo:rustc-link-search=native={}", vmlib_path.display());
    println!("cargo:rustc-link-lib=static=vmlib");

    if cfg!(feature = "llvmjit") {
        println!("cargo:rustc-link-lib=dylib=dl");
        println!("cargo:rustc-link-lib=dylib=m");
        println!("cargo:rustc-link-lib=dylib=rt");
        println!("cargo:rustc-link-lib=dylib=stdc++");
        println!("cargo:rustc-link-lib=dylib=z");

        let llvm_lib_path = get_llvm_lib_path()?;
        println!("cargo:libdir={}", llvm_lib_path.display());
        println!("cargo:rustc-link-search=native={}", llvm_lib_path.display());

        for llvm_lib in &[
            "LLVMAggressiveInstCombine",
            "LLVMAnalysis",
            "LLVMAsmParser",
            "LLVMAsmPrinter",
            "LLVMBitReader",
            "LLVMBitWriter",
            "LLVMCFGuard",
            "LLVMCodeGen",
            "LLVMCoroutines",
            "LLVMCoverage",
            "LLVMDWARFLinker",
            "LLVMDWP",
            "LLVMDebugInfoCodeView",
            "LLVMDebugInfoDWARF",
            "LLVMDebugInfoGSYM",
            "LLVMDebugInfoMSF",
            "LLVMDebugInfoPDB",
            "LLVMDlltoolDriver",
            "LLVMExecutionEngine",
            "LLVMExtensions",
            "LLVMFileCheck",
            "LLVMFrontendOpenACC",
            "LLVMFrontendOpenMP",
            "LLVMFuzzMutate",
            "LLVMGlobalISel",
            "LLVMIRReader",
            "LLVMInstCombine",
            "LLVMInstrumentation",
            "LLVMInterfaceStub",
            "LLVMInterpreter",
            "LLVMJITLink",
            "LLVMLTO",
            "LLVMLibDriver",
            "LLVMLineEditor",
            "LLVMLinker",
            "LLVMMC",
            "LLVMMCA",
            "LLVMMCDisassembler",
            "LLVMMCJIT",
            "LLVMMCParser",
            "LLVMMIRParser",
            "LLVMObjCARCOpts",
            "LLVMObject",
            "LLVMObjectYAML",
            "LLVMOption",
            "LLVMOrcJIT",
            "LLVMOrcShared",
            "LLVMOrcTargetProcess",
            "LLVMPasses",
            "LLVMProfileData",
            "LLVMRuntimeDyld",
            "LLVMScalarOpts",
            "LLVMSelectionDAG",
            "LLVMSymbolize",
            "LLVMTarget",
            "LLVMTextAPI",
            "LLVMTransformUtils",
            "LLVMVectorize",
            "LLVMX86AsmParser",
            "LLVMX86CodeGen",
            "LLVMX86Desc",
            "LLVMX86Disassembler",
            "LLVMX86Info",
            "LLVMXRay",
            "LLVMipo",
        ] {
            println!("cargo:rustc-link-lib=static={}", llvm_lib);
        }
    }

    Ok(())
}
