
/*
 * Copyright (C) 2025 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * THIS FILE IS GENERATED AUTOMATICALLY, DO NOT EDIT!
 */
#ifndef WASM_C_API_CHECKED_H
#define WASM_C_API_CHECKED_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "wasm_c_api.h"

typedef struct {
    int error_code; // Error code (0 for success, non-zero for errors)
    union {
        wasm_valkind_t wasm_valkind_t_value;
        wasm_mutability_t wasm_mutability_t_value;
        uint32_t uint32_t_value;
        wasm_table_size_t wasm_table_size_t_value;
        _Bool _Bool_value;
        double double_value;
        wasm_externkind_t wasm_externkind_t_value;
        wasm_memory_pages_t wasm_memory_pages_t_value;
        int int_value;
        size_t size_t_value;
        // Add other types as needed
    } value;
} Result;

static inline Result
memcpy_checked(void *__dest, void *__src, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __dest
    if (__dest == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __src
    if (__src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    memcpy(__dest, __src, __n);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
memmove_checked(void *__dest, void *__src, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __dest
    if (__dest == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __src
    if (__src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    memmove(__dest, __src, __n);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
memccpy_checked(void *__dest, void *__src, int __c, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __dest
    if (__dest == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __src
    if (__src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    memccpy(__dest, __src, __c, __n);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
memset_checked(void *__s, int __c, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    memset(__s, __c, __n);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
memcmp_checked(void *__s1, void *__s2, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __s1
    if (__s1 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __s2
    if (__s2 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    int original_result = memcmp(__s1, __s2, __n);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
__memcmpeq_checked(void *__s1, void *__s2, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __s1
    if (__s1 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __s2
    if (__s2 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    int original_result = __memcmpeq(__s1, __s2, __n);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
memchr_checked(void *__s, int __c, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    memchr(__s, __c, __n);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strcpy_checked(void *__dest, void *__src)
{
    Result res;
    // Check for null pointer parameter: __dest
    if (__dest == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __src
    if (__src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    strcpy(__dest, __src);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strncpy_checked(void *__dest, void *__src, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __dest
    if (__dest == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __src
    if (__src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    strncpy(__dest, __src, __n);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strcat_checked(void *__dest, void *__src)
{
    Result res;
    // Check for null pointer parameter: __dest
    if (__dest == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __src
    if (__src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    strcat(__dest, __src);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strncat_checked(void *__dest, void *__src, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __dest
    if (__dest == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __src
    if (__src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    strncat(__dest, __src, __n);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strcmp_checked(void *__s1, void *__s2)
{
    Result res;
    // Check for null pointer parameter: __s1
    if (__s1 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __s2
    if (__s2 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    int original_result = strcmp(__s1, __s2);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strncmp_checked(void *__s1, void *__s2, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __s1
    if (__s1 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __s2
    if (__s2 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    int original_result = strncmp(__s1, __s2, __n);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strcoll_checked(void *__s1, void *__s2)
{
    Result res;
    // Check for null pointer parameter: __s1
    if (__s1 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __s2
    if (__s2 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    int original_result = strcoll(__s1, __s2);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strxfrm_checked(void *__dest, void *__src, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __dest
    if (__dest == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __src
    if (__src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    size_t original_result = strxfrm(__dest, __src, __n);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.size_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strcoll_l_checked(void *__s1, void *__s2, locale_t __l)
{
    Result res;
    // Check for null pointer parameter: __s1
    if (__s1 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __s2
    if (__s2 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    int original_result = strcoll_l(__s1, __s2, __l);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strxfrm_l_checked(void *__dest, void *__src, size_t __n, locale_t __l)
{
    Result res;
    // Check for null pointer parameter: __dest
    if (__dest == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __src
    if (__src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    size_t original_result = strxfrm_l(__dest, __src, __n, __l);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.size_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strdup_checked(void *__s)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    strdup(__s);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strndup_checked(void *__string, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __string
    if (__string == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    strndup(__string, __n);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strchr_checked(void *__s, int __c)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    strchr(__s, __c);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strrchr_checked(void *__s, int __c)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    strrchr(__s, __c);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strcspn_checked(void *__s, void *__reject)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __reject
    if (__reject == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    size_t original_result = strcspn(__s, __reject);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.size_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strspn_checked(void *__s, void *__accept)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __accept
    if (__accept == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    size_t original_result = strspn(__s, __accept);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.size_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strpbrk_checked(void *__s, void *__accept)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __accept
    if (__accept == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    strpbrk(__s, __accept);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strstr_checked(void *__haystack, void *__needle)
{
    Result res;
    // Check for null pointer parameter: __haystack
    if (__haystack == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __needle
    if (__needle == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    strstr(__haystack, __needle);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strtok_checked(void *__s, void *__delim)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __delim
    if (__delim == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    strtok(__s, __delim);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
__strtok_r_checked(void *__s, void *__delim, void *__save_ptr)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __delim
    if (__delim == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __save_ptr
    if (__save_ptr == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    __strtok_r(__s, __delim, __save_ptr);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strtok_r_checked(void *__s, void *__delim, void *__save_ptr)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __delim
    if (__delim == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __save_ptr
    if (__save_ptr == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    strtok_r(__s, __delim, __save_ptr);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strlen_checked(void *__s)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    size_t original_result = strlen(__s);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.size_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strnlen_checked(void *__string, size_t __maxlen)
{
    Result res;
    // Check for null pointer parameter: __string
    if (__string == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    size_t original_result = strnlen(__string, __maxlen);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.size_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strerror_checked(int __errnum)
{
    Result res;
    // Execute the original function
    strerror(__errnum);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strerror_r_checked(int __errnum, void *__buf, size_t __buflen)
{
    Result res;
    // Check for null pointer parameter: __buf
    if (__buf == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    int original_result = strerror_r(__errnum, __buf, __buflen);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strerror_l_checked(int __errnum, locale_t __l)
{
    Result res;
    // Execute the original function
    strerror_l(__errnum, __l);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
bcmp_checked(void *__s1, void *__s2, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __s1
    if (__s1 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __s2
    if (__s2 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    int original_result = bcmp(__s1, __s2, __n);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
bcopy_checked(void *__src, void *__dest, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __src
    if (__src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __dest
    if (__dest == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    bcopy(__src, __dest, __n);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
bzero_checked(void *__s, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    bzero(__s, __n);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
index_checked(void *__s, int __c)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    index(__s, __c);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
rindex_checked(void *__s, int __c)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    rindex(__s, __c);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
ffs_checked(int __i)
{
    Result res;
    // Execute the original function
    int original_result = ffs(__i);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
ffsl_checked(long int __l)
{
    Result res;
    // Execute the original function
    int original_result = ffsl(__l);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
ffsll_checked(long long int __ll)
{
    Result res;
    // Execute the original function
    int original_result = ffsll(__ll);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strcasecmp_checked(void *__s1, void *__s2)
{
    Result res;
    // Check for null pointer parameter: __s1
    if (__s1 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __s2
    if (__s2 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    int original_result = strcasecmp(__s1, __s2);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strncasecmp_checked(void *__s1, void *__s2, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __s1
    if (__s1 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __s2
    if (__s2 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    int original_result = strncasecmp(__s1, __s2, __n);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strcasecmp_l_checked(void *__s1, void *__s2, locale_t __loc)
{
    Result res;
    // Check for null pointer parameter: __s1
    if (__s1 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __s2
    if (__s2 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    int original_result = strcasecmp_l(__s1, __s2, __loc);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
strncasecmp_l_checked(void *__s1, void *__s2, size_t __n, locale_t __loc)
{
    Result res;
    // Check for null pointer parameter: __s1
    if (__s1 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __s2
    if (__s2 == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    int original_result = strncasecmp_l(__s1, __s2, __n, __loc);
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.int_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
explicit_bzero_checked(void *__s, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __s
    if (__s == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    explicit_bzero(__s, __n);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strsep_checked(void *__stringp, void *__delim)
{
    Result res;
    // Check for null pointer parameter: __stringp
    if (__stringp == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __delim
    if (__delim == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    strsep(__stringp, __delim);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
strsignal_checked(int __sig)
{
    Result res;
    // Execute the original function
    strsignal(__sig);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
__stpcpy_checked(void *__dest, void *__src)
{
    Result res;
    // Check for null pointer parameter: __dest
    if (__dest == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __src
    if (__src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    __stpcpy(__dest, __src);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
stpcpy_checked(void *__dest, void *__src)
{
    Result res;
    // Check for null pointer parameter: __dest
    if (__dest == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __src
    if (__src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    stpcpy(__dest, __src);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
__stpncpy_checked(void *__dest, void *__src, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __dest
    if (__dest == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __src
    if (__src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    __stpncpy(__dest, __src, __n);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
stpncpy_checked(void *__dest, void *__src, size_t __n)
{
    Result res;
    // Check for null pointer parameter: __dest
    if (__dest == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __src
    if (__src == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    stpncpy(__dest, __src, __n);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
__assert_fail_checked(void *__assertion, void *__file, unsigned int __line,
                      void *__function)
{
    Result res;
    // Check for null pointer parameter: __assertion
    if (__assertion == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __file
    if (__file == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __function
    if (__function == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    __assert_fail(__assertion, __file, __line, __function);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
__assert_perror_fail_checked(int __errnum, void *__file, unsigned int __line,
                             void *__function)
{
    Result res;
    // Check for null pointer parameter: __file
    if (__file == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __function
    if (__function == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    __assert_perror_fail(__errnum, __file, __line, __function);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
__assert_checked(void *__assertion, void *__file, int __line)
{
    Result res;
    // Check for null pointer parameter: __assertion
    if (__assertion == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: __file
    if (__file == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    __assert(__assertion, __file, __line);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_byte_vec_new_empty_checked(void *out)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_byte_vec_new_empty(out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_byte_vec_new_uninitialized_checked(void *out, size_t)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_byte_vec_new_uninitialized(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_byte_vec_new_checked(void *out, size_t, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_byte_vec_new(out, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_byte_vec_copy_checked(void *out, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_byte_vec_copy(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_byte_vec_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_byte_vec_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_config_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_config_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_config_new_checked(void)
{
    Result res;
    // Execute the original function
    wasm_config_new();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_config_set_mem_alloc_opt_checked(void *, mem_alloc_type_t, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_config_set_mem_alloc_opt(, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_config_set_linux_perf_opt_checked(void *, _Bool)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_config_set_linux_perf_opt(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_config_set_segue_flags_checked(void *config, uint32_t segue_flags)
{
    Result res;
    // Check for null pointer parameter: config
    if (config == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_config_set_segue_flags(config, segue_flags);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_engine_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_engine_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_engine_new_checked(void)
{
    Result res;
    // Execute the original function
    wasm_engine_new();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_engine_new_with_config_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_engine_new_with_config();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_engine_new_with_args_checked(mem_alloc_type_t type, void *opts)
{
    Result res;
    // Check for null pointer parameter: opts
    if (opts == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_engine_new_with_args(type, opts);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_store_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_store_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_store_new_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_store_new();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_valtype_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_valtype_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_valtype_vec_new_empty_checked(void *out)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_valtype_vec_new_empty(out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_valtype_vec_new_uninitialized_checked(void *out, size_t)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_valtype_vec_new_uninitialized(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_valtype_vec_new_checked(void *out, size_t, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_valtype_vec_new(out, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_valtype_vec_copy_checked(void *out, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_valtype_vec_copy(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_valtype_vec_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_valtype_vec_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_valtype_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_valtype_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_valtype_new_checked(wasm_valkind_t)
{
    Result res;
    // Execute the original function
    wasm_valtype_new();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_valtype_kind_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_valkind_t original_result = wasm_valtype_kind();
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_valkind_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_functype_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_functype_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_functype_vec_new_empty_checked(void *out)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_functype_vec_new_empty(out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_functype_vec_new_uninitialized_checked(void *out, size_t)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_functype_vec_new_uninitialized(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_functype_vec_new_checked(void *out, size_t, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_functype_vec_new(out, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_functype_vec_copy_checked(void *out, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_functype_vec_copy(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_functype_vec_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_functype_vec_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_functype_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_functype_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_functype_new_checked(void *params, void *results)
{
    Result res;
    // Check for null pointer parameter: params
    if (params == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: results
    if (results == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_functype_new(params, results);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_functype_params_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_functype_params();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_functype_results_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_functype_results();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_globaltype_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_globaltype_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_globaltype_vec_new_empty_checked(void *out)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_globaltype_vec_new_empty(out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_globaltype_vec_new_uninitialized_checked(void *out, size_t)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_globaltype_vec_new_uninitialized(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_globaltype_vec_new_checked(void *out, size_t, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_globaltype_vec_new(out, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_globaltype_vec_copy_checked(void *out, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_globaltype_vec_copy(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_globaltype_vec_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_globaltype_vec_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_globaltype_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_globaltype_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_globaltype_new_checked(void *, wasm_mutability_t)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_globaltype_new(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_globaltype_content_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_globaltype_content();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_globaltype_mutability_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_mutability_t original_result = wasm_globaltype_mutability();
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_mutability_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_tabletype_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_tabletype_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_tabletype_vec_new_empty_checked(void *out)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_tabletype_vec_new_empty(out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_tabletype_vec_new_uninitialized_checked(void *out, size_t)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_tabletype_vec_new_uninitialized(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_tabletype_vec_new_checked(void *out, size_t, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_tabletype_vec_new(out, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_tabletype_vec_copy_checked(void *out, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_tabletype_vec_copy(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_tabletype_vec_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_tabletype_vec_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_tabletype_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_tabletype_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_tabletype_new_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_tabletype_new(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_tabletype_element_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_tabletype_element();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_tabletype_limits_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_tabletype_limits();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memorytype_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memorytype_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memorytype_vec_new_empty_checked(void *out)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memorytype_vec_new_empty(out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memorytype_vec_new_uninitialized_checked(void *out, size_t)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memorytype_vec_new_uninitialized(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memorytype_vec_new_checked(void *out, size_t, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memorytype_vec_new(out, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memorytype_vec_copy_checked(void *out, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memorytype_vec_copy(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memorytype_vec_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memorytype_vec_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memorytype_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memorytype_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memorytype_new_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memorytype_new();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memorytype_limits_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memorytype_limits();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_vec_new_empty_checked(void *out)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_vec_new_empty(out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_vec_new_uninitialized_checked(void *out, size_t)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_vec_new_uninitialized(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_vec_new_checked(void *out, size_t, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_vec_new(out, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_vec_copy_checked(void *out, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_vec_copy(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_vec_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_vec_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_kind_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externkind_t original_result = wasm_externtype_kind();
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_externkind_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_functype_as_externtype_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_functype_as_externtype();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_globaltype_as_externtype_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_globaltype_as_externtype();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_tabletype_as_externtype_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_tabletype_as_externtype();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memorytype_as_externtype_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memorytype_as_externtype();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_as_functype_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_as_functype();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_as_globaltype_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_as_globaltype();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_as_tabletype_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_as_tabletype();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_as_memorytype_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_as_memorytype();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_functype_as_externtype_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_functype_as_externtype_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_globaltype_as_externtype_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_globaltype_as_externtype_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_tabletype_as_externtype_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_tabletype_as_externtype_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memorytype_as_externtype_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memorytype_as_externtype_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_as_functype_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_as_functype_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_as_globaltype_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_as_globaltype_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_as_tabletype_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_as_tabletype_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_externtype_as_memorytype_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externtype_as_memorytype_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_importtype_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_importtype_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_importtype_vec_new_empty_checked(void *out)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_importtype_vec_new_empty(out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_importtype_vec_new_uninitialized_checked(void *out, size_t)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_importtype_vec_new_uninitialized(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_importtype_vec_new_checked(void *out, size_t, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_importtype_vec_new(out, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_importtype_vec_copy_checked(void *out, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_importtype_vec_copy(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_importtype_vec_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_importtype_vec_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_importtype_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_importtype_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_importtype_new_checked(void *module, void *name, void *)
{
    Result res;
    // Check for null pointer parameter: module
    if (module == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: name
    if (name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_importtype_new(module, name, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_importtype_module_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_importtype_module();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_importtype_name_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_importtype_name();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_importtype_type_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_importtype_type();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_importtype_is_linked_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_importtype_is_linked();
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_exporttype_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_exporttype_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_exporttype_vec_new_empty_checked(void *out)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_exporttype_vec_new_empty(out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_exporttype_vec_new_uninitialized_checked(void *out, size_t)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_exporttype_vec_new_uninitialized(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_exporttype_vec_new_checked(void *out, size_t, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_exporttype_vec_new(out, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_exporttype_vec_copy_checked(void *out, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_exporttype_vec_copy(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_exporttype_vec_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_exporttype_vec_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_exporttype_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_exporttype_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_exporttype_new_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_exporttype_new(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_exporttype_name_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_exporttype_name();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_exporttype_type_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_exporttype_type();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_val_delete_checked(void *v)
{
    Result res;
    // Check for null pointer parameter: v
    if (v == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_val_delete(v);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_val_copy_checked(void *out, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_val_copy(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_val_vec_new_empty_checked(void *out)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_val_vec_new_empty(out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_val_vec_new_uninitialized_checked(void *out, size_t)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_val_vec_new_uninitialized(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_val_vec_new_checked(void *out, size_t, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_val_vec_new(out, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_val_vec_copy_checked(void *out, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_val_vec_copy(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_val_vec_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_val_vec_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_same_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_ref_same(, );
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_ref_get_host_info_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_get_host_info();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_set_host_info_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_set_host_info(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_set_host_info_with_finalizer_checked(void *, void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_set_host_info_with_finalizer(, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_frame_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_frame_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_frame_vec_new_empty_checked(void *out)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_frame_vec_new_empty(out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_frame_vec_new_uninitialized_checked(void *out, size_t)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_frame_vec_new_uninitialized(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_frame_vec_new_checked(void *out, size_t, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_frame_vec_new(out, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_frame_vec_copy_checked(void *out, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_frame_vec_copy(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_frame_vec_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_frame_vec_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_frame_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_frame_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_frame_instance_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_frame_instance();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_frame_func_index_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    uint32_t original_result = wasm_frame_func_index();
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.uint32_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_frame_func_offset_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    size_t original_result = wasm_frame_func_offset();
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.size_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_frame_module_offset_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    size_t original_result = wasm_frame_module_offset();
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.size_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_trap_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_trap_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_trap_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_trap_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_trap_same_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_trap_same(, );
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_trap_get_host_info_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_trap_get_host_info();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_trap_set_host_info_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_trap_set_host_info(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_trap_set_host_info_with_finalizer_checked(void *, void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_trap_set_host_info_with_finalizer(, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_trap_as_ref_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_trap_as_ref();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_trap_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_trap();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_trap_as_ref_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_trap_as_ref_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_trap_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_trap_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_trap_new_checked(void *store, void *)
{
    Result res;
    // Check for null pointer parameter: store
    if (store == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_trap_new(store, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_trap_message_checked(void *, void *out)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_trap_message(, out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_trap_origin_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_trap_origin();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_trap_trace_checked(void *, void *out)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_trap_trace(, out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_foreign_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_foreign_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_foreign_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_foreign_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_foreign_same_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_foreign_same(, );
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_foreign_get_host_info_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_foreign_get_host_info();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_foreign_set_host_info_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_foreign_set_host_info(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_foreign_set_host_info_with_finalizer_checked(void *, void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_foreign_set_host_info_with_finalizer(, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_foreign_as_ref_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_foreign_as_ref();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_foreign_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_foreign();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_foreign_as_ref_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_foreign_as_ref_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_foreign_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_foreign_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_foreign_new_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_foreign_new();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_module_new_checked(void *, void *binary)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: binary
    if (binary == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_new(, binary);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_module_new_ex_checked(void *, void *binary, void *args)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: binary
    if (binary == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: args
    if (args == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_new_ex(, binary, args);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_module_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_module_validate_checked(void *, void *binary)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: binary
    if (binary == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_module_validate(, binary);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_module_imports_checked(void *, void *out)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_imports(, out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_module_exports_checked(void *, void *out)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_exports(, out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_module_serialize_checked(void *, void *out)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_serialize(, out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_module_deserialize_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_deserialize(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_module_share_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_share();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_module_obtain_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_obtain(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_shared_module_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_shared_module_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_module_set_name_checked(void *, void *name)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: name
    if (name == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_module_set_name(, name);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_module_get_name_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_module_get_name();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_module_is_underlying_binary_freeable_checked(void *module)
{
    Result res;
    // Check for null pointer parameter: module
    if (module == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_module_is_underlying_binary_freeable(module);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_func_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_same_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_func_same(, );
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_func_get_host_info_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_get_host_info();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_set_host_info_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_set_host_info(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_set_host_info_with_finalizer_checked(void *, void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_set_host_info_with_finalizer(, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_as_ref_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_as_ref();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_func_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_func();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_as_ref_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_as_ref_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_func_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_func_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_new_checked(void *, void *, wasm_func_callback_t)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_new(, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_new_with_env_checked(void *, void *type,
                               wasm_func_callback_with_env_t, void *env,
                               void *finalizer)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: type
    if (type == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: env
    if (env == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: finalizer
    if (finalizer == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_new_with_env(, type, , env, finalizer);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_type_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_type();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_param_arity_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    size_t original_result = wasm_func_param_arity();
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.size_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_func_result_arity_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    size_t original_result = wasm_func_result_arity();
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.size_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_func_call_checked(void *, void *args, void *results)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: args
    if (args == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: results
    if (results == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_call(, args, results);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_global_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_global_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_global_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_global_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_global_same_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_global_same(, );
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_global_get_host_info_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_global_get_host_info();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_global_set_host_info_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_global_set_host_info(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_global_set_host_info_with_finalizer_checked(void *, void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_global_set_host_info_with_finalizer(, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_global_as_ref_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_global_as_ref();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_global_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_global();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_global_as_ref_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_global_as_ref_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_global_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_global_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_global_new_checked(void *, void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_global_new(, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_global_type_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_global_type();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_global_get_checked(void *, void *out)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_global_get(, out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_global_set_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_global_set(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_table_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_table_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_table_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_table_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_table_same_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_table_same(, );
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_table_get_host_info_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_table_get_host_info();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_table_set_host_info_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_table_set_host_info(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_table_set_host_info_with_finalizer_checked(void *, void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_table_set_host_info_with_finalizer(, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_table_as_ref_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_table_as_ref();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_table_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_table();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_table_as_ref_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_table_as_ref_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_table_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_table_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_table_new_checked(void *, void *, void *init)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: init
    if (init == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_table_new(, , init);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_table_type_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_table_type();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_table_get_checked(void *, wasm_table_size_t index)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_table_get(, index);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_table_set_checked(void *, wasm_table_size_t index, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_table_set(, index, );
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_table_size_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_table_size_t original_result = wasm_table_size();
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_table_size_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_table_grow_checked(void *, wasm_table_size_t delta, void *init)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: init
    if (init == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_table_grow(, delta, init);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_memory_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memory_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memory_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memory_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memory_same_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_memory_same(, );
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_memory_get_host_info_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memory_get_host_info();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memory_set_host_info_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memory_set_host_info(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memory_set_host_info_with_finalizer_checked(void *, void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memory_set_host_info_with_finalizer(, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memory_as_ref_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memory_as_ref();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_memory_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_memory();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memory_as_ref_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memory_as_ref_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_memory_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_memory_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memory_new_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memory_new(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memory_type_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memory_type();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memory_data_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memory_data();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memory_data_size_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    size_t original_result = wasm_memory_data_size();
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.size_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_memory_size_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memory_pages_t original_result = wasm_memory_size();
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_memory_pages_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_memory_grow_checked(void *, wasm_memory_pages_t delta)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_memory_grow(, delta);
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_extern_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_same_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_extern_same(, );
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_extern_get_host_info_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_get_host_info();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_set_host_info_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_set_host_info(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_set_host_info_with_finalizer_checked(void *, void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_set_host_info_with_finalizer(, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_as_ref_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_as_ref();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_extern_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_extern();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_as_ref_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_as_ref_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_extern_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_extern_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_vec_new_empty_checked(void *out)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_vec_new_empty(out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_vec_new_uninitialized_checked(void *out, size_t)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_vec_new_uninitialized(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_vec_new_checked(void *out, size_t, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_vec_new(out, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_vec_copy_checked(void *out, void *)
{
    Result res;
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_vec_copy(out, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_vec_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_vec_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_kind_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_externkind_t original_result = wasm_extern_kind();
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.wasm_externkind_t_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_extern_type_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_type();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_as_extern_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_as_extern();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_global_as_extern_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_global_as_extern();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_table_as_extern_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_table_as_extern();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memory_as_extern_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memory_as_extern();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_as_func_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_as_func();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_as_global_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_as_global();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_as_table_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_as_table();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_as_memory_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_as_memory();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_func_as_extern_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_func_as_extern_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_global_as_extern_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_global_as_extern_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_table_as_extern_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_table_as_extern_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_memory_as_extern_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_memory_as_extern_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_as_func_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_as_func_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_as_global_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_as_global_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_as_table_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_as_table_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_extern_as_memory_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_as_memory_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_instance_delete_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_instance_delete();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_instance_copy_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_instance_copy();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_instance_same_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    _Bool original_result = wasm_instance_same(, );
    // Assign return value and error code
    res.error_code = original_result ? 0 : -2;
    res.value._Bool_value = original_result;
    return res;
}

static inline Result
wasm_instance_get_host_info_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_instance_get_host_info();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_instance_set_host_info_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_instance_set_host_info(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_instance_set_host_info_with_finalizer_checked(void *, void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_instance_set_host_info_with_finalizer(, , );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_instance_as_ref_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_instance_as_ref();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_instance_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_instance();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_instance_as_ref_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_instance_as_ref_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_ref_as_instance_const_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_ref_as_instance_const();
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_instance_new_checked(void *, void *, void *imports, void *trap)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: imports
    if (imports == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: trap
    if (trap == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_instance_new(, , imports, trap);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_instance_new_with_args_checked(void *, void *, void *imports, void *trap,
                                    uint32_t stack_size, uint32_t heap_size)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: imports
    if (imports == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: trap
    if (trap == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_instance_new_with_args(, , imports, trap, stack_size, heap_size);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_instance_new_with_args_ex_checked(void *, void *, void *imports,
                                       void *trap, void *inst_args)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: imports
    if (imports == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: trap
    if (trap == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: inst_args
    if (inst_args == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_instance_new_with_args_ex(, , imports, trap, inst_args);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_instance_exports_checked(void *, void *out)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: out
    if (out == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_instance_exports(, out);
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

static inline Result
wasm_instance_sum_wasm_exec_time_checked(void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    double original_result = wasm_instance_sum_wasm_exec_time();
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.double_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_instance_get_wasm_func_exec_time_checked(void *, void *)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    double original_result = wasm_instance_get_wasm_func_exec_time(, );
    // Assign return value and error code
    if (original_result == 0) {
        res.error_code = 0;
        res.value.double_value = original_result;
    }
    else {
        res.error_code = -2;
    }
    return res;
}

static inline Result
wasm_extern_new_empty_checked(void *, wasm_externkind_t)
{
    Result res;
    // Check for null pointer parameter: None
    if (None == NULL) {
        res.error_code = -1;
        return res;
    }
    // Execute the original function
    wasm_extern_new_empty(, );
    // Assign return value and error code
    res.error_code = 0;
    return res;
}

#endif // WASM_C_API_CHECKED_H
