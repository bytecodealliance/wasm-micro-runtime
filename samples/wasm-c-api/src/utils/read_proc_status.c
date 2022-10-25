#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
report_error(const char *function, int line)
{
    perror(function);
    fprintf(stderr, "%s() failed at line %d in file %s\n", function, line,
            __FILE__);
}

bool
fetch_mem_info(const char *line)
{
    return strncmp(line, "Vm", 2) == 0 || strncmp(line, "Rss", 3) == 0
           || strncmp(line, "Threads:", strlen("Threads:")) == 0;
}

int
read_proc_status(pid_t pid, bool(filter)(const char *), char *buf,
                 size_t buf_cap)
{
    int ret = -1;

    char path[128] = { 0 };
    snprintf(path, sizeof(path) / sizeof(path[0]), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        report_error("fopen", __LINE__);
        goto quit;
    }

    char *line = NULL;
    size_t linecap = 0;
    size_t offset_buf = 0;
    ssize_t linelen = getline(&line, &linecap, f);
    if (linelen < 0) {
        report_error("getline", __LINE__);
        goto close_file;
    }

    memset(buf, 0, buf_cap);
    while (linelen > 0 && offset_buf < buf_cap) {
        if (filter(line)) {
            if (snprintf(buf + offset_buf, buf_cap - offset_buf, "%s", line)
                < 0) {
                report_error("snprintf", __LINE__);
                goto free_line;
            }
            offset_buf += linelen;
        }
        linelen = getline(&line, &linecap, f);
    }

    ret = offset_buf;

free_line:
    free(line);
close_file:
    fclose(f);
quit:
    return ret;
}