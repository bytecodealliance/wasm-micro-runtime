/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "wasi_nn_private.h"
#include "wasi_nn.h"
#include "utils/logger.h"
#include "onnxruntime_c_api.h"

/* Maximum number of graphs and execution contexts */
#define MAX_GRAPHS 10
#define MAX_CONTEXTS 10

/* ONNX Runtime context structure */
typedef struct {
    OrtEnv *env;
    OrtSessionOptions *session_options;
    OrtAllocator *allocator;
    const OrtApi *ort_api;
    std::mutex mutex;
    bool is_initialized;
} OnnxRuntimeContext;

/* Graph structure */
typedef struct {
    OrtSession *session;
    bool is_initialized;
} OnnxRuntimeGraph;

/* Execution context structure */
typedef struct {
    OrtMemoryInfo *memory_info;
    std::vector<const char *> input_names;
    std::vector<const char *> output_names;
    std::unordered_map<uint32_t, OrtValue *> inputs;
    std::unordered_map<uint32_t, OrtValue *> outputs;
    OnnxRuntimeGraph *graph;
    bool is_initialized;
} OnnxRuntimeExecCtx;

/* Global variables */
static OnnxRuntimeContext g_ort_ctx;
static OnnxRuntimeGraph g_graphs[MAX_GRAPHS];
static OnnxRuntimeExecCtx g_exec_ctxs[MAX_CONTEXTS];

/* Helper functions */
static void
check_status_and_log(OrtStatus *status)
{
    if (status != nullptr) {
        const char *msg = g_ort_ctx.ort_api->GetErrorMessage(status);
        NN_ERR_PRINTF("ONNX Runtime error: %s", msg);
        g_ort_ctx.ort_api->ReleaseStatus(status);
    }
}

static wasi_nn_error
convert_ort_error_to_wasi_nn_error(OrtStatus *status)
{
    if (status == nullptr) {
        return success;
    }

    wasi_nn_error err;
    OrtErrorCode code = g_ort_ctx.ort_api->GetErrorCode(status);
    const char *msg = g_ort_ctx.ort_api->GetErrorMessage(status);

    NN_ERR_PRINTF("ONNX Runtime error: %s", msg);

    switch (code) {
        case ORT_INVALID_ARGUMENT:
            err = invalid_argument;
            break;
        case ORT_RUNTIME_EXCEPTION:
            err = runtime_error;
            break;
        case ORT_NOT_IMPLEMENTED:
            err = unsupported_operation;
            break;
        case ORT_INVALID_PROTOBUF:
            err = invalid_encoding;
            break;
        case ORT_MODEL_LOADED:
            err = too_large;
            break;
        case ORT_INVALID_GRAPH:
            err = invalid_encoding;
            break;
        default:
            err = unknown;
            break;
    }

    g_ort_ctx.ort_api->ReleaseStatus(status);
    return err;
}

static tensor_type
convert_ort_type_to_wasi_nn_type(ONNXTensorElementDataType ort_type)
{
    switch (ort_type) {
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:
            return fp32;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16:
            return fp16;
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE:
            return fp64;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8:
            return u8;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:
            return i32;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64:
            return i64;
#else
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8:
            return up8;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:
            return ip32;
#endif
        default:
            NN_WARN_PRINTF("Unsupported ONNX tensor type: %d", ort_type);
            return fp32; // Default to fp32
    }
}

static ONNXTensorElementDataType
convert_wasi_nn_type_to_ort_type(tensor_type type)
{
    switch (type) {
        case fp32:
            return ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT;
        case fp16:
            return ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16;
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
        case fp64:
            return ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE;
        case u8:
            return ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8;
        case i32:
            return ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32;
        case i64:
            return ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64;
#else
        case up8:
            return ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8;
        case ip32:
            return ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32;
#endif
        default:
            NN_WARN_PRINTF("Unsupported wasi-nn tensor type: %d", type);
            return ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT; // Default to float
    }
}

static size_t
get_tensor_element_size(tensor_type type)
{
    switch (type) {
        case fp32:
            return 4;
        case fp16:
            return 2;
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
        case fp64:
            return 8;
        case u8:
            return 1;
        case i32:
            return 4;
        case i64:
            return 8;
#else
        case up8:
            return 1;
        case ip32:
            return 4;
#endif
        default:
            NN_WARN_PRINTF("Unsupported tensor type: %d", type);
            return 4; // Default to 4 bytes (float)
    }
}

