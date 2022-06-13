#include "lib_run_inference.hpp"

#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/model.h>
#include <tensorflow/lite/optional_debug_tools.h> 

std::unique_ptr<tflite::Interpreter> interpreter = NULL;
std::unique_ptr<tflite::FlatBufferModel> model = NULL;

uint32_t _load(graph_builder_array builder, graph_encoding encoding) {
	uint32_t *size = (uint32_t*) builder[1];
	printf("inside _load: %d\n", *size);
	return suces;
    // tflite::ErrorReporter *error_reporter;
    // model = tflite::FlatBufferModel::BuildFromBuffer(
    //     (const char *)builder[0],
    //     1000, // TODO: find how to pass buffer size
    //     error_reporter
    // );
    // tflite::ops::builtin::BuiltinOpResolver resolver;
	// tflite::InterpreterBuilder(*model, resolver)(&interpreter);
}
