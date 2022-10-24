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
  * [How To Build Wasm Application](doc/build_wasm_app.md)
    * [More Tools To Create Wasm Application](doc/other_wasm_compilers.md)

## WAMR in practice

* [Tutorial](gitbook/tutorial/README.md)
  * [Build Tutorial](gitbook/tutorial/build_tutorial/README.md)
    * [Build Iwasm](doc/build_wamr.md)
    * [Build Wamrc](gitbook/tutorial/build_tutorial/build_wamrc.md)
  * [Language Embedding](gitbook/tutorial/language_embedding/README.md)
    * [C/C++](doc/embed_wamr.md)
    * [Python](language-bindings/python/README.md)
    * [Go](language-bindings/go/README.md)
  * [Debugging&IDE Support](gitbook/tutorial/debugging%26IDE_support/README.md)
    * [WAMR Source Debugging](doc/source_debugging.md)
    * [VS Code Support](test-tools/wamr-ide/README.md)
      * [Enable Debugging In VS Code](test-tools/wamr-ide/VSCode-Extension/README.md)
      * [Move lldb Binaries](test-tools/wamr-ide/VSCode-Extension/resource/debug/README.md)

* [Features](gitbook/features/demo_examples/README.md)
  * [Export Native APIs to WASM applications](doc/export_native_api.md)
  * [Multiple Modules As Dependencies](doc/multi_module.md)
  * [Multi-thread, pthread APIs and thread management](doc/pthread_library.md)
  * [Linux SGX(Intel Software Guard Extension) support](doc/linux_sgx.md)
  * [XIP(Execution In Place) support](doc/xip.md)
  * [Socket support](doc/socket_api.md)

* [User Case](gitbook/features/user_case/README.md)

## Programmer's Manual

* [Programmer's Manual](gitbook/programmer's_manual/README.md)
  * [C API Lists](gitbook/programmer's_manual/C_API_Lists.md)

## Community

* [How To Contribute](gitbook/community/contribute-howto.md)

* [WAMR On Github](https://github.com/bytecodealliance/wasm-micro-runtime)

* [WAMR Blogs](https://bytecodealliance.github.io/wamr.dev/)

## Appendix

* [Appendix A. Background Knowledge And Jargons](gitbook/appendix/background-knowledge.md)

* [Appendix B. WebAssembly Details](gitbook/appendix/webassembly-details.md)

* [Appendix C. Complete WAMR Introduction](README.md)
