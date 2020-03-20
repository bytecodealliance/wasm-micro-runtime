/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
/**
 * @file   bh_log.h
 * @date   Tue Nov  8 18:19:10 2011
 *
 * @brief This log system supports wrapping multiple outputs into one
 * log message.  This is useful for outputting variable-length logs
 * without additional memory overhead (the buffer for concatenating
 * the message), e.g. exception stack trace, which cannot be printed
 * by a single log calling without the help of an additional buffer.
 * Avoiding additional memory buffer is useful for resource-constraint
 * systems.  It can minimize the impact of log system on applications
 * and logs can be printed even when no enough memory is available.
 * Functions with prefix "_" are private functions.  Only macros that
 * are not start with "_" are exposed and can be used.
 */

#ifndef _BH_LOG_H
#define _BH_LOG_H

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_DEBUG = 3,
    LOG_LEVEL_VERBOSE = 4
} LogLevel;

void
bh_log_set_verbose_level(uint32 level);

void
bh_log(LogLevel log_level, const char *file, int line, const char *fmt, ...);

#define LOG_FATAL(...)   bh_log(LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)   bh_log(LOG_LEVEL_ERROR, NULL, 0, __VA_ARGS__)
#define LOG_DEBUG(...)   bh_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, 0, __VA_ARGS__)
#define LOG_WARNING(...) bh_log(LOG_LEVEL_WARNING, NULL, 0, __VA_ARGS__)
#define LOG_VERBOSE(...) bh_log(LOG_LEVEL_VERBOSE, NULL, 0, __VA_ARGS__)

void
bh_print_time(const char *prompt);

#ifdef __cplusplus
}
#endif

#endif  /* _BH_LOG_H */
