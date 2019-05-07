Attention: 
=======
Only add files are shared by both wasm application and native runtime into this directory!

The c files are both compiled into the the WASM APP and native runtime.

native_interface.h
=============
The interface declaration for the native API which are exposed to the WASM app

Any API in this file should be incuded with EXPORT_WASM_API() somewhere.

