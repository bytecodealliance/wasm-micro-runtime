/*
 * Regents of the Univeristy of California, All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wamr.h"
#include "wamr_wasi_arguments.h"
#include <condition_variable>
#include <cstdlib>
#include <mutex>
extern WAMRInstance *wamr;
size_t snapshot_threshold;
size_t call_count = 0;
bool checkpoint = false;

void
insert_sock_open_data(uint32_t poolfd, int af, int socktype, uint32_t sockfd)
{
    if (wamr
        && wamr->socket_fd_map_.find(sockfd) != wamr->socket_fd_map_.end()) {
        wamr->socket_fd_map_[sockfd].socketOpenData.poolfd = poolfd;
        wamr->socket_fd_map_[sockfd].socketOpenData.af = af;
        wamr->socket_fd_map_[sockfd].socketOpenData.socktype = socktype;
        wamr->socket_fd_map_[sockfd].socketOpenData.sockfd = sockfd;
    }
    else {
        LOG_DEBUG("socket_fd %d not found", sockfd);
    }
}
#if !defined(BH_PLATFORM_WINDOWS)
void
insert_sock_send_to_data(uint32_t sock, uint8 *si_data, uint32 si_data_len,
                         uint16_t si_flags, const __wasi_addr_t *dest_addr)
{
    if (wamr && wamr->socket_fd_map_.find(sock) != wamr->socket_fd_map_.end()) {
        wamr->socket_fd_map_[sock].socketSentToData.sock = sock;
        wamr->socket_fd_map_[sock].socketSentToData.si_data =
            std::vector<uint8_t>(si_data, si_data + si_data_len);
        wamr->socket_fd_map_[sock].socketSentToData.si_flags = si_flags;

        if (dest_addr->kind == IPv4) {
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.ip.is_4 =
                true;
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.ip.ip4[0] =
                dest_addr->addr.ip4.addr.n0;
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.ip.ip4[1] =
                dest_addr->addr.ip4.addr.n1;
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.ip.ip4[2] =
                dest_addr->addr.ip4.addr.n2;
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.ip.ip4[3] =
                dest_addr->addr.ip4.addr.n3;
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.port =
                dest_addr->addr.ip4.port;
        }
        else {
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.ip.is_4 =
                false;
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.ip.ip6[0] =
                dest_addr->addr.ip6.addr.n0;
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.ip.ip6[1] =
                dest_addr->addr.ip6.addr.n1;
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.ip.ip6[2] =
                dest_addr->addr.ip6.addr.n2;
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.ip.ip6[3] =
                dest_addr->addr.ip6.addr.n3;
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.ip.ip6[4] =
                dest_addr->addr.ip6.addr.h0;
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.ip.ip6[5] =
                dest_addr->addr.ip6.addr.h1;
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.ip.ip6[6] =
                dest_addr->addr.ip6.addr.h2;
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.ip.ip6[7] =
                dest_addr->addr.ip6.addr.h3;
            wamr->socket_fd_map_[sock].socketSentToData.dest_addr.port =
                dest_addr->addr.ip6.port;
        }
    }
    else {
        LOG_DEBUG("socket_fd", sock, " not found");
    }
}

void
insert_sock_recv_from_data(uint32_t sock, uint8 *ri_data, uint32 ri_data_len,
                           uint16_t ri_flags, __wasi_addr_t *src_addr)
{

    if (wamr->time != std::chrono::high_resolution_clock::time_point())
        wamr->latencies.emplace_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - wamr->time)
                .count());
    wamr->time = std::chrono::high_resolution_clock::now();
    if (wamr->latencies.size() % 1000 == 0) {
        long long sum = std::accumulate(wamr->latencies.begin(),
                                        wamr->latencies.end(), 0LL);
        double average_latency =
            static_cast<double>(sum) / wamr->latencies.size();
        fprintf(stderr, "average latency %f\n", average_latency);
    }
    if (wamr->socket_fd_map_.find(sock) != wamr->socket_fd_map_.end()) {
        WasiSockRecvFromData recvFromData{};
        recvFromData.sock = sock;
        recvFromData.ri_data =
            std::vector<uint8_t>(ri_data, ri_data + ri_data_len);
        recvFromData.ri_flags = ri_flags;
        if (wamr->socket_fd_map_[sock].is_collection) {
            wamr->socket_fd_map_[sock].replay_start_index += 1;
        }
        if (src_addr->kind == IPv4) {
            recvFromData.src_addr.ip.is_4 = true;
            recvFromData.src_addr.ip.ip4[0] = src_addr->addr.ip4.addr.n0;
            recvFromData.src_addr.ip.ip4[1] = src_addr->addr.ip4.addr.n1;
            recvFromData.src_addr.ip.ip4[2] = src_addr->addr.ip4.addr.n2;
            recvFromData.src_addr.ip.ip4[3] = src_addr->addr.ip4.addr.n3;
            recvFromData.src_addr.port = src_addr->addr.ip4.port;
        }
        else {
            recvFromData.src_addr.ip.is_4 = false;
            recvFromData.src_addr.ip.ip6[0] = src_addr->addr.ip6.addr.n0;
            recvFromData.src_addr.ip.ip6[1] = src_addr->addr.ip6.addr.n1;
            recvFromData.src_addr.ip.ip6[2] = src_addr->addr.ip6.addr.n2;
            recvFromData.src_addr.ip.ip6[3] = src_addr->addr.ip6.addr.n3;
            recvFromData.src_addr.ip.ip6[4] = src_addr->addr.ip6.addr.h0;
            recvFromData.src_addr.ip.ip6[5] = src_addr->addr.ip6.addr.h1;
            recvFromData.src_addr.ip.ip6[6] = src_addr->addr.ip6.addr.h2;
            recvFromData.src_addr.ip.ip6[7] = src_addr->addr.ip6.addr.h3;
            recvFromData.src_addr.port = src_addr->addr.ip6.port;
        }
        LOG_DEBUG("insert_sock_recv_from_data %d %d", sock,
                  ((int)((struct mvvm_op_data *)ri_data)->op));
        if (((struct mvvm_op_data *)ri_data)->op == MVVM_SOCK_FIN) {
            wamr->socket_fd_map_[sock].is_collection = false;
            return;
        }
        wamr->socket_fd_map_[sock].socketRecvFromDatas.emplace_back(
            recvFromData);
    }
    else {
        LOG_DEBUG("socket_fd", sock, " not found");
    }
}
void
replay_sock_recv_from_data(uint32_t sock, uint8 **ri_data,
                           unsigned long *recv_size, __wasi_addr_t *src_addr)
{
    // check from wamr->socket_fd_map_[sock] and drain one
    // should be in the same order
    if (wamr->socket_fd_map_[sock].socketRecvFromDatas.empty()) {
        LOG_DEBUG("no recvfrom data %d", sock);
        *recv_size = 0;
        return;
    }
    // shoud we check the src_addr?
    if (wamr->socket_fd_map_[sock].replay_start_index
        >= (int)wamr->socket_fd_map_[sock].socketRecvFromDatas.size()) {
        LOG_DEBUG("replay index out of bound %d", sock);
        *recv_size = 0;
        return;
    }
    auto recvFromData =
        wamr->socket_fd_map_[sock]
            .socketRecvFromDatas[wamr->socket_fd_map_[sock].replay_start_index];
    wamr->socket_fd_map_[sock].replay_start_index++;
    memcpy(*ri_data, recvFromData.ri_data.data(), recvFromData.ri_data.size());
    *recv_size = recvFromData.ri_data.size();
    if (src_addr->kind == IPv4) {
        src_addr->addr.ip4.addr.n0 = recvFromData.src_addr.ip.ip4[0];
        src_addr->addr.ip4.addr.n1 = recvFromData.src_addr.ip.ip4[1];
        src_addr->addr.ip4.addr.n2 = recvFromData.src_addr.ip.ip4[2];
        src_addr->addr.ip4.addr.n3 = recvFromData.src_addr.ip.ip4[3];
        src_addr->addr.ip4.port = recvFromData.src_addr.port;
    }
    else {
        src_addr->addr.ip6.addr.n0 = recvFromData.src_addr.ip.ip6[0];
        src_addr->addr.ip6.addr.n1 = recvFromData.src_addr.ip.ip6[1];
        src_addr->addr.ip6.addr.n2 = recvFromData.src_addr.ip.ip6[2];
        src_addr->addr.ip6.addr.n3 = recvFromData.src_addr.ip.ip6[3];
        src_addr->addr.ip6.addr.h0 = recvFromData.src_addr.ip.ip6[4];
        src_addr->addr.ip6.addr.h1 = recvFromData.src_addr.ip.ip6[5];
        src_addr->addr.ip6.addr.h2 = recvFromData.src_addr.ip.ip6[6];
        src_addr->addr.ip6.addr.h3 = recvFromData.src_addr.ip.ip6[7];
        src_addr->addr.ip6.port = recvFromData.src_addr.port;
    }
}
#endif
// only support one type of requests at a time
void
set_tcp()
{
    wamr->op_data.is_tcp = true;
}
int
get_sock_fd(int fd)
{
    if (wamr->socket_fd_map_.find(fd) != wamr->socket_fd_map_.end())
        return wamr->new_sock_map_[fd];
    else
        return fd;
};

/** fopen, fseek, fwrite, fread */
void
insert_fd(int fd, const char *path, int flags, int offset, enum fd_op op)
{
    if (fd > 2) {
        LOG_DEBUG("insert_fd(fd,filename,flags, offset) fd: %d flags: %d "
                  "offset: %d, op: %d",
                  fd, flags, offset, ((int)op));
        std::string path_;
        std::vector<std::tuple<int, int, enum fd_op>> ops_;
        std::tie(path_, ops_) = wamr->fd_map_[fd];
        // policy is compression
        if (strcmp(path, "") != 0) {
            ops_.emplace_back(flags, offset, op);
            wamr->fd_map_[fd] = std::make_tuple(std::string(path), ops_);
        }
        else {
            if (ops_.size() == 1) {
                ops_.emplace_back(flags, offset, MVVM_FSEEK);
            }
            else {
                ops_.back() = std::make_tuple(1, offset, MVVM_FSEEK);
            }
            wamr->fd_map_[fd] = std::make_tuple(path_, ops_);
        }
    }
}

