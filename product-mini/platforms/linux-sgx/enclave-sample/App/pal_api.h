/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef __WAMR_PAL_API_H__
#define __WAMR_PAL_API_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * WAMR PAL API version number
 */
#define WAMR_PAL_VERSION 2

/*
 * @brief Get version of WAMR PAL API
 *
 * @retval If > 0, then success; otherwise, it is an invalid version.
 */
int wamr_pal_get_version(void);

/*
 * WAMR PAL attributes
 */
typedef struct wamr_pal_attr {
    // WAMR instance directory.
    //
    // The default value is "."; that is, the current working directory
    const char     *instance_dir;
    // Log level.
    //
    // Specifies the log verbose level (0 to 5, default is 2)
    // large level with more log
    //
    const char     *log_level;
} wamr_pal_attr_t;

#define WAMR_PAL_ATTR_INITVAL { \
    .instance_dir = ".",        \
    .log_level = 2              \
}

/*
 * The struct which consists of file descriptors of standard I/O
 */
typedef struct wamr_stdio_fds {
    int stdin_fd;
    int stdout_fd;
    int stderr_fd;
} wamr_stdio_fds_t;

/*
 * The struct which consists of arguments needed by wamr_pal_create_process
 */
struct wamr_pal_create_process_args {

    // Path to new process.
    //
    // The path of the command which will be created as a new process.
    //
    // Mandatory field. Must not be NULL.
    const char *path;

    // Argments array pass to new process.
    //
    // The arguments to the command. By convention, the argv[0] should be the program name.
    // And the last element of the array must be NULL to indicate the length of array.
    //
    // Mandatory field. Must not be NULL.
    const char **argv;

    // Untrusted environment variable array pass to new process.
    //
    // The untrusted env vars to the command. And the last element of the array must be
    // NULL to indicate the length of array.
    //
    // Optional field.
    const char **env;

    // File descriptors of the redirected standard I/O (i.e., stdin, stdout, stderr)
    //
    // If set to NULL, will use the original standard I/O file descriptors.
    //
    // Optional field.
    const struct wamr_stdio_fds *stdio;

    // Output. Pid of new process in libos.
    //
    // If wamr_pal_create_process returns success, pid of the new process will
    // be updated.
    //
    // Mandatory field. Must not be NULL.
    int *pid;
};

/*
 * The struct which consists of arguments needed by wamr_pal_exec
 */
struct wamr_pal_exec_args {
    // Pid of new process created with wamr_pal_create_process.
    //
    // Mandatory field.
    int pid;

    // Output. The exit status of the command. The semantic of
    // this value follows the one described in wait(2) man
    // page. For example, if the program terminated normally,
    // then WEXITSTATUS(exit_status) gives the value returned
    // from a main function.
    //
    // Mandatory field. Must not be NULL.
    int *exit_value;
};

int wamr_pal_init(const struct wamr_pal_attr *args);

int wamr_pal_create_process(struct wamr_pal_create_process_args *args);

int wamr_pal_destroy(void);

int wamr_pal_exec(struct wamr_pal_exec_args *args);

int wamr_pal_kill(int pid, int sig);

int pal_get_version(void);

int pal_init(const struct wamr_pal_attr *attr);

int pal_create_process(struct wamr_pal_create_process_args *args);

int pal_exec(struct wamr_pal_exec_args *args);

int pal_kill(int pid, int sig);

int pal_destroy(void);


#ifdef __cplusplus
}
#endif

#endif /* __WAMR_PAL_API_H__ */

