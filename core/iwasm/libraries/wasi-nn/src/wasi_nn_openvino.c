/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasi_nn_types.h"
#include "wasi_nn_openvino.h"
#include "utils/logger.h"
#include "bh_platform.h"

#include "openvino/c/openvino.h"

/*
 * refer to
 * https://docs.openvino.ai/2024/openvino-workflow/running-inference/integrate-openvino-with-your-application.html
 *
 * Steps about integrating OpenVINO are:
 *
 * 1. Create OpenVINO Runtime Core
 * 2. Compile Model
 * 3. Create Inference Request
 * 4. Set Inputs
 * 5. Start Inference
 * 6. Process Inference Results
 *
 * from 4. to 6. is the Inference Loop
 */

typedef struct {
    ov_core_t *core;
    /* keep input model files */
    void *weight_data;
    ov_tensor_t *weights_tensor;
    ov_model_t *model;
    /* add prepostprocess */
    ov_model_t *new_model;
    ov_compiled_model_t *compiled_model;
    ov_infer_request_t *infer_request;
    ov_tensor_t *input_tensor;
} OpenVINOContext;

/*
 * BE AWARE OF "goto fail"
 */
#define CHECK_OV_STATUS(status, error_code)                \
    do {                                                   \
        ov_status_e s = status;                            \
        if (s != OK) {                                     \
            NN_ERR_PRINTF("return status \"%s\", line %d", \
                          ov_get_error_info(s), __LINE__); \
            error_code = runtime_error;                    \
            goto fail;                                     \
        }                                                  \
    } while (0)

static void
dump_ov_shape_t(const ov_shape_t *shape, int32_t output_len, char *output)
{
    int ret = 0;

    ret = snprintf(output, output_len, "%ld,[", shape->rank);
    if (!ret)
        return;

    output_len -= ret;
    output += ret;

    for (unsigned i = 0; i < shape->rank && output_len; i++) {
        ret = snprintf(output, output_len, " %ld", shape->dims[i]);
        if (!ret)
            return;

        output_len -= ret;
        output += ret;
    }

    snprintf(output, output_len, "]");
    return;
}

#ifndef NDEBUG
static void
print_model_input_output_info(ov_model_t *model)
{
    wasi_nn_error ov_error = success;
    char *friendly_name = NULL;
    size_t input_size = 0;
    ov_output_const_port_t *input_port = NULL;
    ov_shape_t input_shape = { 0 };
    ov_element_type_e input_type;
    char shape_info[64] = { 0 };
    ov_output_const_port_t *output_port = NULL;
    ov_shape_t output_shape = { 0 };
    ov_element_type_e output_type;

    CHECK_OV_STATUS(ov_model_get_friendly_name(model, &friendly_name),
                    ov_error);
    NN_DBG_PRINTF("model name: %s", friendly_name);

    ov_model_inputs_size(model, &input_size);
    for (unsigned i = 0; i < input_size; i++) {
        CHECK_OV_STATUS(ov_model_const_input_by_index(model, i, &input_port),
                        ov_error);
        CHECK_OV_STATUS(ov_const_port_get_shape(input_port, &input_shape),
                        ov_error);
        CHECK_OV_STATUS(ov_port_get_element_type(input_port, &input_type),
                        ov_error);

        dump_ov_shape_t(&input_shape, 60, shape_info);
        NN_DBG_PRINTF("model input[%u]. element_type: %d, shape: %s", i,
                      input_type, shape_info);

        ov_shape_free(&input_shape);
        memset(&input_shape, 0, sizeof(input_shape));
        ov_output_const_port_free(input_port);
        input_port = NULL;
    }

    size_t output_size = 0;
    ov_model_outputs_size(model, &output_size);
    for (unsigned i = 0; i < output_size; i++) {
        CHECK_OV_STATUS(ov_model_const_output_by_index(model, i, &output_port),
                        ov_error);
        CHECK_OV_STATUS(ov_const_port_get_shape(output_port, &output_shape),
                        ov_error);
        CHECK_OV_STATUS(ov_port_get_element_type(output_port, &output_type),
                        ov_error);

        dump_ov_shape_t(&output_shape, 60, shape_info);
        NN_DBG_PRINTF("model output[%u]. element_type: %d, shape: %s", i,
                      output_type, shape_info);

        ov_shape_free(&output_shape);
        memset(&output_shape, 0, sizeof(output_shape));
        ov_output_const_port_free(output_port);
        output_port = NULL;
    }

    ov_error = ov_error;
fail:
    if (friendly_name)
        ov_free(friendly_name);
    ov_shape_free(&input_shape);
    if (input_port)
        ov_output_const_port_free(input_port);
    ov_shape_free(&output_shape);
    if (output_port)
        ov_output_const_port_free(output_port);
    return;
}
#endif

