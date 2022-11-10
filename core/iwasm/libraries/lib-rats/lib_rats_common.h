/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2020-2021 Alibaba Cloud
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _RATS_WAMR_COMMON_H
#define _RATS_WAMR_COMMON_H

#include <stdint.h>
#include <stddef.h>

typedef struct rats_sgx_evidence {
    char quote[8192];          /* The quote of the Enclave */
    uint32_t quote_size;       /* The size of the quote */
    char user_data[64];        /* The custom data in the quote */
    uint32_t product_id;       /* Product ID of the Enclave */
    char mr_enclave[32];       /* The MRENCLAVE of the Enclave */
    uint32_t security_version; /* Security Version of the Enclave */
    char mr_signer[32];        /* The MRSIGNER of the Enclave */
    uint64_t att_flags;        /* Flags of the Enclave in attributes */
    uint64_t att_xfrm;         /* XSAVE Feature Request Mask */
} rats_sgx_evidence_t;

#endif