/* Backend API implementation */

extern "C" {

__attribute__((visibility("default"))) wasi_nn_error
init_backend(void **onnx_ctx)
{
    std::lock_guard<std::mutex> lock(g_ort_ctx.mutex);

    if (g_ort_ctx.is_initialized) {
        *onnx_ctx = &g_ort_ctx;
        return success;
    }

    g_ort_ctx.ort_api = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    if (!g_ort_ctx.ort_api) {
        NN_ERR_PRINTF("Failed to get ONNX Runtime API");
        return unknown;
    }

    NN_INFO_PRINTF("Creating ONNX Runtime environment...");
    OrtStatus *status = g_ort_ctx.ort_api->CreateEnv(ORT_LOGGING_LEVEL_VERBOSE, "wasi-nn", &g_ort_ctx.env);
    if (status != nullptr) {
        const char* error_message = g_ort_ctx.ort_api->GetErrorMessage(status);
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        NN_ERR_PRINTF("Failed to create ONNX Runtime environment: %s", error_message);
        g_ort_ctx.ort_api->ReleaseStatus(status);
        return err;
    }
    NN_INFO_PRINTF("ONNX Runtime environment created successfully");

    status = g_ort_ctx.ort_api->CreateSessionOptions(&g_ort_ctx.session_options);
    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        g_ort_ctx.ort_api->ReleaseEnv(g_ort_ctx.env);
        NN_ERR_PRINTF("Failed to create ONNX Runtime session options");
        return err;
    }

    status = g_ort_ctx.ort_api->SetSessionGraphOptimizationLevel(g_ort_ctx.session_options, ORT_ENABLE_BASIC);
    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        g_ort_ctx.ort_api->ReleaseSessionOptions(g_ort_ctx.session_options);
        g_ort_ctx.ort_api->ReleaseEnv(g_ort_ctx.env);
        NN_ERR_PRINTF("Failed to set graph optimization level");
        return err;
    }

    status = g_ort_ctx.ort_api->GetAllocatorWithDefaultOptions(&g_ort_ctx.allocator);
    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        g_ort_ctx.ort_api->ReleaseSessionOptions(g_ort_ctx.session_options);
        g_ort_ctx.ort_api->ReleaseEnv(g_ort_ctx.env);
        NN_ERR_PRINTF("Failed to get default allocator");
        return err;
    }

    for (int i = 0; i < MAX_GRAPHS; i++) {
        g_graphs[i].is_initialized = false;
        g_graphs[i].session = nullptr;
    }
    
    for (int i = 0; i < MAX_CONTEXTS; i++) {
        g_exec_ctxs[i].is_initialized = false;
        g_exec_ctxs[i].memory_info = nullptr;
        g_exec_ctxs[i].graph = nullptr;
        g_exec_ctxs[i].input_names.clear();
        g_exec_ctxs[i].output_names.clear();
        g_exec_ctxs[i].inputs.clear();
        g_exec_ctxs[i].outputs.clear();
    }

    g_ort_ctx.is_initialized = true;
    *onnx_ctx = &g_ort_ctx;

    NN_INFO_PRINTF("ONNX Runtime backend initialized");
    return success;
}

__attribute__((visibility("default"))) wasi_nn_error
deinit_backend(void *onnx_ctx)
{
    OnnxRuntimeContext *ctx = (OnnxRuntimeContext *)onnx_ctx;
    std::lock_guard<std::mutex> lock(ctx->mutex);

    if (!ctx->is_initialized) {
        return success;
    }

    for (int i = 0; i < MAX_GRAPHS; i++) {
        if (g_graphs[i].is_initialized) {
            ctx->ort_api->ReleaseSession(g_graphs[i].session);
            g_graphs[i].is_initialized = false;
        }
    }

    for (int i = 0; i < MAX_CONTEXTS; i++) {
        if (g_exec_ctxs[i].is_initialized) {
            for (auto &input : g_exec_ctxs[i].inputs) {
                ctx->ort_api->ReleaseValue(input.second);
            }
            for (auto &output : g_exec_ctxs[i].outputs) {
                ctx->ort_api->ReleaseValue(output.second);
            }
            ctx->ort_api->ReleaseMemoryInfo(g_exec_ctxs[i].memory_info);
            g_exec_ctxs[i].is_initialized = false;
        }
    }

    ctx->ort_api->ReleaseSessionOptions(ctx->session_options);
    ctx->ort_api->ReleaseEnv(ctx->env);
    ctx->is_initialized = false;

    NN_INFO_PRINTF("ONNX Runtime backend deinitialized");
    return success;
}

