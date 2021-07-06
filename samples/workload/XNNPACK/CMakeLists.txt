# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required (VERSION 3.0)

project(xnnpack_wasm)

################  EMCC ################
if(NOT DEFINED ENV{EMSDK})
  message(SEND_ERROR
    "can not find emsdk. "
    "please refer to https://emscripten.org/docs/getting_started/downloads.html "
    "and install it, "
    "or active emsdk by 'source ./emsdk_env.sh'"
  )
endif()

include(ExternalProject)

ExternalProject_Add(xnnpack
    PREFIX xnnpack
    GIT_REPOSITORY https://github.com/google/XNNPACK.git
    GIT_TAG        master
    GIT_PROGRESS   ON
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/xnnpack
    UPDATE_COMMAND git checkout .
                   && git apply ${CMAKE_CURRENT_SOURCE_DIR}/xnnpack.patch
    CONFIGURE_COMMAND ""
    BUILD_COMMAND  cd ${CMAKE_CURRENT_SOURCE_DIR}/xnnpack
                   && bazel --output_user_root=build_user_output build -c opt --config=wasm
                        //:qs8_gemm_bench
                        //:qs8_requantization_bench
                        //:qu8_gemm_bench
                        //:qu8_requantization_bench
                        //:f16_igemm_bench
                        //:f16_gemm_bench
                        //:f16_spmm_bench
                        //:f32_igemm_bench
                        //:f16_relu_bench
                        //:f32_conv_hwc_bench
                        //:f32_conv_hwc2chw_bench
                        //:f16_dwconv_bench
                        //:f32_dwconv_bench
                        //:f32_dwconv2d_chw_bench
                        //:f32_gemm_bench
                        //:f32_hswish_bench
                        //:f32_raddexpminusmax_bench
                        //:f32_raddextexp_bench
                        //:f32_raddstoreexpminusmax_bench
                        //:f32_relu_bench
                        //:f32_rmax_bench
                        //:f32_sigmoid_bench
                        //:f32_spmm_bench
                        //:f32_softmax_bench
                        //:f32_velu_bench
                        //:f32_vscaleexpminusmax_bench
                        //:f32_vscaleextexp_bench
                        //:f32_vsqrt_bench
                        //:f32_im2col_gemm_bench
                        //:rounding_bench
                        //:average_pooling_bench
                        //:bankers_rounding_bench
                        //:ceiling_bench
                        //:channel_shuffle_bench
                        //:convolution_bench
                        //:deconvolution_bench
                        //:elu_bench
                        //:floor_bench
                        //:global_average_pooling_bench
                        //:hardswish_bench
                        //:max_pooling_bench
                        //:sigmoid_bench
                        //:prelu_bench
                        //:softmax_bench
                        //:square_root_bench
                        //:truncation_bench
                        //:f32_dwconv_e2e_bench
                        //:f32_gemm_e2e_bench
                        //:qs8_gemm_e2e_bench
                        //:end2end_bench
                        //:f32_exp_ulp_eval
                        //:f32_expminus_ulp_eval
                        //:f32_expm1minus_ulp_eval
                        //:f32_extexp_ulp_eval
                        //:f32_sigmoid_ulp_eval
                        //:f32_sqrt_ulp_eval
                        #--sandbox_debug
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory
                      ${CMAKE_CURRENT_SOURCE_DIR}/xnnpack/bazel-out/wasm-opt/bin/
                      ${CMAKE_CURRENT_SOURCE_DIR}/build/wasm-opt
)
