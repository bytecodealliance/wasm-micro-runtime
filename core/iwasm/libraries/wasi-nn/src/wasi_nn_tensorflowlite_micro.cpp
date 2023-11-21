/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasi_nn.h"
#include "wasi_nn_tensorflowlite_micro.hpp"
#include "logger.h"

#include "bh_platform.h"
#include "wasm_export.h"

#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_log.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <typeinfo>

/* Maximum number of graphs per WASM instance */
#define MAX_GRAPHS_PER_INST 10
/* Maximum number of graph execution context per WASM instance*/
#define MAX_GRAPH_EXEC_CONTEXTS_PER_INST 10

// In order to use optimized tensorflow lite kernels, a signed int8_t quantized
// model is preferred over the legacy unsigned model format. This means that
// throughout this project, input images must be converted from unisgned to
// signed format. The easiest and quickest way to convert from unsigned to
// signed 8-bit integers is to subtract 128 from the unsigned value to get a
// signed value.

constexpr int scratchBufSize = 39 * 1024;

// An area of memory to use for input, output, and intermediate arrays.
constexpr int kTensorArenaSize = 81 * 1024 + scratchBufSize;
// constexpr int kTensorArenaSize = 2 * 1024 * 1024;
static uint8_t* wasi_nn_tensor_arena[MAX_GRAPH_EXEC_CONTEXTS_PER_INST] = {};//[kTensorArenaSize]; // Maybe we should move this to external

/*static*/ tflite::MicroMutableOpResolver<6> wasi_nn_micro_op_resolver;

typedef struct {
    char *model_pointer;
    // Raw pointer rather than unique_ptr here, because tflite::Model and
    // tflite::MicroInterpreter don't have such constructor functions yet.
    tflite::Model* model;
    execution_target target;
} Model;

typedef struct {
    uint32_t current_models;
    Model models[MAX_GRAPHS_PER_INST];
    uint32_t current_interpreters;
    tflite::MicroInterpreter* interpreters[MAX_GRAPH_EXEC_CONTEXTS_PER_INST];
    korp_mutex g_lock;
} TFLiteContext;

typedef enum{
    INPUT_TENSOR = 0,
    OUTPUT_TENSOR,
} DataFlow;

/* Utils */

/* Transform TfLiteType to CPP type */
static size_t GetTfLiteTypeSize(TfLiteType tflite_type_enum)
{
    size_t type_size = 0;

    switch (tflite_type_enum) {
        case kTfLiteInt32:
            type_size = sizeof(int32_t);
            break;
        case kTfLiteUInt32:
            type_size = sizeof(uint32_t);
            break;
        case kTfLiteInt16:
            type_size = sizeof(int16_t);
            break;
        case kTfLiteUInt16:
            type_size = sizeof(uint16_t);
            break;
        case kTfLiteInt64:
            type_size = sizeof(int64_t);
            break;
        case kTfLiteFloat32:
            type_size = sizeof(float);
            break;
        case kTfLiteUInt8:
            type_size = sizeof(unsigned char);
            break;
        case kTfLiteInt8:
            type_size = sizeof(int8_t);
            break;
        case kTfLiteBool:
            type_size = sizeof(bool);
            break;
        case kTfLiteFloat16:
            type_size = sizeof(TfLiteFloat16);
            break;
        case kTfLiteFloat64:
            type_size = sizeof(double);
            break;
        case kTfLiteUInt64:
            type_size = sizeof(uint64_t);
            break;
        default:
            NN_ERR_PRINTF("Type not supported");
            break;
    }

    return type_size;
}