static ov_element_type_e
wasi_nn_tensor_type_to_openvino_element_type(tensor_type wasi_nn_type)
{
    switch (wasi_nn_type) {
        case fp16:
            return F16;
        case fp32:
            return F32;
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
        case fp64:
            return F64;
        case bf16:
            return BF16;
        case i64:
            return I64;
        case u8:
            return U8;
        case i32:
            return I32;
#else
        case up8:
            return U8;
        case ip32:
            return I32;
#endif
        default:
            break;
    }

    NN_ERR_PRINTF("%d is an undefined tensor type", wasi_nn_type);
    return UNDEFINED;
}

static wasi_nn_error
uint32_array_to_int64_array(uint32_t array_size, uint32_t *src, int64_t **dst)
{
    *dst = os_malloc(array_size * sizeof(int64_t));
    if (!(*dst))
        return runtime_error;

    for (unsigned i = 0; i < array_size; i++) {
        (*dst)[i] = src[i];
    }

    return success;
}

__attribute__((visibility("default"))) wasi_nn_error
load(void *ctx, graph_builder_array *builder, graph_encoding encoding,
     execution_target target, graph *g)
{
    OpenVINOContext *ov_ctx = (OpenVINOContext *)ctx;
    wasi_nn_error ret = unsupported_operation;

    if (encoding != openvino) {
        NN_ERR_PRINTF("Unexpected encoding %d.", encoding);
        return invalid_argument;
    }

    /*FIXME: unblock non-cpu device after supporting */
    if (target != cpu) {
        NN_ERR_PRINTF("Unexpected device %d.", target);
        return invalid_argument;
    }

    if (builder->size != 2) {
        NN_ERR_PRINTF("Unexpected builder format.");
        return invalid_argument;
    }

    /*
     * The first builder is the XML file.
     * The second builder is the weight file.
     */
    graph_builder xml = builder->buf[0];
    graph_builder weight = builder->buf[1];

    /* if xml is a String with a model in IR */
    if (!(xml.buf[xml.size] == '\0' && xml.buf[xml.size - 1] != '\0')) {
        NN_ERR_PRINTF("Invalid xml string.");
        return invalid_argument;
    }

    /* transfer weight to an ov tensor */
    {
        ov_ctx->weight_data = os_malloc(weight.size);
        if (!ov_ctx->weight_data)
            goto fail;
        memcpy(ov_ctx->weight_data, weight.buf, weight.size);

        ov_element_type_e type = U8;
        int64_t dims[1] = { weight.size };
        ov_shape_t shape = { 1, dims };
        CHECK_OV_STATUS(ov_tensor_create_from_host_ptr(type, shape,
                                                       ov_ctx->weight_data,
                                                       &ov_ctx->weights_tensor),
                        ret);
    }

    /* load model from buffer */
    CHECK_OV_STATUS(ov_core_read_model_from_memory_buffer(
                        ov_ctx->core, (char *)xml.buf, xml.size,
                        ov_ctx->weights_tensor, &ov_ctx->model),
                    ret);
#ifndef NDEBUG
    print_model_input_output_info(ov_ctx->model);
#endif

    ret = success;
fail:
    return ret;
}

__attribute__((visibility("default"))) wasi_nn_error
load_by_name(void *ctx, const char *filename, uint32_t filename_len, graph *g)
{
    OpenVINOContext *ov_ctx = (OpenVINOContext *)ctx;
    wasi_nn_error ret = unsupported_operation;

    CHECK_OV_STATUS(
        ov_core_read_model(ov_ctx->core, filename, NULL, &ov_ctx->model), ret);

    ret = success;
fail:
    return ret;
}

__attribute__((visibility("default"))) wasi_nn_error
init_execution_context(void *ctx, graph g, graph_execution_context *exec_ctx)
{
    return success;
}