/** frename */
void
rename_fd(int old_fd, char const *old_path, int new_fd, char const *new_path)
{
    LOG_DEBUG("rename_fd(int old_fd, char const *old_path, int new_fd, char "
              "const *new_path) old:%d "
              "old_fd:%d new_fd:%d, new_path:%s",
              old_fd, old_path, new_fd, new_path);
    if (wamr->fd_map_.find(old_fd) != wamr->fd_map_.end()) {
        auto new_fd_ = wamr->fd_map_[old_fd];
        std::string path_;
        std::vector<std::tuple<int, int, enum fd_op>> ops_;
        std::tie(path_, ops_) = new_fd_;
        if (strcmp(old_path, "") == 0)
            wamr->fd_map_[new_fd] =
                std::make_tuple(std::string(new_path), ops_);
        else
            wamr->fd_map_[new_fd] = std::make_tuple(path_, ops_);
        wamr->fd_map_.erase(old_fd);
    }
};
/** fclose */
void
remove_fd(int fd)
{
    LOG_DEBUG("remove_fd(fd) fd%d", fd);
    if (wamr->fd_map_.find(fd) != wamr->fd_map_.end())
        wamr->fd_map_.erase(fd);
    else {
        if (wamr->socket_fd_map_.find(fd) != wamr->socket_fd_map_.end())
            wamr->socket_fd_map_.erase(fd);
        else
            LOG_DEBUG("fd not found %d", fd);
    }
}
bool
is_atomic_checkpointable()
{
    return checkpoint;
}