static void* GetTypedTensorData(tflite::MicroInterpreter* interpreters, TfLiteType tflite_type_enum, 
                    DataFlow dataflow, int tensor_idx)
{
    switch (tflite_type_enum) {
        case kTfLiteInt32:
            if (dataflow == INPUT_TENSOR)
                return interpreters->typed_input_tensor<int32_t>(tensor_idx);
            else if (dataflow == OUTPUT_TENSOR)
                return interpreters->typed_output_tensor<int32_t>(tensor_idx);
            break;
        case kTfLiteUInt32:
            if (dataflow == INPUT_TENSOR)
                return interpreters->typed_input_tensor<uint32_t>(tensor_idx);
            else if (dataflow == OUTPUT_TENSOR)
                return interpreters->typed_output_tensor<uint32_t>(tensor_idx);
            break;
        case kTfLiteInt16:
            if (dataflow == INPUT_TENSOR)
                return interpreters->typed_input_tensor<int16_t>(tensor_idx);
            else if (dataflow == OUTPUT_TENSOR)
                return interpreters->typed_output_tensor<int16_t>(tensor_idx);
            break;
        case kTfLiteUInt16:
            if (dataflow == INPUT_TENSOR)
                return interpreters->typed_input_tensor<uint16_t>(tensor_idx);
            else if (dataflow == OUTPUT_TENSOR)
                return interpreters->typed_output_tensor<uint16_t>(tensor_idx);
            break;
        case kTfLiteInt64:
            if (dataflow == INPUT_TENSOR)
                return interpreters->typed_input_tensor<int64_t>(tensor_idx);
            else if (dataflow == OUTPUT_TENSOR)
                return interpreters->typed_output_tensor<int64_t>(tensor_idx);
            break;
        case kTfLiteFloat32:
            if (dataflow == INPUT_TENSOR)
                return interpreters->typed_input_tensor<float>(tensor_idx);
            else if (dataflow == OUTPUT_TENSOR)
                return interpreters->typed_output_tensor<float>(tensor_idx);
            break;
        case kTfLiteUInt8:
            if (dataflow == INPUT_TENSOR)
                return interpreters->typed_input_tensor<unsigned char>(tensor_idx);
            else if (dataflow == OUTPUT_TENSOR)
                return interpreters->typed_output_tensor<unsigned char>(tensor_idx);
            break;
        case kTfLiteInt8:
            if (dataflow == INPUT_TENSOR)
                return interpreters->typed_input_tensor<int8_t>(tensor_idx);
            else if (dataflow == OUTPUT_TENSOR)
                return interpreters->typed_output_tensor<int8_t>(tensor_idx);
            break;
        case kTfLiteBool:
            if (dataflow == INPUT_TENSOR)
                return interpreters->typed_input_tensor<bool>(tensor_idx);
            else if (dataflow == OUTPUT_TENSOR)
                return interpreters->typed_output_tensor<bool>(tensor_idx);
            break;
        case kTfLiteFloat16:
            if (dataflow == INPUT_TENSOR)
                return interpreters->typed_input_tensor<TfLiteFloat16>(tensor_idx);
            else if (dataflow == OUTPUT_TENSOR)
                return interpreters->typed_output_tensor<TfLiteFloat16>(tensor_idx);
            break;
        case kTfLiteFloat64:
            if (dataflow == INPUT_TENSOR)
                return interpreters->typed_input_tensor<double>(tensor_idx);
            else if (dataflow == OUTPUT_TENSOR)
                return interpreters->typed_output_tensor<double>(tensor_idx);
            break;
        case kTfLiteUInt64:
            if (dataflow == INPUT_TENSOR)
                return interpreters->typed_input_tensor<uint64_t>(tensor_idx);
            else if (dataflow == OUTPUT_TENSOR)
                return interpreters->typed_output_tensor<uint64_t>(tensor_idx);
            break;
        default:
            NN_ERR_PRINTF("Data type not supported");
            return NULL;
    }
    NN_ERR_PRINTF("DataFlow type not supported");
    return NULL;
}

static error
initialize_g(TFLiteContext *tfl_ctx, graph *g)
{
    os_mutex_lock(&tfl_ctx->g_lock);
    if (tfl_ctx->current_models == MAX_GRAPHS_PER_INST) {
        os_mutex_unlock(&tfl_ctx->g_lock);
        NN_ERR_PRINTF("Excedded max graphs per WASM instance");
        return runtime_error;
    }
    *g = tfl_ctx->current_models++;
    os_mutex_unlock(&tfl_ctx->g_lock);
    return success;
}

static error
initialize_graph_ctx(TFLiteContext *tfl_ctx, graph g,
                     graph_execution_context *ctx)
{
    os_mutex_lock(&tfl_ctx->g_lock);
    if (tfl_ctx->current_interpreters == MAX_GRAPH_EXEC_CONTEXTS_PER_INST) {
        os_mutex_unlock(&tfl_ctx->g_lock);
        NN_ERR_PRINTF("Excedded max graph execution context per WASM instance");
        return runtime_error;
    }
    *ctx = tfl_ctx->current_interpreters++;
    os_mutex_unlock(&tfl_ctx->g_lock);
    return success;
}

