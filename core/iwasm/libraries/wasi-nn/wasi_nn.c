#include "wasi_nn.h"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/model.h>
#include <tensorflow/lite/optional_debug_tools.h> 

std::unique_ptr<tflite::Interpreter> interpreter = NULL;
std::unique_ptr<tflite::FlatBufferModel> model =NULL;

//Load model functions 

int main()
{
    char* f=read_file("ssd_tflite");

    uint8_t *builder = (uint8_t*) f;

    uint8_t *builder_array[1] = { builder };

    load(builder_array, 1);

    load(model builder, graph_encoding encoding)
}


void load(graph_builder_array builder, graph_encoding encoding)
{
    model = tflite::FlatBufferModel::BuildFromBuffer(builder[0]);
    tflite::ops::builtin::BuiltinOpResolver resolver;
	tflite::InterpreterBuilder builder(*model, resolver);
	builder(&interpreter);

}

void init_execution_context()
{

}

//run inference functions

void set_input()
{
    interpreter->AllocateTensors() ;

}

void compute (graph_execution_context context )
{

    return 
}


void get_output()
{

}

/*
static NativeSymbol native_symbols_wasi_nn[] = {
    REG_NATIVE_FUNC(args_get, "(**)i"),
    REG_NATIVE_FUNC(args_sizes_get, "(**)i"),
    REG_NATIVE_FUNC(clock_res_get, "(i*)i"),
    REG_NATIVE_FUNC(clock_time_get, "(iI*)i"),
    REG_NATIVE_FUNC(environ_get, "(**)i"),
    REG_NATIVE_FUNC(environ_sizes_get, "(**)i"),
};
*/


uint32
get_wasi_nn_export_apis(NativeSymbol **p_libc_wasi_apis)
{
    *p_libc_wasi_apis = native_symbols_libc_wasi;
    return sizeof(native_symbols_libc_wasi) / sizeof(NativeSymbol);
}
