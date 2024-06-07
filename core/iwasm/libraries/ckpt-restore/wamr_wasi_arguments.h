/*
 * Regents of the Univeristy of California, All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef MVVM_WAMR_WASI_CONTEXT_H
#define MVVM_WAMR_WASI_CONTEXT_H

#include "ckpt_restore.h"
#include "wamr_serializer.h"
#include "wasm_runtime.h"
#include <atomic>
#include <map>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <tuple>
#include <vector>

struct WAMRAddrPool {
    uint8 ip4[4];
    uint16 ip6[8];
    bool is_4;
    uint8 mask;
};
struct WAMRWasiAddr {
    WAMRAddrPool ip;
    uint16 port;
};
struct WasiSockOpenData {
    uint32 poolfd;
    int af;
    int socktype;
    uint32 sockfd;
};
struct WasiSockSendToData {
    uint32 sock;
    std::vector<uint8_t> si_data;
    uint16_t si_flags;
    WAMRWasiAddr dest_addr;
    uint32 so_data_len;
};

struct WasiSockRecvFromData {
    uint32_t sock;
    std::vector<uint8_t> ri_data;
    uint16_t ri_flags;
    WAMRWasiAddr src_addr;
    uint32 ro_data_len;
};
struct SocketMetaData {
    int domain{};
    int type{};
    int protocol{};
    SocketAddrPool socketAddress{};
    WasiSockOpenData socketOpenData{};
    int replay_start_index{};
    bool is_server = false;
    bool is_collection = false;
    WasiSockSendToData socketSentToData{}; //
    std::vector<WasiSockRecvFromData> socketRecvFromDatas;
};
struct WAMRWASIArguments {
    std::map<int,
             std::tuple<std::string, std::vector<std::tuple<int, int, fd_op>>>>
        fd_map;
    std::map<int, SocketMetaData> socket_fd_map;
    std::vector<struct sync_op_t> sync_ops;
    std::vector<std::string> dir;
    std::vector<std::string> map_dir;
    std::vector<std::string> arg;
    std::vector<std::string> argv_list;
    std::vector<std::string> env_list;
    std::vector<std::string> addr_pool;
    std::vector<std::string> ns_lookup_list;
    std::map<uint64, std::pair<int, int>> tid_start_arg_map;
    std::map<int, int> child_tid_map;
    uint32_t exit_code;

    void dump_impl(WASIArguments *env);
    void restore_impl(WASIArguments *env);
};
#if __has_include(<expected>) && __cplusplus > 202002L
template<SerializerTrait<WASIArguments *> T>
void
dump(T t, WASIArguments *env)
{
    t->dump_impl(env);
}
template<SerializerTrait<WASIArguments *> T>
void
restore(T t, WASIArguments *env)
{
    t->restore_impl(env);
};
#else
void
dump(WAMRWASIArguments *t, WASIArguments *env);
void
restore(WAMRWASIArguments *t, WASIArguments *env);
#endif
#endif // MVVM_WAMR_WASI_CONTEXT_H
