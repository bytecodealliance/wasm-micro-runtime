/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _PLATFORM_INTERNAL_H
#define _PLATFORM_INTERNAL_H

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <os_api.h>

#ifndef BH_PLATFORM_ESP_IDF
#define BH_PLATFORM_ESP_IDF
#endif

#define BH_APPLET_PRESERVED_STACK_SIZE (2 * BH_KB)

/* Default thread priority */
#define BH_THREAD_DEFAULT_PRIORITY 5

extern int errno;

typedef TaskHandle_t korp_thread;
typedef korp_thread korp_tid;
typedef struct {
    bool is_recursive;
    SemaphoreHandle_t sem;
} korp_mutex;

struct os_thread_wait_node;
typedef struct os_thread_wait_node *os_thread_wait_list;
typedef struct korp_cond {
    SemaphoreHandle_t wait_list_lock;
    os_thread_wait_list thread_wait_list;
} korp_cond;

int
os_printf(const char *format, ...);
int
os_vprintf(const char *format, va_list ap);

/* clang-format off */
/* math functions which are not provided by os */
double sqrt(double x);
double floor(double x);
double ceil(double x);
double fmin(double x, double y);
double fmax(double x, double y);
double rint(double x);
double fabs(double x);
double trunc(double x);
float floorf(float x);
float ceilf(float x);
float fminf(float x, float y);
float fmaxf(float x, float y);
float rintf(float x);
float truncf(float x);
int signbit(double x);
int isnan(double x);

int atoi(const char *s);
int strncasecmp(const char *s1, const char *s2, size_t n);
long int strtol(const char *str, char **endptr, int base);
unsigned long int strtoul(const char *str, char **endptr, int base);
unsigned long long int strtoull(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);
float strtof(const char *nptr, char **endptr);
char *strstr(const char *haystack, const char *needle);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
void *memchr(const void *s, int c, size_t n);
int isalnum(int c);
int isxdigit(int c);
int isdigit(int c);
int isprint(int c);
int isgraph(int c);
int isspace(int c);
int isalpha(int c);
int isupper(int c);
int toupper(int c);
int tolower(int c);
void *memmove(void *dest, const void *src, size_t n);

uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);
/* clang-format on */

#endif