__attribute__((visibility("default"))) wasi_nn_error
set_input(void *ctx, graph_execution_context exec_ctx, uint32_t index,
          tensor *wasi_nn_tensor)
{
    OpenVINOContext *ov_ctx = (OpenVINOContext *)ctx;
    wasi_nn_error ret = unsupported_operation;
    ov_shape_t input_shape = { 0 };
    int64_t *ov_dims = NULL;

    ov_preprocess_prepostprocessor_t *ppp = NULL;
    ov_preprocess_input_info_t *input_info = NULL;
    ov_preprocess_input_tensor_info_t *input_tensor_info = NULL;
    ov_layout_t *input_layout = NULL;
    ov_preprocess_preprocess_steps_t *input_process = NULL;
    ov_preprocess_input_model_info_t *p_input_model = NULL;
    ov_layout_t *model_layout = NULL;
    ov_preprocess_output_info_t *output_info = NULL;
    ov_preprocess_output_tensor_info_t *output_tensor_info = NULL;

    /* wasi_nn_tensor -> ov_tensor */
    {
        ret = uint32_array_to_int64_array(wasi_nn_tensor->dimensions->size,
                                          wasi_nn_tensor->dimensions->buf,
                                          &ov_dims);
        if (ret != success)
            goto fail;

        /* NCHW -> NHWC */
        if (wasi_nn_tensor->dimensions->size == 4 || ov_dims[1] == 3) {
            /* N */
            /* H */
            ov_dims[1] = ov_dims[2];
            /* W */
            ov_dims[2] = ov_dims[3];
            /* C */
            ov_dims[3] = 3;
        }

        CHECK_OV_STATUS(ov_shape_create(wasi_nn_tensor->dimensions->size,
                                        ov_dims, &input_shape),
                        ret);

        ov_element_type_e input_type =
            wasi_nn_tensor_type_to_openvino_element_type(wasi_nn_tensor->type);
        if (input_type == UNDEFINED)
            goto fail;

        char shape_info[64] = { 0 };
        dump_ov_shape_t(&input_shape, 60, shape_info);
        NN_DBG_PRINTF("input tensor. element_type: %d, shape: %s", input_type,
                      shape_info);

        CHECK_OV_STATUS(ov_tensor_create_from_host_ptr(input_type, input_shape,
                                                       wasi_nn_tensor->data,
                                                       &ov_ctx->input_tensor),
                        ret);
    }

    /* set preprocess based on wasi_nn_tensor */
    {
        CHECK_OV_STATUS(
            ov_preprocess_prepostprocessor_create(ov_ctx->model, &ppp), ret);

        /* reuse user' created tensor's info */
        CHECK_OV_STATUS(ov_preprocess_prepostprocessor_get_input_info_by_index(
                            ppp, index, &input_info),
                        ret);
        CHECK_OV_STATUS(ov_preprocess_input_info_get_tensor_info(
                            input_info, &input_tensor_info),
                        ret);
        CHECK_OV_STATUS(ov_preprocess_input_tensor_info_set_from(
                            input_tensor_info, ov_ctx->input_tensor),
                        ret);
        /* ! HAS TO BE NHWC. Match previous layout conversion */
        CHECK_OV_STATUS(ov_layout_create("NHWC", &input_layout), ret);
        CHECK_OV_STATUS(ov_preprocess_input_tensor_info_set_layout(
                            input_tensor_info, input_layout),
                        ret);

        /* add RESIZE */
        CHECK_OV_STATUS(ov_preprocess_input_info_get_preprocess_steps(
                            input_info, &input_process),
                        ret);
        CHECK_OV_STATUS(
            ov_preprocess_preprocess_steps_resize(input_process, RESIZE_LINEAR),
            ret);

        /* input model */
        CHECK_OV_STATUS(
            ov_preprocess_input_info_get_model_info(input_info, &p_input_model),
            ret);
        // TODO: what if not?
        CHECK_OV_STATUS(ov_layout_create("NCHW", &model_layout), ret);
        CHECK_OV_STATUS(ov_preprocess_input_model_info_set_layout(p_input_model,
                                                                  model_layout),
                        ret);

        /* output -> F32(possibility) */
        CHECK_OV_STATUS(ov_preprocess_prepostprocessor_get_output_info_by_index(
                            ppp, index, &output_info),
                        ret);
        CHECK_OV_STATUS(ov_preprocess_output_info_get_tensor_info(
                            output_info, &output_tensor_info),
                        ret);
        CHECK_OV_STATUS(
            ov_preprocess_output_set_element_type(output_tensor_info, F32),
            ret);

        CHECK_OV_STATUS(
            ov_preprocess_prepostprocessor_build(ppp, &ov_ctx->new_model), ret);
    }

    CHECK_OV_STATUS(ov_core_compile_model(ov_ctx->core, ov_ctx->new_model,
                                          "CPU", 0, &ov_ctx->compiled_model),
                    ret);

    CHECK_OV_STATUS(ov_compiled_model_create_infer_request(
                        ov_ctx->compiled_model, &ov_ctx->infer_request),
                    ret);

    /* install ov_tensor -> infer_request */
    CHECK_OV_STATUS(ov_infer_request_set_input_tensor_by_index(
                        ov_ctx->infer_request, index, ov_ctx->input_tensor),
                    ret);
    ret = success;

fail:
    if (ov_dims)
        os_free(ov_dims);
    ov_shape_free(&input_shape);
    if (ppp)
        ov_preprocess_prepostprocessor_free(ppp);
    if (input_info)
        ov_preprocess_input_info_free(input_info);
    if (input_tensor_info)
        ov_preprocess_input_tensor_info_free(input_tensor_info);
    if (input_layout)
        ov_layout_free(input_layout);
    if (input_process)
        ov_preprocess_preprocess_steps_free(input_process);
    if (p_input_model)
        ov_preprocess_input_model_info_free(p_input_model);
    if (model_layout)
        ov_layout_free(model_layout);
    if (output_info)
        ov_preprocess_output_info_free(output_info);
    if (output_tensor_info)
        ov_preprocess_output_tensor_info_free(output_tensor_info);

    return ret;
}