/*
    create fd-socketmetadata map and store the "domain", "type", "protocol"
   value
*/
void
insert_socket(int fd, int domain, int type, int protocol)
{
    // if protocol == 1, is the remote protocol, we need to getsockname?

    LOG_DEBUG("insert_socket(fd, domain, type, protocol) %d %d %d %d", fd,
              domain, type, protocol);
    if (wamr) {
        if (wamr->socket_fd_map_.find(fd) != wamr->socket_fd_map_.end()) {
            LOG_DEBUG("socket_fd already exist", fd);
        }
        else {
            SocketMetaData metaData{};
            metaData.domain = domain;
            metaData.type = type;
            metaData.protocol = protocol;
            wamr->socket_fd_map_[fd] = metaData;
            if (protocol == 1)
                wamr->socket_fd_map_[fd].is_server = true;

            if (protocol > 1) {
                wamr->socket_fd_map_[fd].socketAddress =
                    wamr->socket_fd_map_[protocol].socketAddress;
            }
        }
    }
}

void
update_socket_fd_address(int fd, SocketAddrPool *address)
{
    if (wamr)
        wamr->socket_fd_map_[fd].socketAddress.port = address->port;
    LOG_DEBUG("#update_socket_fd_address(fd, address) %d %d", fd,
              wamr->socket_fd_map_[fd].socketAddress.port);
    if (address->is_4) {
        wamr->socket_fd_map_[fd].socketAddress.is_4 = true;
        wamr->socket_fd_map_[fd].socketAddress.ip4[0] = address->ip4[0];
        wamr->socket_fd_map_[fd].socketAddress.ip4[1] = address->ip4[1];
        wamr->socket_fd_map_[fd].socketAddress.ip4[2] = address->ip4[2];
        wamr->socket_fd_map_[fd].socketAddress.ip4[3] = address->ip4[3];
    }
    else {
        wamr->socket_fd_map_[fd].socketAddress.ip6[0] = address->ip6[0];
        wamr->socket_fd_map_[fd].socketAddress.ip6[1] = address->ip6[1];
        wamr->socket_fd_map_[fd].socketAddress.ip6[2] = address->ip6[2];
        wamr->socket_fd_map_[fd].socketAddress.ip6[3] = address->ip6[3];
        wamr->socket_fd_map_[fd].socketAddress.ip6[4] = address->ip6[4];
        wamr->socket_fd_map_[fd].socketAddress.ip6[5] = address->ip6[5];
        wamr->socket_fd_map_[fd].socketAddress.ip6[6] = address->ip6[6];
        wamr->socket_fd_map_[fd].socketAddress.ip6[7] = address->ip6[7];
    }
}
#if !defined(BH_PLATFORM_WINDOWS)
void
init_gateway(SocketAddrPool *address)
{
    // tell gateway to keep alive the server
    if (wamr && wamr->op_data.op != MVVM_SOCK_RESUME
        && wamr->op_data.op != MVVM_SOCK_RESUME_TCP_SERVER) {
        struct sockaddr_in addr {};
        int fd = 0;
        ssize_t rc;
        wamr->op_data.op = MVVM_SOCK_INIT;
        wamr->op_data.addr[0][0] = wamr->local_addr;
        memcpy(&wamr->op_data.addr[0][1], address, sizeof(SocketAddrPool));

        // Create a socket
        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            LOG_DEBUG("socket error");
            exit(EXIT_FAILURE);
        }

        addr.sin_family = AF_INET;
        addr.sin_port = htons(MVVM_SOCK_PORT);

        // Convert IPv4 and IPv6 addresses from text to binary form
        if (inet_pton(AF_INET, MVVM_SOCK_ADDR, &addr.sin_addr) <= 0) {
            LOG_DEBUG("AF_INET not supported");
            exit(EXIT_FAILURE);
        }
        // Connect to the server
        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            LOG_DEBUG("Connection Failed ", errno);
            exit(EXIT_FAILURE);
        }

        LOG_DEBUG("Connected successfully");
        rc = send(fd, &wamr->op_data, sizeof(struct mvvm_op_data), 0);
        if (rc == -1) {
            LOG_DEBUG("send error");
            exit(EXIT_FAILURE);
        }

        // Clean up
        close(fd);
    }
}
#endif