static error
is_valid_graph(TFLiteContext *tfl_ctx, graph g)
{
    if (g >= MAX_GRAPHS_PER_INST) {
        NN_ERR_PRINTF("Invalid graph: %d >= %d.", g, MAX_GRAPHS_PER_INST);
        return runtime_error;
    }
    if (tfl_ctx->models[g].model_pointer == NULL) {
        NN_ERR_PRINTF("Context (model) non-initialized.");
        return runtime_error;
    }
    if (tfl_ctx->models[g].model == NULL) {
        NN_ERR_PRINTF("Context (tflite model) non-initialized.");
        return runtime_error;
    }
    return success;
}

static error
is_valid_graph_execution_context(TFLiteContext *tfl_ctx,
                                 graph_execution_context ctx)
{
    if (ctx >= MAX_GRAPH_EXEC_CONTEXTS_PER_INST) {
        NN_ERR_PRINTF("Invalid graph execution context: %d >= %d", ctx,
                      MAX_GRAPH_EXEC_CONTEXTS_PER_INST);
        return runtime_error;
    }
    if (tfl_ctx->interpreters[ctx] == NULL) {
        NN_ERR_PRINTF("Context (interpreter) non-initialized.");
        return runtime_error;
    }
    return success;
}

/* WASI-NN (tensorflow) implementation */

error
tensorflowlite_micro_load(void *tflite_ctx, graph_builder_array *builder,
                    graph_encoding encoding, execution_target target, graph *g)
{
    TFLiteContext *tfl_ctx = (TFLiteContext *)tflite_ctx;

    if (builder->size != 1) {
        NN_ERR_PRINTF("Unexpected builder format.");
        return invalid_argument;
    }

    if (encoding != tensorflowlite_micro) {
        NN_ERR_PRINTF("Encoding is not tensorflowlite_micro.");
        return invalid_argument;
    }

    if (target != cpu) {
        NN_ERR_PRINTF("Only CPU target is supported.");
        return invalid_argument;
    }

    error res;
    if (success != (res = initialize_g(tfl_ctx, g)))
        return res;

    uint32_t size = builder->buf[0].size;

    // Save model
    tfl_ctx->models[*g].model_pointer = (char *)malloc(size);
    if (tfl_ctx->models[*g].model_pointer == NULL) {
        NN_ERR_PRINTF("Error when allocating memory for model.");
        return missing_memory;
    }

    bh_memcpy_s(tfl_ctx->models[*g].model_pointer, size, builder->buf[0].buf,
                size);

    // Map the model into a usable data structure. This doesn't involve any
    // copying or parsing, it's a very lightweight operation.
    tfl_ctx->models[*g].model = const_cast<tflite::Model*>(tflite::GetModel(tfl_ctx->models[*g].model_pointer));

    if (tfl_ctx->models[*g].model == NULL) {
        NN_ERR_PRINTF("Loading model error.");
        free(tfl_ctx->models[*g].model_pointer);
        tfl_ctx->models[*g].model_pointer = NULL;
        return missing_memory;
    }
    if (tfl_ctx->models[*g].model->version() != TFLITE_SCHEMA_VERSION) {
        NN_ERR_PRINTF("Model provided is schema version %d not equal to supported "
                "version %d.", tfl_ctx->models[*g].model->version(), TFLITE_SCHEMA_VERSION);
        free(tfl_ctx->models[*g].model_pointer);
        return missing_memory;
    }

    // Save target
    tfl_ctx->models[*g].target = target;
    return success;
}

