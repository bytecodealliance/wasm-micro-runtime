#include "lib_export.h"
#include "native_interface.h"
#include "connection_api.h"
#include "gui_api.h"

static NativeSymbol extended_native_symbol_defs[] = {
#include "runtime_sensor.inl"
#include "connection.inl"
#include "wamr_gui.inl"
};

#include "ext_lib_export.h"
