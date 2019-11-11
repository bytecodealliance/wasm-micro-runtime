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

#include <stdarg.h>

#include "bh_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The following functions are the primitive operations of this log
 * system.  A normal usage of the log system is to call bh_log_printf
 * or bh_log_vprintf one or multiple times and then call bh_log_commit
 * to wrap (mark) the previous outputs into one log message.  The
 * bh_log and macros LOG_ERROR etc. can be used to output log messages
 * that can be wrapped into one log calling.
 */
int _bh_log_init(void);
void _bh_log_set_verbose_level(int level);
void _bh_log_printf(const char *fmt, ...);
void _bh_log_vprintf(const char *fmt, va_list ap);
void _bh_log_commit(void);

#if BEIHAI_ENABLE_LOG != 0
# define bh_log_init()               _bh_log_init ()
# define bh_log_set_verbose_level(l) _bh_log_set_verbose_level (l)
# define bh_log_printf(...)          _bh_log_printf (__VA_ARGS__)
# define bh_log_vprintf(...)         _bh_log_vprintf (__VA_ARGS__)
# define bh_log_commit()             _bh_log_commit ()
#else  /* BEIHAI_ENABLE_LOG != 0 */
# define bh_log_init()               0
# define bh_log_set_verbose_level(l) (void)0
# define bh_log_printf(...)          (void)0
# define bh_log_vprintf(...)         (void)0
# define bh_log_commit()             (void)0
#endif  /* BEIHAI_ENABLE_LOG != 0 */

void _bh_log(const char *tag, const char *file, int line, const char *fmt, ...);

/* Always print fatal message */
# define LOG_FATAL(...)         _bh_log ("V0.", NULL, 0, __VA_ARGS__)

#if BEIHAI_ENABLE_LOG != 0
# define LOG_ERROR(...)         _bh_log ("V1.", NULL, 0, __VA_ARGS__)
# define LOG_WARNING(...)       _bh_log ("V2.", NULL, 0, __VA_ARGS__)
# define LOG_INFO_RELEASE(...)  _bh_log ("V3.", NULL, 0, __VA_ARGS__)
# define LOG_INFO_APP_DEV(...)  _bh_log ("V4.", NULL, 0, __VA_ARGS__)
# define LOG_VERBOSE(...)       _bh_log ("V5.", NULL, 0, __VA_ARGS__)
# if BEIHAI_ENABLE_MEMORY_PROFILING != 0
#  define LOG_PROFILE(...)      _bh_log ("V3.", NULL, 0, __VA_ARGS__)
# else
#  define LOG_PROFILE(...)      (void)0
#endif
# if BEIHAI_ENABLE_QUEUE_PROFILING != 0
#  define LOG_QUEUE_PROFILE(...) _bh_log ("V3.", NULL, 0, __VA_ARGS__)
# else
#  define LOG_QUEUE_PROFILE(...) (void)0
#endif
# if BEIHAI_ENABLE_GC_STAT_PROFILING != 0
#  define LOG_GC_STAT_PROFILE(...) _bh_log ("V3.", NULL, 0, __VA_ARGS__)
# else
#  define LOG_GC_STAT_PROFILE(...) (void)0
#endif
#else  /* BEIHAI_ENABLE_LOG != 0 */
# define LOG_ERROR(...)         (void)0
# define LOG_WARNING(...)       (void)0
# define LOG_INFO_APP_DEV(...)  (void)0
# define LOG_INFO_RELEASE(...)  (void)0
# define LOG_VERBOSE(...)       (void)0
# define LOG_PROFILE(...)       (void)0
# define LOG_QUEUE_PROFILE(...) (void)0
# define LOG_GC_STAT_PROFILE(...) (void)0
#endif  /* BEIHAI_ENABLE_LOG != 0 */

#define LOG_PROFILE_INSTANCE_HEAP_CREATED(heap)                 \
  LOG_PROFILE ("PROF.INSTANCE.HEAP_CREATED: HEAP=%08X", heap)
#define LOG_PROFILE_INSTANCE_THREAD_STARTED(heap)                   \
  LOG_PROFILE ("PROF.INSTANCE.THREAD_STARTED: HEAP=%08X", heap)
#define LOG_PROFILE_HEAP_ALLOC(heap, size)                          \
  LOG_PROFILE ("PROF.HEAP.ALLOC: HEAP=%08X SIZE=%d", heap, size)
#define LOG_PROFILE_HEAP_FREE(heap, size)                           \
  LOG_PROFILE ("PROF.HEAP.FREE: HEAP=%08X SIZE=%d", heap, size)
#define LOG_PROFILE_HEAP_NEW(heap, size)                            \
  LOG_PROFILE ("PROF.HEAP.NEW: HEAP=%08X SIZE=%d", heap, size)
#define LOG_PROFILE_HEAP_GC(heap, size)                         \
  LOG_PROFILE ("PROF.HEAP.GC: HEAP=%08X SIZE=%d", heap, size)
#define LOG_PROFILE_STACK_PUSH(used)                \
  LOG_PROFILE ("PROF.STACK.PUSH: USED=%d", used)
#define LOG_PROFILE_STACK_POP()                 \
  LOG_PROFILE ("PROF.STACK.POP")

/* Please add your component ahead of LOG_COM_MAX */
enum {
    LOG_COM_APP_MANAGER = 0,
    LOG_COM_GC,
    LOG_COM_HMC,
    LOG_COM_UTILS,
    LOG_COM_VERIFIER_JEFF,
    LOG_COM_VMCORE_JEFF,
    LOG_COM_MAX
};

#if defined(BH_DEBUG)
void log_parse_coms(const char *coms);
int bh_log_dcom_is_enabled(int component);

#define LOG_DEBUG(component, ...) do {                          \
    if (bh_log_dcom_is_enabled (component))                     \
      _bh_log ("V6: ", __FILE__, __LINE__, __VA_ARGS__);        \
  } while (0)

#else  /* defined(BH_DEBUG) */

#define LOG_DEBUG(component, ...) (void)0

#endif  /* defined(BH_DEBUG) */

#ifdef __cplusplus
}
#endif

#endif  /* _BH_LOG_H */
