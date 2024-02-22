/*
 * Copyright (C) 2023 Liquid Reply GmbH. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

extern crate bindgen;
extern crate cmake;

use cmake::Config;
use std::{env, path::Path, path::PathBuf};

fn main() {
    let wamr_root = Path::new("../../../../").canonicalize().unwrap();
    assert!(wamr_root.exists());

    let llvm_dir = wamr_root.join("core/deps/llvm/build");
    assert!(llvm_dir.exists());

    let enable_llvm_jit = if cfg!(feature = "llvmjit") { "1" } else { "0" };
    let dst = Config::new(&wamr_root)
        // running mode
        .define("WAMR_BUILD_AOT", "1")
        .define("WAMR_BUILD_INTERP", "1")
        .define("WAMR_BUILD_FAST_INTERP", "1")
        .define("WAMR_BUILD_JIT", enable_llvm_jit)
        // mvp
        .define("WAMR_BUILD_BULK_MEMORY", "1")
        .define("WAMR_BUILD_REF_TYPES", "1")
        .define("WAMR_BUILD_SIMD", "1")
        // wasi
        .define("WAMR_BUILD_LIBC_WASI", "1")
        // `nostdlib`
        .define("WAMR_BUILD_LIBC_BUILTIN", "1")
        .build_target("iwasm_static")
        .build();

    println!("cargo:rustc-link-search=native={}/build", dst.display());
    println!("cargo:rustc-link-lib=static=vmlib");

    // //TODO: support macos?
    if cfg!(feature = "llvmjit") {
        println!("cargo:rustc-link-lib=dylib=dl");
        println!("cargo:rustc-link-lib=dylib=m");
        println!("cargo:rustc-link-lib=dylib=rt");
        println!("cargo:rustc-link-lib=dylib=stdc++");
        println!("cargo:rustc-link-lib=dylib=z");

        println!("cargo:libdir={}/lib", llvm_dir.display());
        println!("cargo:rustc-link-search=native={}/lib", llvm_dir.display());
        println!("cargo:rustc-link-lib=static=LLVMXRay");
        println!("cargo:rustc-link-lib=static=LLVMLibDriver");
        println!("cargo:rustc-link-lib=static=LLVMDlltoolDriver");
        println!("cargo:rustc-link-lib=static=LLVMCoverage");
        println!("cargo:rustc-link-lib=static=LLVMLineEditor");
        println!("cargo:rustc-link-lib=static=LLVMX86Disassembler");
        println!("cargo:rustc-link-lib=static=LLVMX86AsmParser");
        println!("cargo:rustc-link-lib=static=LLVMX86CodeGen");
        println!("cargo:rustc-link-lib=static=LLVMX86Desc");
        println!("cargo:rustc-link-lib=static=LLVMX86Info");
        println!("cargo:rustc-link-lib=static=LLVMOrcJIT");
        println!("cargo:rustc-link-lib=static=LLVMMCJIT");
        println!("cargo:rustc-link-lib=static=LLVMJITLink");
        println!("cargo:rustc-link-lib=static=LLVMInterpreter");
        println!("cargo:rustc-link-lib=static=LLVMExecutionEngine");
        println!("cargo:rustc-link-lib=static=LLVMRuntimeDyld");
        println!("cargo:rustc-link-lib=static=LLVMOrcTargetProcess");
        println!("cargo:rustc-link-lib=static=LLVMOrcShared");
        println!("cargo:rustc-link-lib=static=LLVMDWP");
        println!("cargo:rustc-link-lib=static=LLVMSymbolize");
        println!("cargo:rustc-link-lib=static=LLVMDebugInfoPDB");
        println!("cargo:rustc-link-lib=static=LLVMDebugInfoGSYM");
        println!("cargo:rustc-link-lib=static=LLVMOption");
        println!("cargo:rustc-link-lib=static=LLVMObjectYAML");
        println!("cargo:rustc-link-lib=static=LLVMMCA");
        println!("cargo:rustc-link-lib=static=LLVMMCDisassembler");
        println!("cargo:rustc-link-lib=static=LLVMLTO");
        println!("cargo:rustc-link-lib=static=LLVMPasses");
        println!("cargo:rustc-link-lib=static=LLVMCFGuard");
        println!("cargo:rustc-link-lib=static=LLVMCoroutines");
        println!("cargo:rustc-link-lib=static=LLVMObjCARCOpts");
        println!("cargo:rustc-link-lib=static=LLVMipo");
        println!("cargo:rustc-link-lib=static=LLVMVectorize");
        println!("cargo:rustc-link-lib=static=LLVMLinker");
        println!("cargo:rustc-link-lib=static=LLVMInstrumentation");
        println!("cargo:rustc-link-lib=static=LLVMFrontendOpenMP");
        println!("cargo:rustc-link-lib=static=LLVMFrontendOpenACC");
        println!("cargo:rustc-link-lib=static=LLVMExtensions");
        println!("cargo:rustc-link-lib=static=LLVMDWARFLinker");
        println!("cargo:rustc-link-lib=static=LLVMGlobalISel");
        println!("cargo:rustc-link-lib=static=LLVMMIRParser");
        println!("cargo:rustc-link-lib=static=LLVMAsmPrinter");
        println!("cargo:rustc-link-lib=static=LLVMDebugInfoMSF");
        println!("cargo:rustc-link-lib=static=LLVMDebugInfoDWARF");
        println!("cargo:rustc-link-lib=static=LLVMSelectionDAG");
        println!("cargo:rustc-link-lib=static=LLVMCodeGen");
        println!("cargo:rustc-link-lib=static=LLVMIRReader");
        println!("cargo:rustc-link-lib=static=LLVMAsmParser");
        println!("cargo:rustc-link-lib=static=LLVMInterfaceStub");
        println!("cargo:rustc-link-lib=static=LLVMFileCheck");
        println!("cargo:rustc-link-lib=static=LLVMFuzzMutate");
        println!("cargo:rustc-link-lib=static=LLVMTarget");
        println!("cargo:rustc-link-lib=static=LLVMScalarOpts");
        println!("cargo:rustc-link-lib=static=LLVMInstCombine");
        println!("cargo:rustc-link-lib=static=LLVMAggressiveInstCombine");
        println!("cargo:rustc-link-lib=static=LLVMTransformUtils");
        println!("cargo:rustc-link-lib=static=LLVMBitWriter");
        println!("cargo:rustc-link-lib=static=LLVMAnalysis");
        println!("cargo:rustc-link-lib=static=LLVMProfileData");
        println!("cargo:rustc-link-lib=static=LLVMObject");
        println!("cargo:rustc-link-lib=static=LLVMTextAPI");
        println!("cargo:rustc-link-lib=static=LLVMMCParser");
        println!("cargo:rustc-link-lib=static=LLVMMC");
        println!("cargo:rustc-link-lib=static=LLVMDebugInfoCodeView");
        println!("cargo:rustc-link-lib=static=LLVMBitReader");
        println!("cargo:rustc-link-lib=static=LLVMCore");
        println!("cargo:rustc-link-lib=static=LLVMRemarks");
        println!("cargo:rustc-link-lib=static=LLVMBitstreamReader");
        println!("cargo:rustc-link-lib=static=LLVMBinaryFormat");
        println!("cargo:rustc-link-lib=static=LLVMTableGen");
        println!("cargo:rustc-link-lib=static=LLVMSupport");
        println!("cargo:rustc-link-lib=static=LLVMDemangle");
    }

    let wamr_header = wamr_root.join("core/iwasm/include/wasm_export.h");
    assert!(wamr_header.exists());

    let bindings = bindgen::Builder::default()
        .ctypes_prefix("::core::ffi")
        .use_core()
        .header(wamr_header.into_os_string().into_string().unwrap())
        .derive_default(true)
        .generate()
        .expect("Unable to generate bindings");
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings");
}