__attribute__((visibility("default"))) wasi_nn_error
load(void *onnx_ctx, graph_builder_array *builder, graph_encoding encoding,
     execution_target target, graph *g)
{
    if (encoding != onnx) {
        NN_ERR_PRINTF("Unsupported encoding: %d", encoding);
        return invalid_encoding;
    }

    if (target != cpu) {
        NN_ERR_PRINTF("Only CPU target is supported");
        return unsupported_operation;
    }

    OnnxRuntimeContext *ctx = (OnnxRuntimeContext *)onnx_ctx;
    std::lock_guard<std::mutex> lock(ctx->mutex);

    int graph_index = -1;
    for (int i = 0; i < MAX_GRAPHS; i++) {
        if (!g_graphs[i].is_initialized) {
            graph_index = i;
            break;
        }
    }

    if (graph_index == -1) {
        NN_ERR_PRINTF("Maximum number of graphs reached");
        return runtime_error;
    }

    if (builder->size == 0 || builder->buf == NULL) {
        NN_ERR_PRINTF("No model data provided");
        return invalid_argument;
    }

    NN_INFO_PRINTF("[ONNX Runtime] Loading model of size %zu bytes...", builder->buf[0].size);

    if (builder->buf[0].size > 16) {
        NN_INFO_PRINTF("Model header bytes: %02x %02x %02x %02x %02x %02x %02x %02x",
                     ((uint8_t*)builder->buf[0].buf)[0], ((uint8_t*)builder->buf[0].buf)[1],
                     ((uint8_t*)builder->buf[0].buf)[2], ((uint8_t*)builder->buf[0].buf)[3],
                     ((uint8_t*)builder->buf[0].buf)[4], ((uint8_t*)builder->buf[0].buf)[5],
                     ((uint8_t*)builder->buf[0].buf)[6], ((uint8_t*)builder->buf[0].buf)[7]);
    }

    OrtStatus *status = ctx->ort_api->CreateSessionFromArray(
        ctx->env, builder->buf[0].buf, builder->buf[0].size,
        ctx->session_options, &g_graphs[graph_index].session);
    
    if (status != nullptr) {
        const char* error_message = ctx->ort_api->GetErrorMessage(status);
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        NN_ERR_PRINTF("Failed to create ONNX Runtime session: %s", error_message);
        ctx->ort_api->ReleaseStatus(status);
        return err;
    }
    
    NN_INFO_PRINTF("ONNX Runtime session created successfully");

    g_graphs[graph_index].is_initialized = true;
    *g = graph_index;

    NN_INFO_PRINTF("ONNX model loaded as graph %d", graph_index);
    return success;
}

__attribute__((visibility("default"))) wasi_nn_error
load_by_name(void *onnx_ctx, const char *name, graph *g)
{
    OnnxRuntimeContext *ctx = (OnnxRuntimeContext *)onnx_ctx;
    std::lock_guard<std::mutex> lock(ctx->mutex);

    int graph_index = -1;
    for (int i = 0; i < MAX_GRAPHS; i++) {
        if (!g_graphs[i].is_initialized) {
            graph_index = i;
            break;
        }
    }

    if (graph_index == -1) {
        NN_ERR_PRINTF("Maximum number of graphs reached");
        return runtime_error;
    }

    OrtStatus *status = ctx->ort_api->CreateSession(
        ctx->env, name, ctx->session_options, &g_graphs[graph_index].session);

    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        NN_ERR_PRINTF("Failed to create ONNX Runtime session from file: %s", name);
        return err;
    }

    g_graphs[graph_index].is_initialized = true;
    *g = graph_index;

    NN_INFO_PRINTF("ONNX model loaded from file %s as graph %d", name, graph_index);
    return success;
}

