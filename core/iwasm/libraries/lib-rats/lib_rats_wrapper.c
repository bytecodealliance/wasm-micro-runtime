/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2020-2021 Alibaba Cloud
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include <librats/api.h>
#include <string.h>
#include <openssl/sha.h>

#include "sgx_quote_3.h"
#include "wasm_export.h"
#include "bh_common.h"
#include "lib_rats_common.h"

extern char wasm_module_hash[SHA256_DIGEST_LENGTH];

static int
librats_collect_wrapper(wasm_exec_env_t exec_env, rats_sgx_evidence_t *evidence,
                        uint32_t evidence_size, const char *buffer,
                        uint32_t buffer_size)
{
    attestation_evidence_t att_ev;

    char final_hash[SHA256_DIGEST_LENGTH] = { '\0' };
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, wasm_module_hash, SHA256_DIGEST_LENGTH);
    if (buffer != NULL)
        SHA256_Update(&sha256, buffer, strlen(buffer));
    SHA256_Final((unsigned char *)final_hash, &sha256);

    rats_attester_err_t ret_code =
        librats_collect_evidence(&att_ev, (const uint8_t *)final_hash);
    if (ret_code != RATS_ATTESTER_ERR_NONE) {
        return (int)ret_code;
    }

    sgx_quote3_t *quote_ptr = (sgx_quote3_t *)att_ev.ecdsa.quote;
    bh_memcpy_s(evidence->quote, att_ev.ecdsa.quote_len, att_ev.ecdsa.quote,
                att_ev.ecdsa.quote_len);
    evidence->quote_size = att_ev.ecdsa.quote_len;
    bh_memcpy_s(evidence->user_data, SGX_REPORT_DATA_SIZE,
                quote_ptr->report_body.report_data.d, SGX_REPORT_DATA_SIZE);
    bh_memcpy_s(evidence->mr_enclave, sizeof(sgx_measurement_t),
                quote_ptr->report_body.mr_enclave.m, sizeof(sgx_measurement_t));
    bh_memcpy_s(evidence->mr_signer, sizeof(sgx_measurement_t),
                quote_ptr->report_body.mr_signer.m, sizeof(sgx_measurement_t));
    evidence->product_id = quote_ptr->report_body.isv_prod_id;
    evidence->security_version = quote_ptr->report_body.isv_svn;
    evidence->att_flags = quote_ptr->report_body.attributes.flags;
    evidence->att_xfrm = quote_ptr->report_body.attributes.flags;

    return 0;
}

static int
librats_verify_wrapper(wasm_exec_env_t exec_env, rats_sgx_evidence_t *evidence,
                       uint32_t evidence_size, const char *buffer,
                       uint32_t buffer_size)
{
    attestation_evidence_t att_ev;

    char final_hash[SHA256_DIGEST_LENGTH] = { '\0' };
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, wasm_module_hash, SHA256_DIGEST_LENGTH);
    if (buffer != NULL)
        SHA256_Update(&sha256, buffer, strlen(buffer));
    SHA256_Final((unsigned char *)final_hash, &sha256);

    const char *tee_type = "sgx_ecdsa";
    bh_memcpy_s(att_ev.type, strlen(tee_type), tee_type, strlen(tee_type));
    bh_memcpy_s(att_ev.ecdsa.quote, evidence->quote_size, evidence->quote,
                evidence->quote_size);
    att_ev.ecdsa.quote_len = evidence->quote_size;

    rats_verifier_err_t ret_code = librats_verify_evidence(
        &att_ev, (const uint8_t *)final_hash, NULL, NULL);
    return ret_code == RATS_VERIFIER_ERR_NONE ? 0 : (int)ret_code;
}

/* clang-format off */
#define REG_NATIVE_FUNC(func_name, signature) \
    { #func_name, func_name##_wrapper, signature, NULL }
/* clang-format on */

static NativeSymbol native_symbols_lib_rats[] = {
    REG_NATIVE_FUNC(librats_collect, "(*~*~)i"),
    REG_NATIVE_FUNC(librats_verify, "(*~*~)i")
};

uint32_t
get_lib_rats_export_apis(NativeSymbol **p_lib_rats_apis)
{
    *p_lib_rats_apis = native_symbols_lib_rats;
    return sizeof(native_symbols_lib_rats) / sizeof(NativeSymbol);
}