void
insert_sync_op(wasm_exec_env_t exec_env, const uint32 *mutex,
               enum sync_op locking)
{
    LOG_DEBUG("insert sync on offset %d, as op: %d ",
              (uint32)(((uint8 *)mutex)
                       - ((WASMModuleInstance *)exec_env->module_inst)
                             ->memories[0]
                             ->memory_data),
              ((int)locking));
    struct sync_op_t sync_op = { .tid = ((uint64_t)exec_env->handle),
                                 .ref = *mutex,
                                 .sync_op = locking,
                                 .expected = 0,
                                 .wait64 = 0 };
    if (wamr)
        wamr->sync_ops.push_back(sync_op);
}
void
insert_sync_op_atomic_wait(wasm_exec_env_t exec_env, const uint32 *mutex,
                           uint64 expected, bool wait64)
{
    struct sync_op_t sync_op = {
        .tid = ((uint64_t)exec_env->handle),
        .ref = (uint32)(((uint8 *)mutex)
                        - ((WASMModuleInstance *)exec_env->module_inst)
                              ->memories[0]
                              ->memory_data),
        .sync_op = SYNC_OP_ATOMIC_WAIT,
        .expected = expected,
        .wait64 = wait64
    };
    if (wamr)
        wamr->sync_ops.push_back(sync_op);
}
void
insert_sync_op_atomic_wake(wasm_exec_env_t exec_env, const uint32 *mutex)
{
    // Calculate the ref value for the given mutex, similar to
    // insert_sync_op_atomic_wait
    uint32 ref = (uint32)(((uint8 *)mutex)
                          - ((WASMModuleInstance *)exec_env->module_inst)
                                ->memories[0]
                                ->memory_data);

    // Remove elements from wamr->sync_ops where the ref matches the given
    // mutex's ref
    auto new_end =
        std::remove_if(wamr->sync_ops.begin(), wamr->sync_ops.end(),
                       [ref](const sync_op_t &op) { return op.ref == ref; });
    if (wamr)
        wamr->sync_ops.erase(new_end, wamr->sync_ops.end());
}
void
insert_sync_op_atomic_notify(wasm_exec_env_t exec_env, const uint32 *mutex,
                             uint32 count)
{
    struct sync_op_t sync_op = {
        .tid = ((uint64_t)exec_env->handle),
        .ref = (uint32)(((uint8 *)mutex)
                        - ((WASMModuleInstance *)exec_env->module_inst)
                              ->memories[0]
                              ->memory_data),
        .sync_op = SYNC_OP_ATOMIC_NOTIFY,
        .expected = count,
        .wait64 = 0
    };
    if (wamr)
        wamr->sync_ops.push_back(sync_op);
}
void
insert_tid_start_arg(uint64_t tid, size_t start_arg, size_t vtid)
{
    LOG_DEBUG("insert_tid_start_arg %lu %lu %d", tid, start_arg, vtid);
    if (wamr)
        wamr->tid_start_arg_map[tid] = std::make_pair(start_arg, vtid);
};

