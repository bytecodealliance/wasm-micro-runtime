#include "lib_run_inference.hpp"

#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/model.h>
#include <tensorflow/lite/optional_debug_tools.h>
#include <tensorflow/lite/error_reporter.h>

enum Idx { GRAPH = 0, GRAPH_SIZE = 1 };

std::unique_ptr<tflite::Interpreter> interpreter = NULL;
std::unique_ptr<tflite::FlatBufferModel> model = NULL;

uint32_t
_load(graph_builder_array builder, graph_encoding encoding)
{

    if (encoding != tensorflow) {
        return invalid_argument;
    }

    uint32_t *size = (uint32_t *)builder[Idx::GRAPH_SIZE];

    tflite::ErrorReporter *error_reporter;

    model = tflite::FlatBufferModel::BuildFromBuffer(
        (const char *)builder[Idx::GRAPH], *size, error_reporter);

    if (model == nullptr) {
        printf("failure: null model \n");
        return invalid_argument;
    }

    // Build the interpreter with the InterpreterBuilder.
    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder tflite_builder(*model, resolver);
    tflite_builder(&interpreter);

    if (interpreter == nullptr) {
        printf("failure: null interpreter \n");
        return invalid_argument;
    }

    return success;
}


uint32_t set_input()
{

    for (int i=0 ;  i<input_tensor.size()  ;  i++  )
  	{
    	input[i]=  input_tensor[i];
  	}


}