__attribute__((visibility("default"))) wasi_nn_error
init_execution_context(void *onnx_ctx, graph g, graph_execution_context *ctx)
{
    if (g >= MAX_GRAPHS || !g_graphs[g].is_initialized) {
        NN_ERR_PRINTF("Invalid graph handle: %d", g);
        return invalid_argument;
    }

    OnnxRuntimeContext *ort_ctx = (OnnxRuntimeContext *)onnx_ctx;
    std::lock_guard<std::mutex> lock(ort_ctx->mutex);

    int ctx_index = -1;
    for (int i = 0; i < MAX_CONTEXTS; i++) {
        if (!g_exec_ctxs[i].is_initialized) {
            ctx_index = i;
            break;
        }
    }

    if (ctx_index == -1) {
        NN_ERR_PRINTF("Maximum number of execution contexts reached");
        return runtime_error;
    }

    OnnxRuntimeExecCtx *exec_ctx = &g_exec_ctxs[ctx_index];
    exec_ctx->graph = &g_graphs[g];

    OrtStatus *status = ort_ctx->ort_api->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &exec_ctx->memory_info);
    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        NN_ERR_PRINTF("Failed to create CPU memory info");
        return err;
    }

    size_t num_input_nodes;
    status = ort_ctx->ort_api->SessionGetInputCount(exec_ctx->graph->session, &num_input_nodes);
    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        ort_ctx->ort_api->ReleaseMemoryInfo(exec_ctx->memory_info);
        NN_ERR_PRINTF("Failed to get input count");
        return err;
    }

    for (size_t i = 0; i < num_input_nodes; i++) {
        char *input_name;
        status = ort_ctx->ort_api->SessionGetInputName(exec_ctx->graph->session, i, ort_ctx->allocator, &input_name);
        if (status != nullptr) {
            wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
            ort_ctx->ort_api->ReleaseMemoryInfo(exec_ctx->memory_info);
            NN_ERR_PRINTF("Failed to get input name");
            return err;
        }
        exec_ctx->input_names.push_back(input_name);
    }

    size_t num_output_nodes;
    status = ort_ctx->ort_api->SessionGetOutputCount(exec_ctx->graph->session, &num_output_nodes);
    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        ort_ctx->ort_api->ReleaseMemoryInfo(exec_ctx->memory_info);
        for (const char *name : exec_ctx->input_names) {
            ort_ctx->allocator->Free(ort_ctx->allocator, (void *)name);
        }
        NN_ERR_PRINTF("Failed to get output count");
        return err;
    }

    for (size_t i = 0; i < num_output_nodes; i++) {
        char *output_name;
        status = ort_ctx->ort_api->SessionGetOutputName(exec_ctx->graph->session, i, ort_ctx->allocator, &output_name);
        if (status != nullptr) {
            wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
            ort_ctx->ort_api->ReleaseMemoryInfo(exec_ctx->memory_info);
            for (const char *name : exec_ctx->input_names) {
                ort_ctx->allocator->Free(ort_ctx->allocator, (void *)name);
            }
            NN_ERR_PRINTF("Failed to get output name");
            return err;
        }
        exec_ctx->output_names.push_back(output_name);
    }

    exec_ctx->is_initialized = true;
    *ctx = ctx_index;

    NN_INFO_PRINTF("Execution context %d initialized for graph %d", ctx_index, g);
    return success;
}

__attribute__((visibility("default"))) wasi_nn_error
set_input(void *onnx_ctx, graph_execution_context ctx, uint32_t index, tensor *input_tensor)
{
    if (ctx >= MAX_CONTEXTS || !g_exec_ctxs[ctx].is_initialized) {
        NN_ERR_PRINTF("Invalid execution context handle: %d", ctx);
        return invalid_argument;
    }

    if (index >= g_exec_ctxs[ctx].input_names.size()) {
        NN_ERR_PRINTF("Invalid input index: %d (max: %zu)", index, g_exec_ctxs[ctx].input_names.size() - 1);
        return invalid_argument;
    }

    OnnxRuntimeContext *ort_ctx = (OnnxRuntimeContext *)onnx_ctx;
    std::lock_guard<std::mutex> lock(ort_ctx->mutex);
    OnnxRuntimeExecCtx *exec_ctx = &g_exec_ctxs[ctx];

    size_t num_dims = input_tensor->dimensions->size;
    int64_t *ort_dims = (int64_t *)malloc(num_dims * sizeof(int64_t));
    if (!ort_dims) {
        NN_ERR_PRINTF("Failed to allocate memory for tensor dimensions");
        return runtime_error;
    }

    for (size_t i = 0; i < num_dims; i++) {
        ort_dims[i] = input_tensor->dimensions->buf[i];
    }

    ONNXTensorElementDataType ort_type = convert_wasi_nn_type_to_ort_type(input_tensor->type);

    OrtValue *input_value = nullptr;
    size_t total_elements = 1;
    for (size_t i = 0; i < num_dims; i++) {
        total_elements *= input_tensor->dimensions->buf[i];
    }
    
    OrtStatus *status = ort_ctx->ort_api->CreateTensorWithDataAsOrtValue(
        exec_ctx->memory_info, input_tensor->data,
        get_tensor_element_size(input_tensor->type) * total_elements,
        ort_dims, num_dims, ort_type, &input_value);

    free(ort_dims);

    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        NN_ERR_PRINTF("Failed to create input tensor");
        return err;
    }

    if (exec_ctx->inputs.count(index) > 0) {
        ort_ctx->ort_api->ReleaseValue(exec_ctx->inputs[index]);
    }
    exec_ctx->inputs[index] = input_value;

    NN_INFO_PRINTF("Input tensor set for context %d, index %d", ctx, index);
    return success;
}

