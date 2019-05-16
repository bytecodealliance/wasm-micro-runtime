#include "lib-export.h"
#include "sensor_api.h"

static NativeSymbol extended_native_symbol_defs[] = {
#include "runtime_sensor.inl"
        };

#include "ext-lib-export.h"