void
wamr_handle_map(uint64_t old_tid, uint64_t tid)
{
    LOG_DEBUG("wamr_handle_map old:< %lu > new:< %lu >", old_tid, tid);
    if (wamr)
        wamr->tid_map[old_tid] = tid;
};

void
insert_parent_child(uint64_t tid, uint64_t child_tid)
{
    LOG_DEBUG("insert_parent_child %lu %lu", tid, child_tid);
    if (wamr)
        wamr->child_tid_map[child_tid] = tid;
}

void
lightweight_checkpoint(WASMExecEnv *exec_env)
{
    if (wamr && exec_env->is_restore) {
        return;
    }
    int fid = -1;
    if (((AOTFrame *)exec_env->cur_frame)) {
        fid = (int)((AOTFrame *)exec_env->cur_frame)->func_index;
    }
    LOG_DEBUG("checkpoint %p, func(%d)", ((void *)exec_env), fid);
    if (fid == -1) {
        LOG_DEBUG("skip checkpoint");
        return;
    }

    std::unique_lock as_ul(wamr->as_mtx);
    wamr->lwcp_list[((uint64_t)exec_env->handle)]++;
    wamr->ready++;
}

void
lightweight_uncheckpoint(WASMExecEnv *exec_env)
{
    if (wamr && exec_env->is_restore) {
        return;
    }
    int fid = -1;
    if (((AOTFrame *)exec_env->cur_frame)) {
        fid = (int)((AOTFrame *)exec_env->cur_frame)->func_index;
    }
    LOG_DEBUG("uncheckpoint %p func(%d)", ((void *)exec_env), fid);
    if (fid == -1) {
        LOG_DEBUG("skip uncheckpoint");
        return;
    }
    std::unique_lock as_ul(wamr->as_mtx);
    if (wamr->lwcp_list[((uint64_t)exec_env->handle)] == 0) {
        // someone has reset our counter
        // which means we've been serialized
        // so we shouldn't return back to the wasm state
        std::condition_variable cv;
        cv.wait(as_ul);
    }
    wamr->lwcp_list[((uint64_t)exec_env->handle)]--;
    wamr->ready--;
}

void
print_stack(AOTFrame *frame)
{
    if (frame) {
        LOG_DEBUG("stack: ");
        for (int *i = (int *)frame->lp; i < (int *)frame->sp; i++) {
            LOG_DEBUG("%d ", *i);
        }
        LOG_DEBUG("\n");
    }
    else {
        LOG_DEBUG("no cur_frame");
    }
}

