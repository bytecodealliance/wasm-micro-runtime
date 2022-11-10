/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2020-2021 Alibaba Cloud
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _RATS_WAMR_API_H
#define _RATS_WAMR_API_H

#include <stdint.h>
#include <string.h>
#include "lib_rats_common.h"

#ifdef __cplusplus
extern "C" {
#endif

int
librats_collect(rats_sgx_evidence_t *evidence, uint32_t evidence_size,
                const char *buffer, uint32_t buffer_size);

int
librats_verify(rats_sgx_evidence_t *evidence, uint32_t evidence_size,
               const char *buffer, uint32_t buffer_size);

#define librats_collect(evidence, buffer) \
    librats_collect(evidence, sizeof(evidence), buffer, strlen(buffer))

#define librats_verify(evidence, buffer) \
    librats_verify(evidence, sizeof(evidence), buffer, strlen(buffer))

#ifdef __cplusplus
}
#endif

#endif