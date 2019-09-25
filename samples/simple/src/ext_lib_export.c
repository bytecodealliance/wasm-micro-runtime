#include "lib_export.h"
#include "sensor_api.h"
#include "connection_api.h"

#if WASM_ENABLE_GUI != 0
#include "gui_api.h"
#endif

static NativeSymbol extended_native_symbol_defs[] = {
#include "runtime_sensor.inl"
#include "connection.inl"
#if WASM_ENABLE_GUI != 0
#include "wamr_gui.inl"
#endif
        };

#include "ext_lib_export.h"
