# Summary: structure of chapters and subchapters of the book

* [WAMR Document Home Page](gitbook/home_page.md)

## Basics

* [Introduction](gitbook/basics/introduction/README.md)
  * [WebAssembly](gitbook/basics/introduction/webassembly.md)
  * [WAMR Project](gitbook/basics/introduction/wamr-project.md)

* [Getting Started](gitbook/basics/getting_started/README.md)
  * [Host Environment Preparation](gitbook/basics/getting_started/host-prerequsites.md)
  * [Hello-world Program On Host](gitbook/basics/getting_started/on-host.md)
  * [Docker Environment Preparation](doc/devcontainer.md)
  * [Hello-world Program On Docker](gitbook/basics/getting_started/on-docker.md)
  * [Build And Run WASM Application](doc/build_wasm_app.md)
    * [More Tools To Create WASM Application](doc/other_wasm_compilers.md)

## WAMR In Practice

* [Tutorial](gitbook/tutorial/README.md)
  * [Build Tutorial](gitbook/tutorial/build_tutorial/README.md)
    * [Build IWASM](doc/build_wamr.md)
    * [Build WAMRC](gitbook/tutorial/build_tutorial/build_wamrc.md)
  * [Language Embedding](gitbook/tutorial/language_embedding/README.md)
    * [C/C++](doc/embed_wamr.md)
    * [Python](language-bindings/python/README.md)
    * [Go](language-bindings/go/README.md)
  * [Debugging&IDE Support](gitbook/tutorial/debugging%26IDE_support/README.md)
    * [WAMR Source Debugging With LLDB](doc/source_debugging.md)
    * [VS Code Support](test-tools/wamr-ide/README.md)
      * [Enable Debugging In VS Code](test-tools/wamr-ide/VSCode-Extension/README.md)
      * [Move LLDB Binaries](test-tools/wamr-ide/VSCode-Extension/resource/debug/README.md)

* [Advance Tutorial](gitbook/advance_tutorial/README.md)
  * [Performance Test And Fine-tuning](gitbook/advance_tutorial/performance_tuning/README.md)
    * [Memory Usage Tunning](doc/memory_tune.md)
  * [Application Framework](doc/wamr_api.md)
  * [Remote Application Management](gitbook/advance_tutorial/remote_applicatoin_management/README.md)
    * [Example Program: Using "host_tool"](samples/simple/README.md)

* [Features](gitbook/features/README.md)
  * [Export Native APIs To WASM Applications](doc/export_native_api.md)
    * [Example 1: Export C Functions to WASM](samples/basic/README.md)
    * [Example 2: Using "native-lib"](samples/native-lib/README.md)
  * [Multiple Modules As Dependencies](doc/multi_module.md)
    * [Multi-modules Example](samples/multi-module/README.md)
  * [Multi-thread, Pthread APIs And Thread Management](doc/pthread_library.md)
    * [Multi-thread Example](samples/multi-thread/README.md)
  * [Linux SGX(Intel Software Guard Extension) Support](doc/linux_sgx.md)
    * [Example: SGX Remote Attestation](samples/sgx-ra/README.md)
  * [XIP(Execution In Place) Support](doc/xip.md)
  * [Socket Support](doc/socket_api.md)
    * [Example: Use Socket Api in WAMR](samples/socket-api/README.md)
  * [Post-MVP Features](gitbook/features/demo_examples/README.md)
    * [WASM C API](samples/wasm-c-api/README.md)
    * [128-bit SIMD](samples/workload/README.md)
    * [Reference Types](samples/ref-types)

* [More Examples](gitbook/examples/README.md)
  * [File Interaction Of WASI](samples/file/README.md)
  * [GUI Example](gitbook/examples/gui_examples/README.md)
    * [Littlevgl](samples/littlevgl/README.md)
    * [LVGL](samples/gui/README.md)
  * [Same WASM Program Executing Concurrently](samples/spawn-thread)
  * [Build And Run Workload](samples/workload)

* [User Case](gitbook/features/user_case/README.md)

## Programmer's Manual

* [Programmer's Manual](gitbook/programmer's_manual/README.md)
  * [C API Lists](gitbook/programmer's_manual/C_API_Lists.md)

## Community

* [How To Contribute](CONTRIBUTING.md)

* [WAMR On Github](https://github.com/bytecodealliance/wasm-micro-runtime)

* [WAMR Blogs](https://bytecodealliance.github.io/wamr.dev/)

## Appendix

* [Appendix A. Background Knowledge And Glossary Of Terms](gitbook/appendix/background-knowledge.md)

* [Appendix B. WebAssembly Details](gitbook/appendix/webassembly-details.md)

* [Appendix C. Complete WAMR Guide](README.md)
