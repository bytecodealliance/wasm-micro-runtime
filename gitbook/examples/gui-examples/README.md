# GUI example

In this section, we provide you two examples using GUI with WAMR

- **[littlevgl](../../../samples/littlevgl/README.md)**: Demonstrating the graphic user interface application usage on WAMR. The whole [LVGL](https://github.com/lvgl/lvgl) 2D user graphic library and the UI application are built into the WASM application.  It uses **WASI libc** and executes apps in **AOT mode** by default.

- **[gui](../../../samples/gui/README.md)**: Move the [LVGL](https://github.com/lvgl/lvgl) library into the runtime and define a WASM application interface by wrapping the littlevgl API. It uses **WASI libc** and executes apps in **interpreter** mode by default.
