/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/**
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

#ifndef _WASM_LOG_H
#define _WASM_LOG_H

#include "bh_platform.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * The following functions are the primitive operations of this log system.
 * A normal usage of the log system is to call wasm_log_begin and then call
 * wasm_log_printf or wasm_log_vprintf one or multiple times and then call
 * wasm_log_end to wrap (mark) the previous outputs into one log message.
 * The wasm_log and macros LOG_ERROR etc. can be used to output log messages
 * by one log calling.
 */
int  _wasm_log_init (void);
void _wasm_log_set_verbose_level (int level);
bool _wasm_log_begin (int level);
void _wasm_log_printf (const char *fmt, ...);
void _wasm_log_vprintf (const char *fmt, va_list ap);
void _wasm_log_end (void);
void _wasm_log (int level, const char *file, int line,
                const char *fmt, ...);

#if WASM_ENABLE_LOG != 0
# define wasm_log_init()               _wasm_log_init ()
# define wasm_log_set_verbose_level(l) _wasm_log_set_verbose_level (l)
# define wasm_log_begin(l)             _wasm_log_begin (l)
# define wasm_log_printf(...)          _wasm_log_printf (__VA_ARGS__)
# define wasm_log_vprintf(...)         _wasm_log_vprintf (__VA_ARGS__)
# define wasm_log_end()                _wasm_log_end ()
# define wasm_log(...)                 _wasm_log (__VA_ARGS__)
#else  /* WASM_ENABLE_LOG != 0 */
# define wasm_log_init()               0
# define wasm_log_set_verbose_level(l) (void)0
# define wasm_log_begin()              false
# define wasm_log_printf(...)          (void)0
# define wasm_log_vprintf(...)         (void)0
# define wasm_log_end()                (void)0
# define wasm_log(...)                 (void)0
#endif  /* WASM_ENABLE_LOG != 0 */

#define LOG_ERROR(...)          wasm_log (0, NULL, 0, __VA_ARGS__)
#define LOG_WARNING(...)        wasm_log (1, NULL, 0, __VA_ARGS__)
#define LOG_VERBOSE(...)        wasm_log (2, NULL, 0, __VA_ARGS__)

#if defined(WASM_DEBUG)
# define LOG_DEBUG(...)         _wasm_log (1, __FILE__, __LINE__, __VA_ARGS__)
#else  /* defined(WASM_DEBUG) */
# define LOG_DEBUG(...)         (void)0
#endif  /* defined(WASM_DEBUG) */

#define LOG_PROFILE_HEAP_GC(heap, size)                         \
  LOG_VERBOSE("PROF.HEAP.GC: HEAP=%08X SIZE=%d", heap, size)

#ifdef __cplusplus
}
#endif


#endif  /* _WASM_LOG_H */