error
tensorflowlite_micro_init_execution_context(void *tflite_ctx, graph g,
                                      graph_execution_context *ctx)
{
    TFLiteContext *tfl_ctx = (TFLiteContext *)tflite_ctx;

    error res;
    if (success != (res = is_valid_graph(tfl_ctx, g)))
        return res;

    if (success != (res = initialize_graph_ctx(tfl_ctx, g, ctx)))
        return res;

    // tflite::MicroMutableOpResolver<6> wasi_nn_micro_op_resolver;
    // todo : maybe we can add more default op here? ......
    wasi_nn_micro_op_resolver.AddAveragePool2D();
    wasi_nn_micro_op_resolver.AddConv2D();
    wasi_nn_micro_op_resolver.AddDepthwiseConv2D();
    wasi_nn_micro_op_resolver.AddReshape();
    wasi_nn_micro_op_resolver.AddSoftmax();
    wasi_nn_micro_op_resolver.AddMaxPool2D();

    if (wasi_nn_tensor_arena[g] != NULL) {
        free(wasi_nn_tensor_arena[g]);
        wasi_nn_tensor_arena[g] = NULL;
    }
    if (wasi_nn_tensor_arena[g] == NULL) {
        // wasi_nn_tensor_arena = (uint8_t *) heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        wasi_nn_tensor_arena[g] = (uint8_t *)malloc(kTensorArenaSize);
    }
    if (wasi_nn_tensor_arena[g] == NULL) {
        NN_ERR_PRINTF("Couldn't allocate memory of %d bytes\n", kTensorArenaSize);
        return missing_memory;
    }

    tfl_ctx->interpreters[*ctx] = new tflite::MicroInterpreter(tfl_ctx->models[g].model, 
                                                               wasi_nn_micro_op_resolver, 
                                                               wasi_nn_tensor_arena[g], 
                                                               kTensorArenaSize);

    if (tfl_ctx->interpreters[*ctx] == NULL) {
        NN_ERR_PRINTF("Error when generating the interpreter.");
        free(wasi_nn_tensor_arena[g]);
        return missing_memory;
    }

    bool use_default = false;
    switch (tfl_ctx->models[g].target) {
        case gpu:
        {
            NN_WARN_PRINTF("GPU not enabled.");
            use_default = true;
            break;
        }
        default:
            use_default = true;
    }
    if (use_default)
        NN_WARN_PRINTF("Default encoding is CPU.");

    // Allocate memory from the wasi_nn_tensor_arena for the model's tensors.
    TfLiteStatus allocate_status = tfl_ctx->interpreters[*ctx]->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        NN_ERR_PRINTF("AllocateTensors() failed");
        free(wasi_nn_tensor_arena[g]);
        return missing_memory;
    }
    return success;
}

error
tensorflowlite_micro_set_input(void *tflite_ctx, graph_execution_context ctx,
                         uint32_t index, tensor *input_tensor)
{
    TFLiteContext *tfl_ctx = (TFLiteContext *)tflite_ctx;

    error res;
    if (success != (res = is_valid_graph_execution_context(tfl_ctx, ctx)))
        return res;

    uint32_t num_tensors =
        tfl_ctx->interpreters[ctx]->inputs().size();
    NN_DBG_PRINTF("Number of tensors (%d)", num_tensors);
    if (index + 1 > num_tensors) {
        return runtime_error;
    }

    auto model_tensor = tfl_ctx->interpreters[ctx]->input(index);
    if (model_tensor == NULL) {
        NN_ERR_PRINTF("Missing memory for model input tensor");
        return missing_memory;
    }

    uint32_t model_tensor_size = (uint32_t)model_tensor->bytes/GetTfLiteTypeSize(model_tensor->type);
    // uint32_t model_tensor_size = (uint32_t)model_tensor->bytes/sizeof(float);

    uint32_t input_tensor_size = 1;
    for (uint32_t i = 0; i < input_tensor->dimensions->size; i++)
        input_tensor_size *= (uint32_t)input_tensor->dimensions->buf[i];

    if (model_tensor_size != input_tensor_size) {
        NN_ERR_PRINTF("Input tensor shape from the model is different than the "
                      "one provided");
        return invalid_argument;
    }

    auto *input = GetTypedTensorData(tfl_ctx->interpreters[ctx], model_tensor->type, INPUT_TENSOR, index);
    if (input == NULL){
        NN_ERR_PRINTF("Missing memory for model input tensor");
        return missing_memory;
    }
    bh_memcpy_s(input, model_tensor_size * GetTfLiteTypeSize(model_tensor->type), input_tensor->data,
                model_tensor_size * GetTfLiteTypeSize(model_tensor->type));

    return success;
}

error
tensorflowlite_micro_compute(void *tflite_ctx, graph_execution_context ctx)
{
    TFLiteContext *tfl_ctx = (TFLiteContext *)tflite_ctx;

    error res;
    if (success != (res = is_valid_graph_execution_context(tfl_ctx, ctx)))
        return res;

    tfl_ctx->interpreters[ctx]->Invoke();
    return success;
}

