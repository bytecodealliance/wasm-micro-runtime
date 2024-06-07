/*
 * Regents of the Univeristy of California, All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef MVVM_WAMR_H
#define MVVM_WAMR_H

#include "wamr_exec_env.h"
#include "wamr_wasi_arguments.h"
#include "wasm_runtime.h"
#include "ckpt_restore.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <mutex>
#include <numeric>
#include <ranges>
#include <semaphore>
#include <sstream>
#include <string>
#include <tuple>
#if !defined(BH_PLATFORM_WINDOWS)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

#define MVVM_SOCK_ADDR "172.17.0.1"
#define MVVM_SOCK_ADDR6 "fe80::42:aeff:fe1f:b579"
#define MVVM_SOCK_MASK 24
#define MVVM_SOCK_MASK6 48
#define MVVM_SOCK_PORT 1235
#define MVVM_MAX_ADDR 5
#define MVVM_SOCK_INTERFACE "docker0"

enum opcode {
    MVVM_SOCK_SUSPEND = 0,
    MVVM_SOCK_SUSPEND_TCP_SERVER = 1,
    MVVM_SOCK_RESUME = 2,
    MVVM_SOCK_RESUME_TCP_SERVER = 3,
    MVVM_SOCK_INIT = 4,
    MVVM_SOCK_FIN = 5
};
struct mvvm_op_data {
    enum opcode op;
    bool is_tcp;
    int size;
    SocketAddrPool addr[MVVM_MAX_ADDR][2];
};
std::string
removeExtension(std::string &);
bool
is_ip_in_cidr(const char *base_ip, int subnet_mask_len, uint32_t ip);
bool
is_ipv6_in_cidr(const char *base_ip_str, int subnet_mask_len,
                struct in6_addr *ip);

class WAMRInstance
{
  public:
    WASMExecEnv *cur_env{};
    WASMExecEnv *exec_env{};
    WASMModuleInstanceCommon *module_inst{};
    WASMModuleCommon *module_{};
    WASMFunctionInstanceCommon *func{};

    std::string aot_file_path{};
    std::string wasm_file_path{};
    std::condition_variable int3_cv{};
    std::mutex int3_mtx{};
    std::unique_lock<std::mutex> int3_ul;
    std::vector<std::size_t> int3_addr{};
    std::vector<std::pair<std::size_t, std::size_t>> switch_addr{};
    std::vector<const char *> dir_{};
    std::vector<const char *> map_dir_{};
    std::vector<const char *> env_{};
    std::vector<const char *> arg_{};
    std::vector<const char *> addr_{};
    std::vector<const char *> ns_pool_{};
    std::vector<WAMRExecEnv *> execEnv{};
    std::map<int,
             std::tuple<std::string, std::vector<std::tuple<int, int, fd_op>>>>
        fd_map_{};
    // add offset to pair->tuple, 3rd param 'int'
    std::map<int, int> new_sock_map_{};
    std::map<int, SocketMetaData, std::greater<>> socket_fd_map_{};
    SocketAddrPool local_addr{};
    // lwcp is LightWeight CheckPoint
    std::map<uint64, int> lwcp_list;
    size_t ready = 0;
    std::mutex as_mtx{};
    std::vector<struct sync_op_t> sync_ops;
    bool should_snapshot{};
    WASMMemoryInstance **tmp_buf = nullptr;
    uint32 tmp_buf_size{};
    std::vector<struct sync_op_t>::iterator sync_iter;
    std::map<uint64, uint64> tid_map;
    std::map<korp_tid, korp_tid> korp_tid_map;
    std::map<uint64, uint64> child_tid_map;
    std::map<uint64, std::pair<int, int>> tid_start_arg_map;
    uint32 id{};
    size_t cur_thread;
    std::chrono::time_point<std::chrono::high_resolution_clock> time;
    std::vector<long long> latencies;
    bool is_jit;
    bool is_aot{};
    char error_buf[128]{};
    struct mvvm_op_data op_data {};
    uint32 buf_size{}, stack_size = 65536, heap_size = 3355443200;
    typedef struct ThreadArgs {
        wasm_exec_env_t exec_env;
    } ThreadArgs;

    explicit WAMRInstance(const char *wasm_path, bool is_jit);
    explicit WAMRInstance(const char *wasm_path, WASMExecEnv *exec_env);
    void instantiate();
    void recover(std::vector<std::unique_ptr<WAMRExecEnv>> *);
    bool load_wasm_binary(const char *wasm_path, char **buffer_ptr);
    bool get_int3_addr();
    bool replace_int3_with_nop();
    bool replace_mfence_with_nop();
    bool replace_nop_with_int3();
    void replay_sync_ops(bool, wasm_exec_env_t);
    WASMFunction *get_func();
    void set_func(WASMFunction *);
#if WASM_ENABLE_AOT != 0
    std::vector<uint32> get_args();
    AOTFunctionInstance *get_func(int index);
    AOTModule *get_module() const;
#endif
    WASMExecEnv *get_exec_env();
    WASMModuleInstance *get_module_instance() const;

    void set_wasi_args(WAMRWASIArguments &addrs);
    void set_wasi_args(const std::vector<std::string> &dir_list,
                       const std::vector<std::string> &map_dir_list,
                       const std::vector<std::string> &env_list,
                       const std::vector<std::string> &arg_list,
                       const std::vector<std::string> &addr_list,
                       const std::vector<std::string> &ns_lookup_pool);
    void spawn_child(WASMExecEnv *main_env, bool);
    void find_func(const char *name);
    int invoke_main();
    void invoke_init_c();
    int invoke_fopen(std::string &path, uint32 option);
    int invoke_frenumber(uint32 fd, uint32 to);
    int invoke_fseek(uint32 fd, uint32 flags, uint32 offset);
    int invoke_preopen(uint32 fd, const std::string &path);
    int invoke_sock_open(uint32_t domain, uint32_t socktype, uint32_t protocol,
                         uint32_t sockfd);
    int invoke_sock_client_connect(uint32_t sockfd, struct sockaddr *sock,
                                   socklen_t sock_size);
    int invoke_sock_server_connect(uint32_t sockfd, struct sockaddr *sock,
                                   socklen_t sock_size);
    int invoke_sock_accept(uint32_t sockfd, struct sockaddr *sock,
                           socklen_t sock_size);
    int invoke_sock_getsockname(uint32_t sockfd, struct sockaddr **sock,
                                socklen_t *sock_size);
    int invoke_recv(int sockfd, uint8 **buf, size_t len, int flags);
    int invoke_recvfrom(int sockfd, uint8 **buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);
    ~WAMRInstance();
};

#endif // MVVM_WAMR_H