void
print_exec_env_debug_info(WASMExecEnv *exec_env)
{
    LOG_DEBUG("----");
    if (!exec_env) {
        LOG_DEBUG("no exec_env");
        return;
    }
    if (exec_env->cur_frame) {
        int call_depth = 0;
        auto p = (AOTFrame *)exec_env->cur_frame;
        while (p) {
            uint32 *frame_lp = p->lp;
            LOG_DEBUG("%d", (size_t)((size_t)frame_lp - (size_t)p));
            LOG_DEBUG("depth %d, function %d, ip %d, lp %p, sp %p", call_depth,
                      p->func_index, p->ip_offset, (void *)frame_lp,
                      (void *)p->sp);
            call_depth++;
            print_stack(p);

            p = p->prev_frame;
        }
    }
    else {
        LOG_DEBUG("no cur_frame");
    }
    LOG_DEBUG("----");
}

void
print_memory(WASMExecEnv *exec_env)
{
    if (!exec_env)
        return;
    auto module_inst =
        reinterpret_cast<WASMModuleInstance *>(exec_env->module_inst);
    if (!module_inst)
        return;
    for (size_t j = 0; j < module_inst->memory_count; j++) {
        auto mem = module_inst->memories[j];
        if (mem) {
            LOG_DEBUG("memory data size %d", mem->memory_data_size);
            if (mem->memory_data_size >= 70288 + 64) {
                for (int *i = (int *)(mem->memory_data);
                     i < (int *)(mem->memory_data_end); ++i) {
                    if (1 <= *i && *i <= 9)
                        LOG_DEBUG("%zu = %d\n", (uint8 *)i - mem->memory_data,
                                  *i);
                }
                LOG_DEBUG("\n");
            }
        }
    }
}
void
segfault_handler(int sig)
{
    wamr->int3_cv.wait(wamr->int3_ul);
}
void
sigtrap_handler(int sig)
{
    auto exec_env = wamr->get_exec_env();
    print_exec_env_debug_info(exec_env);
    print_memory(exec_env);

#if defined(BH_PLATFORM_WINDOWS)
    signal(SIGILL, sigtrap_handler);
#endif
    call_count++;
    if (snapshot_threshold != 0 || checkpoint)
        if (call_count >= snapshot_threshold || checkpoint) {
            fprintf(stderr, "serializing\n");
            serialize_to_file(exec_env);
            fprintf(stderr, "serialized\n");
            exit(-1);
        }
}

void
register_sigtrap()
{
#if defined(BH_PLATFORM_WINDOWS)
    signal(SIGILL, sigtrap_handler);
    LOG_DEBUG("SIGILL registered");
#else
    struct sigaction sa {};
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sigtrap_handler;
    sa.sa_flags = SA_RESTART;

    struct sigaction sb {};
    sigemptyset(&sb.sa_mask);
    sb.sa_handler = segfault_handler;
    sb.sa_flags = SA_RESTART;

    // Register the signal handler for SIGTRAP
    if (sigaction(SIGTRAP, &sa, nullptr) == -1) {
        LOG_DEBUG("Error: cannot handle SIGTRAP");
        exit(-1);
    }
    else {
        if (sigaction(SIGSYS, &sa, nullptr) == -1) {
            LOG_DEBUG("Error: cannot handle SIGSYS");
            exit(-1);
        }
        else {
            if (sigaction(SIGSEGV, &sb, nullptr) == -1) {
                LOG_DEBUG("Error: cannot handle SIGSEGV");
                exit(-1);
            }
            else {
                LOG_DEBUG("SIGSEGV registered");
            }
            LOG_DEBUG("SIGSYS registered");
        }
        LOG_DEBUG("SIGTRAP registered");
    }
#endif
}

// Signal handler function for SIGINT
void
sigint_handler(int sig)
{
    if (checkpoint || wamr->module_->module_type == Wasm_Module_Bytecode) {
        serialize_to_file(wamr->exec_env);
        return;
    }
    LOG_DEBUG("Caught signal %d, performing custom logic...\n", sig);
    checkpoint = true;
    wamr->int3_ul = std::unique_lock(wamr->int3_mtx);
    wamr->replace_nop_with_int3();
    wamr->int3_cv.notify_all();

    register_sigtrap();
}
void
register_sigint()
{
#if defined(BH_PLATFORM_WINDOWS)
    // Define the sigaction structure
    signal(SIGINT, sigint_handler);
#else
    // Define the sigaction structure
    struct sigaction sa {};

    // Clear the structure
    sigemptyset(&sa.sa_mask);

    // Set the signal handler function
    sa.sa_handler = sigint_handler;

    // Set the flags
    sa.sa_flags = SA_RESTART;

    // Register the signal handler for SIGINT
    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        LOG_DEBUG("Error: cannot handle SIGINT");
        exit(EXIT_FAILURE);
    }
#endif
}