__attribute__((visibility("default"))) wasi_nn_error
compute(void *ctx, graph_execution_context exec_ctx)
{
    OpenVINOContext *ov_ctx = (OpenVINOContext *)ctx;
    wasi_nn_error ret = unsupported_operation;

    CHECK_OV_STATUS(ov_infer_request_infer(ov_ctx->infer_request), ret);
    ret = success;
fail:
    return ret;
}

__attribute__((visibility("default"))) wasi_nn_error
get_output(void *ctx, graph_execution_context exec_ctx, uint32_t index,
           tensor_data output_tensor, uint32_t *output_tensor_size)
{
    OpenVINOContext *ov_ctx = (OpenVINOContext *)ctx;
    wasi_nn_error ret = unsupported_operation;
    ov_tensor_t *ov_tensor = NULL;
    void *data = NULL;
    size_t byte_size = 0;

    CHECK_OV_STATUS(ov_infer_request_get_output_tensor_by_index(
                        ov_ctx->infer_request, index, &ov_tensor),
                    ret);

    CHECK_OV_STATUS(ov_tensor_get_byte_size(ov_tensor, &byte_size), ret);

    CHECK_OV_STATUS(ov_tensor_data(ov_tensor, &data), ret);

    memcpy(output_tensor, data, byte_size);

    *output_tensor_size = (uint32_t)byte_size;

    ret = success;

fail:
    if (ov_tensor)
        ov_tensor_free(ov_tensor);
    return ret;
}

__attribute__((visibility("default"))) wasi_nn_error
init_backend(void **ctx)
{
    ov_version_t version;
    OpenVINOContext *ov_ctx = NULL;
    wasi_nn_error ret = unsupported_operation;

    if (!ctx) {
        ret = invalid_argument;
        goto fail;
    }

    /* Get OpenVINO runtime version */
    CHECK_OV_STATUS(ov_get_openvino_version(&version), ret);
    NN_INFO_PRINTF("OpenVINO INFO:");
    NN_INFO_PRINTF("  Description : %s", version.description);
    NN_INFO_PRINTF("  Build Number: %s", version.buildNumber);
    ov_version_free(&version);

    ov_ctx = (OpenVINOContext *)os_malloc(sizeof(OpenVINOContext));
    if (!ov_ctx) {
        NN_ERR_PRINTF("Allocate for OpenVINOContext failed");
        ret = runtime_error;
        goto fail;
    }

    memset(ov_ctx, 0, sizeof(OpenVINOContext));

    /* Initialize OpenVINO Runtime Core */
    CHECK_OV_STATUS(ov_core_create(&ov_ctx->core), ret);

    *ctx = (void *)ov_ctx;
    return success;
fail:
    openvino_destroy((void *)ov_ctx);
    return ret;
}

__attribute__((visibility("default"))) wasi_nn_error
deinit_backend(void *ctx)
{
    OpenVINOContext *ov_ctx = (OpenVINOContext *)ctx;

    if (!ov_ctx)
        return invalid_argument;

    if (ov_ctx->weight_data)
        os_free(ov_ctx->weight_data);

    if (ov_ctx->weights_tensor)
        ov_tensor_free(ov_ctx->weights_tensor);

    if (ov_ctx->input_tensor)
        ov_tensor_free(ov_ctx->input_tensor);

    if (ov_ctx->infer_request)
        ov_infer_request_free(ov_ctx->infer_request);

    if (ov_ctx->compiled_model)
        ov_compiled_model_free(ov_ctx->compiled_model);

    if (ov_ctx->model)
        ov_model_free(ov_ctx->model);

    if (ov_ctx->core)
        ov_core_free(ov_ctx->core);

    os_free(ov_ctx);
    return success;
}