__attribute__((visibility("default"))) wasi_nn_error
compute(void *onnx_ctx, graph_execution_context ctx)
{
    if (ctx >= MAX_CONTEXTS || !g_exec_ctxs[ctx].is_initialized) {
        NN_ERR_PRINTF("Invalid execution context handle: %d", ctx);
        return invalid_argument;
    }

    OnnxRuntimeContext *ort_ctx = (OnnxRuntimeContext *)onnx_ctx;
    std::lock_guard<std::mutex> lock(ort_ctx->mutex);
    OnnxRuntimeExecCtx *exec_ctx = &g_exec_ctxs[ctx];

    std::vector<OrtValue *> input_values;
    std::vector<const char *> input_names;

    for (size_t i = 0; i < exec_ctx->input_names.size(); i++) {
        if (exec_ctx->inputs.count(i) == 0) {
            NN_ERR_PRINTF("Input tensor not set for index %zu", i);
            return invalid_argument;
        }
        input_values.push_back(exec_ctx->inputs[i]);
        input_names.push_back(exec_ctx->input_names[i]);
    }

    for (auto &output : exec_ctx->outputs) {
        ort_ctx->ort_api->ReleaseValue(output.second);
    }
    exec_ctx->outputs.clear();

    std::vector<OrtValue*> output_values(exec_ctx->output_names.size());
    
    OrtStatus *status = ort_ctx->ort_api->Run(
        exec_ctx->graph->session, nullptr,
        input_names.data(), input_values.data(), input_values.size(),
        exec_ctx->output_names.data(), exec_ctx->output_names.size(), output_values.data());
        
    for (size_t i = 0; i < output_values.size(); i++) {
        exec_ctx->outputs[i] = output_values[i];
    }

    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        NN_ERR_PRINTF("Failed to run inference");
        return err;
    }

    NN_INFO_PRINTF("Inference computed for context %d", ctx);
    return success;
}

