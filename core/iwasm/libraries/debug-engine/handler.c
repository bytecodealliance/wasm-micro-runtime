/*
 * Copyright (C) 2021 Ant Group.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <bh_log.h>
#include <handler.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include "debug_engine.h"
#include "packets.h"
#include "utils.h"
#include "wasm_runtime.h"

#define MAX_PACKET_SIZE (0x20000)
static char tmpbuf[MAX_PACKET_SIZE];

void
handle_generay_set(WASMGDBServer *server, char *payload)
{
    const char *name;
    char *args;

    args = strchr(payload, ':');
    if (args)
        *args++ = '\0';

    name = payload;
    LOG_VERBOSE("%s:%s\n", __FUNCTION__, payload);

    if (!strcmp(name, "StartNoAckMode")) {
        server->noack = true;
        write_packet(server, "OK");
    }
    if (!strcmp(name, "ThreadSuffixSupported")) {
        write_packet(server, "");
    }
    if (!strcmp(name, "ListThreadsInStopReply")) {
        write_packet(server, "");
    }
    if (!strcmp(name, "EnableErrorStrings")) {
        write_packet(server, "OK");
    }
}

static void
process_xfer(WASMGDBServer *server, const char *name, char *args)
{
    const char *mode = args;

    args = strchr(args, ':');
    if (args)
        *args++ = '\0';

    if (!strcmp(name, "libraries") && !strcmp(mode, "read")) {
        //TODO: how to get current wasm file name?
        uint64_t addr = wasm_debug_instance_get_load_addr(
          (WASMDebugInstance *)server->thread->debug_instance);
#if WASM_ENABLE_LIBC_WASI != 0
        char objname[128];
        wasm_debug_instance_get_current_object_name(
          (WASMDebugInstance *)server->thread->debug_instance, objname, 128);
        sprintf(tmpbuf,
                "l<library-list><library name=\"%s\"><section "
                "address=\"0x%lx\"/></library></library-list>",
                objname, addr);
#else
        sprintf(tmpbuf,
                "l<library-list><library name=\"%s\"><section "
                "address=\"0x%lx\"/></library></library-list>",
                "nobody.wasm", addr);
#endif
        write_packet(server, tmpbuf);
    }
}

void
porcess_wasm_local(WASMGDBServer *server, char *args)
{
    int frame_index;
    int local_index;
    char buf[16];
    int size = 16;
    bool ret;

    sprintf(tmpbuf, "E01");
    if (sscanf(args, "%d;%d", &frame_index, &local_index) == 2) {
        ret = wasm_debug_instance_get_local(
          (WASMDebugInstance *)server->thread->debug_instance, frame_index,
          local_index, buf, &size);
        if (ret && size > 0) {
            mem2hex(buf, tmpbuf, size);
        }
    }
    write_packet(server, tmpbuf);
}

void
porcess_wasm_global(WASMGDBServer *server, char *args)
{
    int frame_index;
    int global_index;
    char buf[16];
    int size = 16;
    bool ret;

    sprintf(tmpbuf, "E01");
    if (sscanf(args, "%d;%d", &frame_index, &global_index) == 2) {
        ret = wasm_debug_instance_get_global(
          (WASMDebugInstance *)server->thread->debug_instance, frame_index,
          global_index, buf, &size);
        if (ret && size > 0) {
            mem2hex(buf, tmpbuf, size);
        }
    }
    write_packet(server, tmpbuf);
}

void
handle_generay_query(WASMGDBServer *server, char *payload)
{
    const char *name;
    char *args;

    args = strchr(payload, ':');
    if (args)
        *args++ = '\0';
    name = payload;
    LOG_VERBOSE("%s:%s\n", __FUNCTION__, payload);

    if (!strcmp(name, "C")) {
        uint64_t pid, tid;
        pid = wasm_debug_instance_get_pid(
          (WASMDebugInstance *)server->thread->debug_instance);
        tid = wasm_debug_instance_get_tid(
          (WASMDebugInstance *)server->thread->debug_instance);
        snprintf(tmpbuf, sizeof(tmpbuf), "QCp%lx.%lx", pid, tid);
        write_packet(server, tmpbuf);
    }
    if (!strcmp(name, "Supported")) {
        sprintf(tmpbuf, "qXfer:libraries:read+;PacketSize=%x;", MAX_PACKET_SIZE);
        write_packet(server, tmpbuf);
    }

    if (!strcmp(name, "Xfer")) {
        name = args;

        if (!args) {
            LOG_ERROR("payload parse error during handle_generay_query");
            return;
        }

        args = strchr(args, ':');

        if (args) {
            *args++ = '\0';
            process_xfer(server, name, args);
        }
    }

    if (!strcmp(name, "HostInfo")) {
        //Todo: change vendor to Intel for outside tree？
        char triple[256];
        mem2hex("wasm32-Ant-wasi-wasm", triple,
                strlen("wasm32-Ant-wasi-wasm"));
        sprintf(tmpbuf,
                "vendor:Ant;ostype:wasi;arch:wasm32;"
                "triple:%s;endian:little;ptrsize:4;",
                triple);

        write_packet(server, tmpbuf);
    }
    if (!strcmp(name, "GetWorkingDir")) {
        if (getcwd(tmpbuf, PATH_MAX))
            write_packet(server, tmpbuf);
    }
    if (!strcmp(name, "QueryGDBServer")) {
        write_packet(server, "");
    }
    if (!strcmp(name, "VAttachOrWaitSupported")) {
        write_packet(server, "");
    }
    if (!strcmp(name, "ProcessInfo")) {
        //Todo: process id parent-pid
        uint64_t pid;
        pid = wasm_debug_instance_get_pid(
          (WASMDebugInstance *)server->thread->debug_instance);
        char triple[256];
        //arch-vendor-os-env(format)
        mem2hex("wasm32-Ant-wasi-wasm", triple,
                strlen("wasm32-Ant-wasi-wasm"));
        sprintf(tmpbuf,
                "pid:%lx;parent-pid:%lx;vendor:Ant;ostype:wasi;arch:wasm32;"
                "triple:%s;endian:little;ptrsize:4;",
                pid, pid, triple);

        write_packet(server, tmpbuf);
    }
    if (!strcmp(name, "RegisterInfo0")) {
        sprintf(
          tmpbuf,
          "name:pc;alt-name:pc;bitsize:64;offset:0;encoding:uint;format:hex;"
          "set:General Purpose Registers;gcc:16;dwarf:16;generic:pc;");
        write_packet(server, tmpbuf);
    }
    else if (!strncmp(name, "RegisterInfo", strlen("RegisterInfo"))) {
        write_packet(server, "E45");
    }
    if (!strcmp(name, "StructuredDataPlugins")) {
        write_packet(server, "");
    }

    if (args && (!strcmp(name, "MemoryRegionInfo"))) {
        uint64_t addr = strtol(args, NULL, 16);
        WASMDebugMemoryInfo *mem_info = wasm_debug_instance_get_memregion(
          (WASMDebugInstance *)server->thread->debug_instance, addr);
        if (mem_info) {
            char name[256];
            mem2hex(mem_info->name, name, strlen(mem_info->name));
            sprintf(tmpbuf, "start:%lx;size:%lx;permissions:%s;name:%s;",
                    (uint64)mem_info->start, mem_info->size, mem_info->permisson, name);
            write_packet(server, tmpbuf);
            wasm_debug_instance_destroy_memregion(
            (WASMDebugInstance *)server->thread->debug_instance, mem_info);
        }
    }

    if (!strcmp(name, "WasmData")) {

    }

    if (!strcmp(name, "WasmMem")) {

    }

    if (!strcmp(name, "Symbol")) {
        write_packet(server, "");
    }

    if (args && (!strcmp(name, "WasmCallStack"))) {
        uint64_t tid = strtol(args, NULL, 16);
        uint64_t buf[1024 / sizeof(uint64_t)];
        uint64_t count = wasm_debug_instance_get_call_stack_pcs(
          (WASMDebugInstance *)server->thread->debug_instance, tid, buf,
          1024 / sizeof(uint64_t));
        if (count > 0) {
            mem2hex((char *)buf, tmpbuf, count * sizeof(uint64_t));
            write_packet(server, tmpbuf);
        }
        else
            write_packet(server, "");
    }

    if (args && (!strcmp(name, "WasmLocal"))) {
        porcess_wasm_local(server, args);
    }

    if (args && (!strcmp(name, "WasmGlobal"))) {
        porcess_wasm_global(server, args);
    }
}

static void
send_thread_stop_status(WASMGDBServer *server, uint32_t status, uint64_t tid)
{
    int tids_number, len = 0, i = 0;
    uint64_t tids[20];
    char pc_string[17];
    uint32_t gdb_status = status;

    if (status == 0) {
        sprintf(tmpbuf, "W%02x", status);
        write_packet(server, tmpbuf);
        return;
    }
    tids_number = wasm_debug_instance_get_tids(
      (WASMDebugInstance *)server->thread->debug_instance, tids, 20);
    uint64_t pc = wasm_debug_instance_get_pc(
      (WASMDebugInstance *)server->thread->debug_instance);

    if (status == WAMR_SIG_SINGSTEP) {
        gdb_status = WAMR_SIG_TRAP;
    }

    //TODO: how name a wasm thread?
    len += sprintf(tmpbuf, "T%02xthread:%lx;name:%s;", gdb_status, tid, "nobody");
    if (tids_number > 0) {
        len += sprintf(tmpbuf + len, "threads:");
        while (i < tids_number) {
            if (i == tids_number - 1)
                len += sprintf(tmpbuf + len, "%lx;", tids[i]);
            else
                len += sprintf(tmpbuf + len, "%lx,", tids[i]);
            i++;
        }
    }
    mem2hex((void *)&pc, pc_string, 8);
    pc_string[8 * 2] = '\0';

    if (status == WAMR_SIG_TRAP) {
        len += sprintf(tmpbuf + len, "thread-pcs:%lx;00:%s,reason:%s;", pc,
                       pc_string, "breakpoint");
    }
    else if (status == WAMR_SIG_SINGSTEP) {
        len += sprintf(tmpbuf + len, "thread-pcs:%lx;00:%s,reason:%s;", pc,
                       pc_string, "trace");
    }
    else if (status > 0) {
        len += sprintf(tmpbuf + len, "thread-pcs:%lx;00:%s,reason:%s;", pc,
                       pc_string, "signal");
    }
    write_packet(server, tmpbuf);
}

void
handle_v_packet(WASMGDBServer *server, char *payload)
{
    const char *name;
    char *args;
    uint32_t status;
    args = strchr(payload, ';');
    if (args)
        *args++ = '\0';
    name = payload;
    LOG_VERBOSE("%s:%s\n", __FUNCTION__, payload);

    if (!strcmp("Cont?", name))
        write_packet(server, "vCont;c;C;s;S;");

    if (!strcmp("Cont", name)) {
        if (args && args[0] == 's') {
            char *numstring = strchr(args, ':');
            if (numstring) {
                *numstring++ = '\0';
                uint64_t tid = strtol(numstring, NULL, 16);
                wasm_debug_instance_set_cur_thread(
                  (WASMDebugInstance *)server->thread->debug_instance, tid);
                wasm_debug_instance_singlestep(
                  (WASMDebugInstance *)server->thread->debug_instance, tid);
                tid = wasm_debug_instance_wait_thread(
                  (WASMDebugInstance *)server->thread->debug_instance, tid,
                  &status);
                send_thread_stop_status(server, status, tid);
            }
        }
    }
}

void
handle_threadstop_request(WASMGDBServer *server, char *payload)
{
    uint64_t tid = wasm_debug_instance_get_tid(
      (WASMDebugInstance *)server->thread->debug_instance);
    uint32_t status;

    tid = wasm_debug_instance_wait_thread(
      (WASMDebugInstance *)server->thread->debug_instance, tid, &status);

    send_thread_stop_status(server, status, tid);
}

void
handle_set_current_thread(WASMGDBServer *server, char *payload)
{
    LOG_VERBOSE("%s:%s\n", __FUNCTION__, payload, payload);
    if ('g' == *payload++) {
        uint64_t tid;
        tid = strtol(payload, NULL, 16);
        if (tid > 0)
            wasm_debug_instance_set_cur_thread(
              (WASMDebugInstance *)server->thread->debug_instance, tid);
    }
    write_packet(server, "OK");
}

void
handle_get_register(WASMGDBServer *server, char *payload)
{
    int i = strtol(payload, NULL, 16);

    if (i != 0) {
        write_packet(server, "E01");
        return;
    }
    uint64_t regdata = wasm_debug_instance_get_pc(
      (WASMDebugInstance *)server->thread->debug_instance);
    mem2hex((void *)&regdata, tmpbuf, 8);
    tmpbuf[8 * 2] = '\0';
    write_packet(server, tmpbuf);
}

void
handle_get_json_request(WASMGDBServer *server, char *payload)
{
    char *args;

    args = strchr(payload, ':');
    if (args)
        *args++ = '\0';
    write_packet(server, "");
}

void
handle_get_read_binary_memory(WASMGDBServer *server, char *payload)
{
    write_packet(server, "");
}

void
handle_get_read_memory(WASMGDBServer *server, char *payload)
{
    size_t maddr, mlen;
    bool ret;

    sprintf(tmpbuf, "%s", "");
    if (sscanf(payload, "%zx,%zx", &maddr, &mlen) == 2) {
        if (mlen * 2 > MAX_PACKET_SIZE) {
            LOG_ERROR("Buffer overflow!");
            mlen = MAX_PACKET_SIZE / 2;
        }
        char *buff = wasm_runtime_malloc(mlen);
        if (buff) {
            ret = wasm_debug_instance_get_mem(
              (WASMDebugInstance *)server->thread->debug_instance, maddr, buff,
              &mlen);
            if (ret) {
                mem2hex(buff, tmpbuf, mlen);
            }
            wasm_runtime_free(buff);
        }
    }
    write_packet(server, tmpbuf);
}

void
handle_get_write_memory(WASMGDBServer *server, char *payload)
{
    size_t maddr, mlen, hex_len;
    int offset, act_len;
    char *buff;
    bool ret;

    sprintf(tmpbuf, "%s", "");
    if (sscanf(payload, "%zx,%zx:%n", &maddr, &mlen, &offset) == 2) {
        payload += offset;
        hex_len = strlen(payload);
        act_len = hex_len / 2 < mlen ? hex_len / 2 : mlen;
        buff = wasm_runtime_malloc(act_len);
        if (buff) {
            hex2mem(payload, buff, act_len);
            ret = wasm_debug_instance_set_mem(
              (WASMDebugInstance *)server->thread->debug_instance, maddr, buff,
              &mlen);
            if (ret) {
                sprintf(tmpbuf, "%s", "OK");
            }
            wasm_runtime_free(buff);
        }
    }
    write_packet(server, tmpbuf);
}

void
handle_add_break(WASMGDBServer *server, char *payload)
{
    size_t type, addr, length;

    if (sscanf(payload, "%zx,%zx,%zx", &type, &addr, &length) == 3) {
        if (type == eBreakpointSoftware) {
            bool ret = wasm_debug_instance_add_breakpoint(
              (WASMDebugInstance *)server->thread->debug_instance, addr,
              length);
            if (ret)
                write_packet(server, "OK");
            else
                write_packet(server, "E01");
            return;
        }
    }
    write_packet(server, "");
}

void
handle_remove_break(WASMGDBServer *server, char *payload)
{
    size_t type, addr, length;

    if (sscanf(payload, "%zx,%zx,%zx", &type, &addr, &length) == 3) {
        if (type == eBreakpointSoftware) {
            bool ret = wasm_debug_instance_remove_breakpoint(
              (WASMDebugInstance *)server->thread->debug_instance, addr,
              length);
            if (ret)
                write_packet(server, "OK");
            else
                write_packet(server, "E01");
            return;
        }
    }
    write_packet(server, "");
}

void
handle_continue_request(WASMGDBServer *server, char *payload)
{
    uint64_t tid;
    uint32_t status;

    wasm_debug_instance_continue(
      (WASMDebugInstance *)server->thread->debug_instance);

    tid = wasm_debug_instance_get_tid(
      (WASMDebugInstance *)server->thread->debug_instance);

    tid = wasm_debug_instance_wait_thread(
      (WASMDebugInstance *)server->thread->debug_instance, tid, &status);

    send_thread_stop_status(server, status, tid);
}

void
handle_kill_request(WASMGDBServer *server, char *payload)
{
    uint64_t tid;
    uint32_t status;

    wasm_debug_instance_kill(
      (WASMDebugInstance *)server->thread->debug_instance);

    tid = wasm_debug_instance_get_tid(
      (WASMDebugInstance *)server->thread->debug_instance);

    tid = wasm_debug_instance_wait_thread(
      (WASMDebugInstance *)server->thread->debug_instance, tid, &status);

    send_thread_stop_status(server, status, tid);
}

static void
handle_malloc(WASMGDBServer *server, char *payload)
{
    char *args;
    uint64_t size;
    int map_port = MMAP_PROT_NONE;
    uint64_t addr;

    sprintf(tmpbuf, "%s", "E03");

    args = strstr(payload, ",");
    if (args) {
        *args++ = '\0';
    }
    else {
        LOG_ERROR("Payload parse error during handle malloc");
        return;
    }

    size = strtol(payload, NULL, 16);
    if (size > 0) {
        while (*args) {
            if (*args == 'r') {
                map_port |= MMAP_PROT_READ;
            }
            if (*args == 'w') {
                map_port |= MMAP_PROT_WRITE;
            }
            if (*args == 'x') {
                map_port |= MMAP_PROT_EXEC;
            }
            args++;
        }
        addr = wasm_debug_instance_mmap(
          (WASMDebugInstance *)server->thread->debug_instance, size, map_port);
        if (addr) {
            sprintf(tmpbuf, "%lx", addr);
        }
    }
    write_packet(server, tmpbuf);
}

static void
handle_free(WASMGDBServer *server, char *payload)
{
    uint64_t addr;
    bool ret;

    sprintf(tmpbuf, "%s", "E03");
    addr = strtol(payload, NULL, 16);

    ret = wasm_debug_instance_ummap(
      (WASMDebugInstance *)server->thread->debug_instance, addr);
    if (ret) {
        sprintf(tmpbuf, "%s", "OK");
    }
    write_packet(server, tmpbuf);
}

void
handle____request(WASMGDBServer *server, char *payload)
{
    char *args;

    if (payload[0] == 'M') {
        args = payload + 1;
        handle_malloc(server, args);
    }
    if (payload[0] == 'm') {
        args = payload + 1;
        handle_free(server, args);
    }
}
