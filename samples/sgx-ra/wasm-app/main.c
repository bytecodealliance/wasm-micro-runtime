/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2020-2021 Alibaba Cloud
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include "lib_rats_wrapper.h"

int
main(int argc, char **argv)
{
    // If you want to declare the evidence of type rats_sgx_evidence_t on the
    // stack, you should modify the stack size of the CMAKE_EXE_LINKER_FLAGS in
    // CMakeLists.txt to 51200 at least.
    rats_sgx_evidence_t *evidence =
        (rats_sgx_evidence_t *)malloc(sizeof(rats_sgx_evidence_t));
    if (!evidence) {
        printf("ERROR: No memory to allocate.\n");
    }

    // Generate user_data by SHA256 buffer and the wasm module.
    // user_data = SHA256(buffer || sha256_wasm_module)
    const char *buffer = "This is a sample.";

    int ret_code = librats_collect(evidence, buffer);
    if (ret_code != 0) {
        printf("ERROR: collect evidence failed, %#x", ret_code);
    }

    // You could use these parameters for further verification.
    printf("User Data: %s\n", evidence->user_data);
    printf("MRENCLAVE: %s\n", evidence->mr_enclave);
    printf("MRSIGNER: %s\n", evidence->mr_signer);
    printf("Product ID: %u\n", evidence->product_id);
    printf("Security Version: %u\n", evidence->security_version);
    printf("Attributes.flags: %llu\n", evidence->att_flags);
    printf("Attribute.xfrm: %llu\n", evidence->att_xfrm);

    ret_code = librats_verify(evidence, buffer);
    if (ret_code != 0) {
        printf("Evidence is not trusted, error code: %#x.\n", ret_code);
    }
    else {
        printf("Evidence is trusted.\n");
    }

    if (evidence) {
        free(evidence);
    }

    return 0;
}
