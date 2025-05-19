/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"

int
os_execve(const char *pathname, char *const argv[], int argc)
{
    pid_t pid;
    int ret;
    /* no environment variables */
    char *const envp[] = { NULL };

    if (pathname == NULL) {
        goto fail;
    }

    if (argc > 0) {
        if (argv == NULL) {
            goto fail;
        }

        /* The `argv[]` must be terminated by a NULL pointer. */
        if (argv[argc - 1] != NULL) {
            goto fail;
        }
    }

    pid = fork();
    if (pid < 0) {
        perror("fork failed: ");
        goto fail;
    }

    if (pid == 0) {
        /* child process */
        ret = execve(pathname, argv, envp);
        if (ret == -1) {
            perror("execve failed(from child): ");
        }
        /* _exit() for thread safe? */
        exit(ret);
    }
    else {
        /* parent process */
        int status;

        ret = waitpid(pid, &status, 0);
        if (ret == -1) {
            perror("waitpid failed: ");
            goto fail;
        }

        if (WIFEXITED(status)) {
            /* child terminated normally with exit code */
            ret = WEXITSTATUS(status);
            if (ret != 0) {
                printf("execute failed(from parent) with exit code: %d\n", ret);
            }
        }
        else {
            /*
             * child terminated abnormally.
             * include if killed or stopped by a signal
             */
            goto fail;
        }
    }

    return ret;
fail:
    return -1;
}