__attribute__((visibility("default"))) wasi_nn_error
get_output(void *onnx_ctx, graph_execution_context ctx, uint32_t index, tensor_data out_buffer, uint32_t *out_buffer_size)
{
    if (ctx >= MAX_CONTEXTS || !g_exec_ctxs[ctx].is_initialized) {
        NN_ERR_PRINTF("Invalid execution context handle: %d", ctx);
        return invalid_argument;
    }

    if (index >= g_exec_ctxs[ctx].output_names.size()) {
        NN_ERR_PRINTF("Invalid output index: %d (max: %zu)", index, g_exec_ctxs[ctx].output_names.size() - 1);
        return invalid_argument;
    }

    OnnxRuntimeContext *ort_ctx = (OnnxRuntimeContext *)onnx_ctx;
    std::lock_guard<std::mutex> lock(ort_ctx->mutex);
    OnnxRuntimeExecCtx *exec_ctx = &g_exec_ctxs[ctx];

    OrtValue *output_value = exec_ctx->outputs[index];
    if (!output_value) {
        NN_ERR_PRINTF("Output tensor not available for index %d", index);
        return runtime_error;
    }

    OrtTensorTypeAndShapeInfo *tensor_info;
    OrtStatus *status = ort_ctx->ort_api->GetTensorTypeAndShape(output_value, &tensor_info);
    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        NN_ERR_PRINTF("Failed to get tensor type and shape");
        return err;
    }

    ONNXTensorElementDataType element_type;
    status = ort_ctx->ort_api->GetTensorElementType(tensor_info, &element_type);
    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        ort_ctx->ort_api->ReleaseTensorTypeAndShapeInfo(tensor_info);
        NN_ERR_PRINTF("Failed to get tensor element type");
        return err;
    }

    size_t num_dims;
    status = ort_ctx->ort_api->GetDimensionsCount(tensor_info, &num_dims);
    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        ort_ctx->ort_api->ReleaseTensorTypeAndShapeInfo(tensor_info);
        NN_ERR_PRINTF("Failed to get tensor dimensions count");
        return err;
    }

    int64_t *dims = (int64_t *)malloc(num_dims * sizeof(int64_t));
    if (!dims) {
        ort_ctx->ort_api->ReleaseTensorTypeAndShapeInfo(tensor_info);
        NN_ERR_PRINTF("Failed to allocate memory for tensor dimensions");
        return runtime_error;
    }

    status = ort_ctx->ort_api->GetDimensions(tensor_info, dims, num_dims);
    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        free(dims);
        ort_ctx->ort_api->ReleaseTensorTypeAndShapeInfo(tensor_info);
        NN_ERR_PRINTF("Failed to get tensor dimensions");
        return err;
    }

    size_t tensor_size;
    status = ort_ctx->ort_api->GetTensorShapeElementCount(tensor_info, &tensor_size);
    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        free(dims);
        ort_ctx->ort_api->ReleaseTensorTypeAndShapeInfo(tensor_info);
        NN_ERR_PRINTF("Failed to get tensor element count");
        return err;
    }

    NN_INFO_PRINTF("Output tensor dimensions: ");
    for (size_t i = 0; i < num_dims; i++) {
        NN_INFO_PRINTF("  dim[%zu] = %lld", i, dims[i]);
    }
    NN_INFO_PRINTF("Total elements: %zu", tensor_size);

    ort_ctx->ort_api->ReleaseTensorTypeAndShapeInfo(tensor_info);
    free(dims);

    if (tensor_size == 0) {
        NN_ERR_PRINTF("Tensor is empty (zero elements)");
        return runtime_error;
    }

    void *tensor_data = nullptr;
    status = ort_ctx->ort_api->GetTensorMutableData(output_value, &tensor_data);
    if (status != nullptr) {
        wasi_nn_error err = convert_ort_error_to_wasi_nn_error(status);
        NN_ERR_PRINTF("Failed to get tensor data");
        return err;
    }
    
    if (tensor_data == nullptr) {
        NN_ERR_PRINTF("Tensor data pointer is null");
        return runtime_error;
    }

    size_t element_size;
    switch (element_type) {
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:
            element_size = sizeof(float);
            break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16:
            element_size = sizeof(uint16_t);
            break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE:
            element_size = sizeof(double);
            break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:
            element_size = sizeof(int32_t);
            break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64:
            element_size = sizeof(int64_t);
            break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8:
            element_size = sizeof(uint8_t);
            break;
        default:
            NN_ERR_PRINTF("Unsupported tensor element type: %d", element_type);
            return unsupported_operation;
    }

    size_t output_size_bytes = tensor_size * element_size;

    NN_INFO_PRINTF("Output tensor size: %zu elements, element size: %zu bytes, total: %zu bytes", 
                  tensor_size, element_size, output_size_bytes);

    if (*out_buffer_size < output_size_bytes) {
        NN_ERR_PRINTF("Output buffer too small: %u bytes provided, %zu bytes needed", 
                     *out_buffer_size, output_size_bytes);
        *out_buffer_size = output_size_bytes;
        return invalid_argument;
    }

    if (tensor_data == nullptr) {
        NN_ERR_PRINTF("Tensor data is null");
        return runtime_error;
    }

    if (out_buffer == nullptr) {
        NN_ERR_PRINTF("Output buffer is null");
        return invalid_argument;
    }

    memcpy(out_buffer, tensor_data, output_size_bytes);
    *out_buffer_size = output_size_bytes;

    NN_INFO_PRINTF("Output tensor retrieved for context %d, index %d, size %zu bytes", 
                  ctx, index, output_size_bytes);
    return success;
}

} /* End of extern "C" */
