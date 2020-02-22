#include "lib_export.h"
#include "sensor_native_api.h"
#include "timer_native_api.h"
#include "req_resp_native_api.h"
#include "connection_native_api.h"

static NativeSymbol extended_native_symbol_defs[] = {
#include "runtime_sensor.inl"
#include "connection.inl"
};

#include "ext_lib_export.h"
