/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
/**
 * @file   bh_log.c
 * @date   Tue Nov  8 18:20:06 2011
 *
 * @brief Implementation of Beihai's log system.
 */

#include "bh_assert.h"
#include "bh_time.h"
#include "bh_thread.h"
#include "bh_log.h"
#include "bh_platform_log.h"

/**
 * The verbose level of the log system.  Only those verbose logs whose
 * levels are less than or equal to this value are outputed.
 */
static int log_verbose_level;

/**
 * The lock for protecting the global output stream of logs.
 */
static korp_mutex log_stream_lock;

/**
 * The current logging thread that owns the log_stream_lock.
 */
static korp_tid cur_logging_thread = INVALID_THREAD_ID;

/**
 * Whether the currently being printed log is ennabled.
 */
static bool cur_log_enabled;

int _bh_log_init()
{
    log_verbose_level = 3;
    return vm_mutex_init(&log_stream_lock);
}

void _bh_log_set_verbose_level(int level)
{
    log_verbose_level = level;
}

void _bh_log_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _bh_log_vprintf(fmt, ap);
    va_end(ap);
}

/**
 * Return true if the given tag is enabled by the configuration.
 *
 * @param tag the tag (or prefix) of the log message.
 *
 * @return true if the log should be enabled.
 */
static bool is_log_enabled(const char *tag)
{
    /* Print all non-verbose or verbose logs whose levels are less than
     or equal to the configured verbose level.  */
    return tag[0] != 'V' || tag[1] - '0' <= log_verbose_level;
}

/**
 * Helper function for converting "..." to va_list.
 */
static void bh_log_emit_helper(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    bh_log_emit(fmt, ap);
    va_end(ap);
}

extern size_t _bh_time_strftime(char *s, size_t max, const char *format,
        int64 time);

void _bh_log_vprintf(const char *fmt, va_list ap)
{
    korp_tid self = vm_self_thread();
    /* Try to own the log stream and start the log output.  */
    if (self != cur_logging_thread) {
        vm_mutex_lock(&log_stream_lock);
        cur_logging_thread = self;

        if (fmt && (cur_log_enabled = is_log_enabled(fmt))) {
            char buf[32];
            bh_time_strftime(buf, 32, "%Y-%m-%d %H:%M:%S",
            bh_time_get_millisecond_from_1970());
            bh_log_emit_helper("\n[%s - %X]: ", buf, (int) self);

            /* Strip the "Vn." prefix.  */
            if (fmt && strlen(fmt) >= 3 && fmt[0] == 'V' && fmt[1] && fmt[2])
                fmt += 3;
        }
    }

    if (cur_log_enabled && fmt)
        bh_log_emit(fmt, ap);
}

void _bh_log_commit()
{
    if (vm_self_thread() != cur_logging_thread)
        /* Ignore the single commit without printing anything.  */
        return;

    cur_logging_thread = INVALID_THREAD_ID;
    vm_mutex_unlock(&log_stream_lock);
}

void _bh_log(const char *tag, const char *file, int line, const char *fmt, ...)
{
    va_list ap;

    if (tag)
        _bh_log_printf(tag);

    if (file)
        _bh_log_printf("%s:%d", file, line);

    va_start(ap, fmt);
    _bh_log_vprintf(fmt, ap);
    va_end(ap);

    _bh_log_commit();
}

#if defined(BH_DEBUG)

BH_STATIC char com_switchs[LOG_COM_MAX]; /* 0: off; 1: on */
BH_STATIC char *com_names[LOG_COM_MAX] = {"app_manager", "gc", "hmc", "utils",
    "verifier_jeff", "vmcore_jeff"};

BH_STATIC int com_find_name(const char **com)
{
    int i;
    const char *c, *name;

    for (i = 0; i < LOG_COM_MAX; i++) {
        c = *com;
        name = com_names[i];
        while (*name != '\0') {
            if (*c != *name)
            break;
            c++;
            name++;
        }
        if (*name == '\0') {
            *com = c;
            return i;
        }
    }

    return LOG_COM_MAX;
}

void log_parse_coms(const char *coms)
{
    int i;

    for (i = LOG_COM_MAX; i--; )
    com_switchs[i] = 0;

    com_switchs[LOG_COM_UTILS] = 1; /* utils is a common part */

    if (coms == NULL)
    return;

    while (*coms != '\0') {
        i = com_find_name(&coms);
        if (i == LOG_COM_MAX)
        break;
        com_switchs[i] = 1;

        if (*coms == '\0')
        return;
        if (*coms != ';')
        break;
        /* *com == ';' */
        coms++;
    }

    /* Log the message without aborting. */
    LOG_DEBUG(LOG_COM_UTILS, "The component names for logging are not right: %s.", coms);
}

int bh_log_dcom_is_enabled(int component)
{
    return com_switchs[component];
}

#endif /* defined(BH_DEBUG) */

