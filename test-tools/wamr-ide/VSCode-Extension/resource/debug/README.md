### If you want to enable `source debugging` for this extension, please build `lldb` firstly following this [instruction](../../../../../doc/source_debugging.md#debugging-with-interpreter).

### After building(`linux` for example), create `bin` folder and `lib` folder respectively in `linux` directory, add following necessary target files into the folders.

```shell
/llvm/build-lldb/bin/lldb # move this file to {VS Code extension directory}/resource/debug/linux/bin/
/llvm/build-lldb/bin/lldb-vscode # move this file to {VS Code extension directory}/resource/debug/linux/bin/
/llvm/build-lldb/lib/liblldb.so.13 # move this file to {VS Code extension directory}/resource/debug/linux/lib/
```

For example, on an Ubuntu machine the VS Code extension directory could be `/home/{usrname}/.vscode-server/extensions/wamr.wamride-1.1.2`

Note: For macOS, the library is named like `liblldb.13.0.1.dylib`.

### Then you can start the extension and run the execute source debugging by clicking the `debug` button in the extension panel.