error
tensorflowlite_micro_get_output(void *tflite_ctx, graph_execution_context ctx,
                          uint32_t index, tensor_data output_tensor,
                          uint32_t *output_tensor_size)
{
    TFLiteContext *tfl_ctx = (TFLiteContext *)tflite_ctx;

    error res;
    if (success != (res = is_valid_graph_execution_context(tfl_ctx, ctx)))
        return res;

    uint32_t num_output_tensors =
        tfl_ctx->interpreters[ctx]->outputs().size();
    NN_DBG_PRINTF("Number of tensors (%d)", num_output_tensors);

    if (index + 1 > num_output_tensors) {
        return runtime_error;
    }

    auto model_tensor = tfl_ctx->interpreters[ctx]->output_tensor(index);
    if (model_tensor == NULL) {
        NN_ERR_PRINTF("Missing memory");
        return missing_memory;
    }

    uint32_t model_tensor_size = 1;
    for (int i = 0; i < (int)model_tensor->dims->size; ++i)
        model_tensor_size *= (uint32_t)model_tensor->dims->data[i];

    if (*output_tensor_size < model_tensor_size) {
        NN_ERR_PRINTF("Insufficient memory to copy tensor %d", index);
        return missing_memory;
    }

    auto *output = GetTypedTensorData(tfl_ctx->interpreters[ctx], model_tensor->type, OUTPUT_TENSOR, index);
    for (uint32_t i = 0; i < model_tensor_size; ++i)
        NN_DBG_PRINTF("output: %f", ((float*)output)[i]);

    // int8_t person_score = model_tensor->data.uint8[1];
    // int8_t no_person_score = model_tensor->data.uint8[0];

    // float person_score_f =
    //     (person_score - model_tensor->params.zero_point) * model_tensor->params.scale;
    // float no_person_score_f =
    //     (no_person_score - model_tensor->params.zero_point) * model_tensor->params.scale;

    // int person_score_int = (person_score_f) * 100 + 0.5;
    // NN_DBG_PRINTF("person_score_f:%f%%, no_person_score_f:%f%%\n\n",
    //           (double)person_score_f * (double)100.0, (double)no_person_score_f * (double)100.0);
    *output_tensor_size = model_tensor_size;
    bh_memcpy_s(output_tensor, model_tensor_size * GetTfLiteTypeSize(model_tensor->type), output,
                model_tensor_size * GetTfLiteTypeSize(model_tensor->type));
    return success;
}

void
tensorflowlite_micro_initialize(void **tflite_ctx)
{
    TFLiteContext *tfl_ctx = new TFLiteContext();
    // TFLiteContext *tfl_ctx = (TFLiteContext*)malloc(sizeof(TFLiteContext));
    if (tfl_ctx == NULL) {
        NN_ERR_PRINTF("Error when allocating memory for tensorflowlite.");
        return;
    }

    NN_DBG_PRINTF("Initializing models.");
    tfl_ctx->current_models = 0;
    for (int i = 0; i < MAX_GRAPHS_PER_INST; ++i) {
        tfl_ctx->models[i].model_pointer = NULL;
    }
    
    for (int i = 0; i < MAX_GRAPH_EXEC_CONTEXTS_PER_INST; ++i) {
        tfl_ctx->interpreters[i] = NULL;
    }

    NN_DBG_PRINTF("Initializing interpreters.");
    tfl_ctx->current_interpreters = 0;

    if (os_mutex_init(&tfl_ctx->g_lock) != 0) {
        NN_ERR_PRINTF("Error while initializing the lock");
    }

    *tflite_ctx = (void *)tfl_ctx;
}

void
tensorflowlite_micro_destroy(void *tflite_ctx)
{
    /*
        TensorFlow Lite memory is internally managed by tensorflow

        Related issues:
        * https://github.com/tensorflow/tensorflow/issues/15880
    */
    TFLiteContext *tfl_ctx = (TFLiteContext *)tflite_ctx;

    NN_DBG_PRINTF("Freeing memory.");
    for (int i = 0; i < MAX_GRAPHS_PER_INST; ++i) {
        if (tfl_ctx->models[i].model_pointer)
            free(tfl_ctx->models[i].model_pointer);
        tfl_ctx->models[i].model = NULL;
        tfl_ctx->models[i].model_pointer = NULL;
    }
    for (int i = 0; i < MAX_GRAPH_EXEC_CONTEXTS_PER_INST; ++i) {
        if (tfl_ctx->interpreters[i])
            tfl_ctx->interpreters[i]->Reset();
        delete tfl_ctx->interpreters[i];
        tfl_ctx->interpreters[i] = NULL;
    }
    for (int i = 0; i < MAX_GRAPH_EXEC_CONTEXTS_PER_INST; ++i) {
        if (wasi_nn_tensor_arena[i])
            free(wasi_nn_tensor_arena[i]);
        wasi_nn_tensor_arena[i] = NULL;
    }
    os_mutex_destroy(&tfl_ctx->g_lock);
    delete tfl_ctx;
    NN_DBG_PRINTF("Memory free'd.");
}
