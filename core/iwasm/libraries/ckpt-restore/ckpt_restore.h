/*
 * Regents of the Univeristy of California, All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_common.h"
#include "wasm_runtime.h"

#ifndef MVVM_WAMR_EXPORT_H
#define MVVM_WAMR_EXPORT_H

#ifdef __cplusplus
extern "C" {
#endif

extern size_t snapshot_threshold;
extern bool checkpoint;

enum fd_op { MVVM_FOPEN = 0, MVVM_FWRITE = 1, MVVM_FREAD = 2, MVVM_FSEEK = 3 };

enum sync_op {
    SYNC_OP_MUTEX_LOCK = 0,
    SYNC_OP_MUTEX_UNLOCK,
    SYNC_OP_COND_WAIT,
    SYNC_OP_COND_SIGNAL,
    SYNC_OP_COND_BROADCAST,
    SYNC_OP_ATOMIC_WAIT,
    SYNC_OP_ATOMIC_NOTIFY,
};

struct sync_op_t {
    uint64 tid;
    uint32 ref;
    enum sync_op sync_op;
    uint64 expected;
    bool wait64;
};

#if WASM_ENABLE_LIBC_WASI != 0
void
insert_sock_send_to_data(uint32_t, uint8 *, uint32, uint16_t,
                         const __wasi_addr_t *);
void
insert_sock_open_data(uint32_t, int, int, uint32_t);
void
insert_sock_recv_from_data(uint32_t, uint8 *, uint32, uint16_t,
                           __wasi_addr_t *);
void
replay_sock_recv_from_data(uint32_t, uint8 **, unsigned long *,
                           __wasi_addr_t *);
void
insert_socket(int, int, int, int);
void
update_socket_fd_address(int, struct SocketAddrPool *);
void
init_gateway(struct SocketAddrPool *address);
void
set_tcp();
int
get_sock_fd(int);
#endif
#if WASM_ENABLE_LIB_PTHREAD != 0
void
insert_sync_op(wasm_exec_env_t exec_env, const uint32 *mutex,
               enum sync_op locking);
void
insert_sync_op_atomic_notify(wasm_exec_env_t exec_env, const uint32 *mutex,
                             uint32);
void
insert_sync_op_atomic_wait(wasm_exec_env_t exec_env, const uint32 *mutex,
                           uint64, bool);
void
insert_sync_op_atomic_wake(wasm_exec_env_t exec_env, const uint32 *mutex);
void
restart_execution(uint32 targs);
void insert_tid_start_arg(uint64_t, size_t, size_t);
void insert_parent_child(uint64_t, uint64_t);
extern int
pthread_create_wrapper(wasm_exec_env_t exec_env, uint32 *thread,
                       const void *attr, uint32 elem_index, uint32 arg);
extern int32
pthread_mutex_lock_wrapper(wasm_exec_env_t, uint32 *);
extern int32
pthread_mutex_unlock_wrapper(wasm_exec_env_t, uint32 *);
extern int32
pthread_mutex_init_wrapper(wasm_exec_env_t, uint32 *, void *);
extern int32
thread_spawn_wrapper(wasm_exec_env_t exec_env, uint32 start_arg);
extern int32
pthread_cond_wait_wrapper(wasm_exec_env_t, uint32 *, uint32 *);
extern int32
pthread_cond_signal_wrapper(wasm_exec_env_t, uint32 *);
extern int32
pthread_cond_broadcast_wrapper(wasm_exec_env_t, uint32 *);
extern uint32
wasm_runtime_atomic_wait(WASMModuleInstanceCommon *module, void *address,
                         uint64 expect, int64 timeout, bool wait64);
extern uint32
wasm_runtime_atomic_notify(WASMModuleInstanceCommon *module, void *address,
                           uint32 count);
#endif
void
serialize_to_file(struct WASMExecEnv *);

void
insert_fd(int, const char *, int, int, enum fd_op op);
void
remove_fd(int);
void
rename_fd(int, char const *, int, char const *);
bool
is_atomic_checkpointable();
void
wamr_handle_map(uint64_t old_tid, uint64_t new_tid);
void
lightweight_checkpoint(WASMExecEnv *);
void
lightweight_uncheckpoint(WASMExecEnv *);
void wamr_wait(wasm_exec_env_t);
void
sigint_handler(int sig);
void
register_sigtrap();
void
register_sigint();
void
sigtrap_handler(int sig);
void
print_memory(WASMExecEnv *);
void
print_exec_env_debug_info(WASMExecEnv *);

#ifdef __cplusplus
}
#endif
#endif // MVVM_WAMR_EXPORT_H