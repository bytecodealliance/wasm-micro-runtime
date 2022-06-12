#include "wasi_nn.h"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "wasm_export.h"

// #include <tensorflow/lite/interpreter.h>
// #include <tensorflow/lite/kernels/register.h>
// #include <tensorflow/lite/model.h>
// #include <tensorflow/lite/optional_debug_tools.h> 

// std::unique_ptr<tflite::Interpreter> interpreter = NULL;
// std::unique_ptr<tflite::FlatBufferModel> model = NULL;

void wasi_nn_load(wasm_exec_env_t exec_env, graph_builder_array builder, graph_encoding encoding)
{
    // tflite::ErrorReporter *error_reporter;
    // model = tflite::FlatBufferModel::BuildFromBuffer(
    //     (const char *)builder[0],
    //     1000, // TODO: find how to pass buffer size
    //     error_reporter
    // );
    // tflite::ops::builtin::BuiltinOpResolver resolver;
	// tflite::InterpreterBuilder(*model, resolver)(&interpreter);
    printf("Inside wasi_nn_load!\n\n");
}

void wasi_nn_init_execution_context()
{

}

void wasi_nn_set_input()
{
    // interpreter->AllocateTensors();
}

void wasi_nn_compute()
{

}

void wasi_nn_get_output()
{

}

/* clang-format off */
#define REG_NATIVE_FUNC(func_name, signature) \
    { #func_name, wasi_nn_##func_name, signature, NULL }
/* clang-format on */

static NativeSymbol native_symbols_wasi_nn[] = {
    REG_NATIVE_FUNC(load, "(ii)"),
};

uint32_t
get_wasi_nn_export_apis(NativeSymbol **p_libc_wasi_apis)
{
    *p_libc_wasi_apis = native_symbols_wasi_nn;
    return sizeof(native_symbols_wasi_nn) / sizeof(NativeSymbol);
}
