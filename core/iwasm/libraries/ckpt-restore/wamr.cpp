/*
 * The WebAssembly Live Migration Project
 *
 *  By: Aibo Hu
 *      Yiwei Yang
 *      Brian Zhao
 *      Andrew Quinn
 *
 *  Copyright 2024 Regents of the Univeristy of California
 *  UC Santa Cruz Sluglab.
 */

#include "wamr.h"
#include "bh_log.h"
#include "platform_common.h"
#include "ckpt_restore.h"
#include "wamr_read_write.h"
#include "wamr_wasi_arguments.h"
#include "wasm_export.h"
#include "wasm_interp.h"
#include "wasm_runtime.h"
#include "wasm_runtime_common.h"
#include <semaphore>
#if WASM_ENABLE_LIB_PTHREAD != 0
#include "thread_manager.h"
#endif
#if defined(BH_PLATFORM_LINUX)
#include <unistd.h>
#elif defined(BH_PLATFORM_DARWIN)
#include <mach/mach.h>
#include <unistd.h>
#else
#include <psapi.h>
#include <windows.h>
#endif

WAMRInstance::ThreadArgs **argptr;
#if __has_include(<expected>) && __cplusplus > 202002L
std::counting_semaphore<100> wakeup(0);
std::counting_semaphore<100> thread_init(0);
#else
template<int MaxCount>
class CountingSemaphore
{
  private:
    std::mutex mtx_;
    std::condition_variable cv_;
    int count_;

  public:
    explicit CountingSemaphore(int count)
      : count_(count)
    {}

    void acquire()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return count_ > 0; });
        --count_;
    }

    void release()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        ++count_;
        cv_.notify_one();
    }

    void release(int n)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        count_ += n;
        cv_.notify_all();
    }
};
CountingSemaphore<100> wakeup(0);
CountingSemaphore<100> thread_init(0);
#endif
WAMRInstance *wamr;
WriteStream *writer;
ReadStream *reader;
std::vector<std::unique_ptr<WAMRExecEnv>> as;

std::string
removeExtension(std::string &filename)
{
    size_t dotPos = filename.find_last_of('.');
    std::string res;
    if (dotPos != std::string::npos) {
        // Extract the substring before the period
        res = filename.substr(0, dotPos);
    }
    else {
        // If there's no period in the string, it means there's no extension.
        LOG_DEBUG("No extension found.");
    }
    return res;
};
static auto string_vec_to_cstr_array =
    [](const std::vector<std::string> &vecStr) {
        std::vector<const char *> cstrArray(vecStr.size());
        if (vecStr.data() == nullptr || vecStr[0].empty())
            return std::vector<const char *>(0);
        std::transform(vecStr.begin(), vecStr.end(), cstrArray.begin(),
                       [](const std::string &str) { return str.c_str(); });
        return cstrArray;
    };

WAMRInstance::WAMRInstance(const char *wasm_path, bool is_jit)
  : is_jit(is_jit)
{
    {
        std::string path(wasm_path);

        if (path.substr(path.length() - 5) == ".wasm") {
            is_aot = false;
            wasm_file_path = path;
            aot_file_path = path.substr(0, path.length() - 5) + ".aot";
        }
        else if (path.substr(path.length() - 4) == ".aot") {
            is_aot = true;
            wasm_file_path = path.substr(0, path.length() - 4) + ".wasm";
            aot_file_path = path;
        }
        else {
            std::cout << "Invalid file extension. Please provide a path ending "
                         "in either '.wasm' or '.aot'."
                      << std::endl;
            throw;
        }
    }

    RuntimeInitArgs wasm_args;
    memset(&wasm_args, 0, sizeof(RuntimeInitArgs));
    wasm_args.mem_alloc_type = Alloc_With_Allocator;
    wasm_args.mem_alloc_option.allocator.malloc_func = ((void *)malloc);
    wasm_args.mem_alloc_option.allocator.realloc_func = ((void *)realloc);
    wasm_args.mem_alloc_option.allocator.free_func = ((void *)free);
    wasm_args.max_thread_num = 16;
    if (!is_jit)
        wasm_args.running_mode = RunningMode::Mode_Interp;
    else
        wasm_args.running_mode = RunningMode::Mode_LLVM_JIT;
    //	static char global_heap_buf[512 * 1024];// what is this?
    //    wasm_args.mem_alloc_type = Alloc_With_Pool;
    //    wasm_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    //    wasm_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
    bh_log_set_verbose_level(0);
    if (!wasm_runtime_full_init(&wasm_args)) {
        LOG_DEBUG("Init runtime environment failed.");
        throw;
    }
    // initialiseWAMRNatives();
    char *buffer{};
    if (!load_wasm_binary(wasm_path, &buffer)) {
        LOG_DEBUG("Load wasm binary failed.\n");
        throw;
    }
    module_ = wasm_runtime_load((uint8_t *)buffer, buf_size, error_buf,
                                sizeof(error_buf));
    if (!module_) {
        LOG_DEBUG("Load wasm module failed. error: %s", error_buf);
        throw;
    }
#if !defined(BH_PLATFORM_WINDOWS)
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        LOG_DEBUG("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            // IPv4
            auto *ipv4 = (struct sockaddr_in *)ifa->ifa_addr;
            uint32_t ip = ntohl(ipv4->sin_addr.s_addr);
            if (is_ip_in_cidr(MVVM_SOCK_ADDR, MVVM_SOCK_MASK, ip)) {
                // Extract IPv4 address
                local_addr.ip4[0] = (ip >> 24) & 0xFF;
                local_addr.ip4[1] = (ip >> 16) & 0xFF;
                local_addr.ip4[2] = (ip >> 8) & 0xFF;
                local_addr.ip4[3] = ip & 0xFF;
                if (local_addr.ip4[1] == 17) {
                    break;
                }
            }
        }
        else if (ifa->ifa_addr->sa_family == AF_INET6) {
            // IPv6
            auto *ipv6 = (struct sockaddr_in6 *)ifa->ifa_addr;
            // Extract IPv6 address
            const auto *bytes = (const uint8_t *)ipv6->sin6_addr.s6_addr;
            if (is_ipv6_in_cidr(MVVM_SOCK_ADDR6, MVVM_SOCK_MASK6,
                                &ipv6->sin6_addr)) {
                for (int i = 0; i < 16; i += 2) {
                    local_addr.ip6[i / 2] = (bytes[i] << 8) + bytes[i + 1];
                }
            }
        }
    }
    local_addr.is_4 = true;

    freeifaddrs(ifaddr);

#endif
}

bool
WAMRInstance::load_wasm_binary(const char *wasm_path, char **buffer_ptr)
{
    // *buffer_ptr = bh_read_file_to_buffer(wasm_path, &buf_size);
    // if (!*buffer_ptr) {
    //     LOG_DEBUG("Open wasm app file failed.\n");
    //     return false;
    // }
    if ((get_package_type((const uint8_t *)*buffer_ptr, buf_size)
         != Wasm_Module_Bytecode)
        && (get_package_type((const uint8_t *)*buffer_ptr, buf_size)
            != Wasm_Module_AoT)) {
        LOG_DEBUG(
            "WASM bytecode or AOT object is expected but other file format");

        BH_FREE(*buffer_ptr);
        return false;
    }

    return true;
}
WAMRInstance::WAMRInstance(const char *wasm_path, WASMExecEnv *exec_env)
{
    std::string path(wasm_path);
    this->exec_env = cur_env = exec_env;
    module_inst = exec_env->module_inst;
    module_ = wasm_exec_env_get_module(exec_env);

    if (path.substr(path.length() - 4) == ".bin") {
        is_aot = module_->module_type == Wasm_Module_AoT;
        wasm_file_path = path.substr(0, path.length() - 4) + ".wasm";
        aot_file_path = path.substr(0, path.length() - 4) + ".aot";
    }
}
WAMRInstance::~WAMRInstance()
{
    if (!exec_env)
        wasm_runtime_destroy_exec_env(exec_env);
    if (!module_inst)
        wasm_runtime_deinstantiate(module_inst);
    if (!module_)
        wasm_runtime_unload(module_);
    wasm_runtime_destroy();
}
void
WAMRInstance::find_func(const char *name)
{
#if WASM_ENABLE_CUSTUM_NAME_SECTION != 0
    if (!(func = wasm_runtime_lookup_function(module_inst, name))) {
        LOG_DEBUG("The wasi\"%s\"function is not found.", name);
        auto target_module = get_module_instance()->e;
        for (int i = 0; i < target_module->function_count; i++) {
            auto cur_func = &target_module->functions[i];
            if (cur_func->is_import_func) {
                LOG_DEBUG("%s %d", cur_func->u.func_import->field_name, i);

                if (!strcmp(cur_func->u.func_import->field_name, name)) {

                    func = ((WASMFunctionInstanceCommon *)cur_func);
                    break;
                }
            }
            else {
                LOG_DEBUG("%s %d", cur_func->u.func->field_name, i);

                if (!strcmp(cur_func->u.func->field_name, name)) {
                    func = ((WASMFunctionInstanceCommon *)cur_func);
                    break;
                }
            }
        }
    }
#else
    LOG_ERROR("Not supported without custom name section");
    exit(-1);
#endif
}
int
WAMRInstance::invoke_main()
{
    if (!(func = wasm_runtime_lookup_wasi_start_function(module_inst))) {
        LOG_DEBUG("The wasi mode main function is not found.");
        return -1;
    }

    return wasm_runtime_call_wasm(exec_env, func, 0, nullptr);
}
void
WAMRInstance::invoke_init_c()
{
    auto name1 = "__wasm_call_ctors";
    if (!(func = wasm_runtime_lookup_function(module_inst, name1))) {
        LOG_DEBUG("The wasi ", name1, " function is not found.");
    }
    else {
        wasm_runtime_call_wasm(exec_env, func, 0, nullptr);
    }
}
int
WAMRInstance::invoke_fopen(std::string &path, uint32 option)
{
    char *buffer_ = nullptr;
    uint32_t buffer_for_wasm;
    find_func("restart_fd");
    buffer_for_wasm =
        wasm_runtime_module_malloc(module_inst, path.size(), (void **)&buffer_);
    if (buffer_for_wasm != 0) {
        uint32 argv[3];
        argv[0] = buffer_for_wasm; // pass the buffer_ address for WASM space
        argv[1] = option;          // the size of buffer_
        strncpy(buffer_, path.c_str(),
                path.size()); // use native address for accessing in runtime
        buffer_[path.size()] = '\0';
        wasm_runtime_call_wasm(exec_env, func, 2, argv);
        wasm_runtime_module_free(module_inst, buffer_for_wasm);
        return ((int)argv[0]);
    }
    return -1;
};
int
WAMRInstance::invoke_frenumber(uint32 fd, uint32 to)
{
    uint32 argv[2] = { fd, to };
    find_func("__wasi_fd_renumber");
    wasm_runtime_call_wasm(exec_env, func, 2, argv);
    return argv[0];
};

int
WAMRInstance::invoke_sock_open(uint32_t domain, uint32_t socktype,
                               uint32_t protocol, uint32_t sockfd)
{
    uint32 argv[4] = { domain, socktype, protocol, sockfd };
    find_func("restart_socket");
    auto res = wasm_runtime_call_wasm(exec_env, func, 4, argv);
    return argv[0];
}
int
WAMRInstance::invoke_sock_client_connect(uint32_t sockfd, struct sockaddr *sock,
                                         socklen_t sock_size)
{
    uint32 argv[1] = { sockfd };
    find_func("restart_socket_client");
    wasm_runtime_call_wasm(exec_env, func, 1, argv);
    int res = argv[0];
    return -1;
}
int
WAMRInstance::invoke_sock_server_connect(uint32_t sockfd, struct sockaddr *sock,
                                         socklen_t sock_size)
{
    uint32 argv[1] = { sockfd };
    find_func("restart_socket_server");
    wasm_runtime_call_wasm(exec_env, func, 1, argv);
    int res = argv[0];
    return -1;
}
int
WAMRInstance::invoke_sock_accept(uint32_t sockfd, struct sockaddr *sock,
                                 socklen_t sock_size)
{
    char *buffer1_ = nullptr;
    char *buffer2_ = nullptr;
    uint32_t buffer1_for_wasm;
    uint32_t buffer2_for_wasm;
    find_func("accept");

    buffer1_for_wasm =
        wasm_runtime_module_malloc(module_inst, sizeof(struct sockaddr),
                                   reinterpret_cast<void **>(&buffer1_));
    buffer2_for_wasm =
        wasm_runtime_module_malloc(module_inst, sizeof(struct sockaddr),
                                   reinterpret_cast<void **>(&buffer2_));
    if (buffer1_for_wasm != 0 && buffer2_for_wasm != 0) {
        uint32 argv[3];
        memcpy(buffer1_, sock,
               sizeof(struct sockaddr)); // use native address for accessing in
                                         // runtime
        memcpy(
            buffer2_, &sock_size,
            sizeof(socklen_t)); // use native address for accessing in runtime
        argv[0] = sockfd;       // pass the buffer_ address for WASM space
        argv[1] = buffer1_for_wasm;
        argv[2] = buffer2_for_wasm;
        wasm_runtime_call_wasm(exec_env, func, 3, argv);
        int res = argv[0];
        wasm_runtime_module_free(module_inst, buffer1_for_wasm);
        wasm_runtime_module_free(module_inst, buffer2_for_wasm);
        return res;
    }
    return -1;
}
int
WAMRInstance::invoke_sock_getsockname(uint32_t sockfd, struct sockaddr **sock,
                                      socklen_t *sock_size)
{
    char *buffer1_ = nullptr;
    char *buffer2_ = nullptr;
    uint32_t buffer1_for_wasm;
    uint32_t buffer2_for_wasm;
    find_func("getsockname");
    buffer1_for_wasm = wasm_runtime_module_malloc(
        module_inst, *sock_size, reinterpret_cast<void **>(&buffer1_));
    buffer2_for_wasm = wasm_runtime_module_malloc(
        module_inst, sizeof(socklen_t), reinterpret_cast<void **>(&buffer2_));
    if (buffer1_for_wasm != 0) {
        uint32 argv[3];
        memcpy(buffer1_, *sock, sizeof(struct sockaddr));
        argv[0] = sockfd;
        argv[1] = buffer1_for_wasm;
        argv[2] = buffer2_for_wasm;
        wasm_runtime_call_wasm(exec_env, func, 3, argv);
        memcpy(*sock, buffer1_, sizeof(struct sockaddr));
        int res = argv[0];
        wasm_runtime_module_free(module_inst, buffer1_for_wasm);
        return res;
    }
    return -1;
}
int
WAMRInstance::invoke_fseek(uint32 fd, uint32 flags, uint32 offset)
{
    // return 0;
    find_func("__wasi_fd_seek");
    uint32 argv[5] = { fd, offset, 0, flags, 0 };
    auto res = wasm_runtime_call_wasm(exec_env, func, 5, argv);
    return argv[0];
}
int
WAMRInstance::invoke_preopen(uint32 fd, const std::string &path)
{

    char *buffer_ = nullptr;
    uint32_t buffer_for_wasm;
    find_func("__wasilibc_nocwd_openat_nomode");
    buffer_for_wasm = wasm_runtime_module_malloc(
        module_inst, 100, reinterpret_cast<void **>(&buffer_));
    if (buffer_for_wasm != 0) {
        uint32 argv[3];
        strncpy(buffer_, path.c_str(),
                path.size()); // use native address for accessing in runtime
        argv[0] = fd;         // pass the buffer_ address for WASM space
        argv[1] = buffer_for_wasm; // the size of buffer_
        argv[2] = 102;             // O_RW | O_CREATE
        wasm_runtime_call_wasm(exec_env, func, 3, argv);
        int res = argv[0];
        wasm_runtime_module_free(module_inst, buffer_for_wasm);
        return res;
    }
    return -1;
}
int
WAMRInstance::invoke_recv(int sockfd, uint8 **buf, size_t len, int flags)
{
    char *buffer_ = nullptr;
    uint32_t buffer_for_wasm;
    find_func("recv");

    buffer_for_wasm = wasm_runtime_module_malloc(
        module_inst, len, reinterpret_cast<void **>(&buffer_));
    if (buffer_for_wasm != 0) {
        uint32 argv[4];
        memcpy(buffer_, *buf,
               len);      // use native address for accessing in runtime
        argv[0] = sockfd; // pass the buffer_ address for WASM space
        argv[1] = buffer_for_wasm; // the size of buffer_
        argv[2] = len;
        argv[3] = flags;
        wasm_runtime_call_wasm(exec_env, func, 4, argv);
        int res = argv[0];
        memcpy(*buf, buffer_, len);
        wasm_runtime_module_free(module_inst, buffer_for_wasm);
        return res;
    }
    return -1;
}
int
WAMRInstance::invoke_recvfrom(int sockfd, uint8 **buf, size_t len, int flags,
                              struct sockaddr *src_addr, socklen_t *addrlen)
{
    char *buffer1_ = nullptr;
    char *buffer2_ = nullptr;
    char *buffer3_ = nullptr;
    uint32_t buffer1_for_wasm;
    uint32_t buffer2_for_wasm;
    uint32_t buffer3_for_wasm;
    find_func("recvfrom");

    buffer1_for_wasm = wasm_runtime_module_malloc(
        module_inst, len, reinterpret_cast<void **>(&buffer1_));
    buffer2_for_wasm =
        wasm_runtime_module_malloc(module_inst, sizeof(struct sockaddr),
                                   reinterpret_cast<void **>(&buffer2_));
    buffer3_for_wasm = wasm_runtime_module_malloc(
        module_inst, sizeof(socklen_t), reinterpret_cast<void **>(&buffer3_));
    if (buffer1_for_wasm != 0 && buffer2_for_wasm != 0
        && buffer3_for_wasm != 0) {
        uint32 argv[6];
        memcpy(buffer1_, *buf,
               len); // use native address for accessing in runtime
        memcpy(buffer2_, src_addr,
               sizeof(struct sockaddr)); // use native address for accessing in
                                         // runtime
        memcpy(
            buffer3_, addrlen,
            sizeof(socklen_t)); // use native address for accessing in runtime
        argv[0] = sockfd;       // pass the buffer_ address for WASM space
        argv[1] = buffer1_for_wasm; // the size of buffer_
        argv[2] = len;
        argv[3] = flags;
        argv[4] = buffer2_for_wasm;
        argv[5] = buffer3_for_wasm;
        wasm_runtime_call_wasm(exec_env, func, 6, argv);
        int res = argv[0];
        memcpy(*buf, buffer1_, len);
        wasm_runtime_module_free(module_inst, buffer1_for_wasm);
        wasm_runtime_module_free(module_inst, buffer2_for_wasm);
        wasm_runtime_module_free(module_inst, buffer3_for_wasm);
        return res;
    }
    return -1;
}
WASMExecEnv *
WAMRInstance::get_exec_env()
{
    return cur_env; // should return the current thread's
}

WASMModuleInstance *
WAMRInstance::get_module_instance() const
{
    return reinterpret_cast<WASMModuleInstance *>(exec_env->module_inst);
}

#if WASM_ENABLE_AOT != 0
AOTModule *
WAMRInstance::get_module() const
{
    return reinterpret_cast<AOTModule *>(
        reinterpret_cast<WASMModuleInstance *>(exec_env->module_inst)->module);
}
#endif

void
restart_execution(uint32 id)
{
    WAMRInstance::ThreadArgs *targs = argptr[id];
    wasm_interp_call_func_bytecode(
        (WASMModuleInstance *)targs->exec_env->module_inst, targs->exec_env,
        targs->exec_env->cur_frame->function,
        targs->exec_env->cur_frame->prev_frame);
}
#if WASM_ENABLE_LIB_PTHREAD != 0
extern "C" {
korp_mutex syncop_mutex;
korp_cond syncop_cv;
}
void
WAMRInstance::replay_sync_ops(bool main, wasm_exec_env_t exec_env)
{
    if (main) {
        std::map<uint32, uint32> ref_map = {};
        for (auto &i : sync_ops) {
            // remap to new tids so that if we reserialize it'll be correct
            i.tid = tid_map[i.tid];
            if (ref_map.find(i.ref) != ref_map.end()) {
                pthread_mutex_init_wrapper(exec_env, &i.ref, nullptr);
                ref_map[i.ref] = i.tid;
            }
        }
        // start from the beginning
        sync_iter = sync_ops.begin();
        thread_init.release(100);
    }
    else {
        // wait for remap to finish
        thread_init.acquire();
    }
    // Actually replay
    os_mutex_lock(&syncop_mutex);
    while (sync_iter != sync_ops.end()) {
        LOG_DEBUG("test %lu == %lu, op %d\n", (uint64_t)exec_env->handle,
                  (uint64_t)sync_iter->tid, ((int)sync_iter->sync_op));
        if (((uint64_t)(*sync_iter).tid) == ((uint64_t)exec_env->handle)) {
            LOG_DEBUG("replay %lu, op %d\n", sync_iter->tid,
                      ((int)sync_iter->sync_op));
            auto mysync = sync_iter;
            ++sync_iter;
            // do op
            switch (mysync->sync_op) {
                case SYNC_OP_MUTEX_LOCK:
                    pthread_mutex_lock_wrapper(exec_env, &(mysync->ref));
                    break;
                case SYNC_OP_MUTEX_UNLOCK:
                    pthread_mutex_unlock_wrapper(exec_env, &(mysync->ref));
                    break;
                case SYNC_OP_COND_WAIT:
                    pthread_cond_wait_wrapper(exec_env, &(mysync->ref),
                                              nullptr);
                    break;
                case SYNC_OP_COND_SIGNAL:
                    pthread_cond_signal_wrapper(exec_env, &(mysync->ref));
                    break;
                case SYNC_OP_COND_BROADCAST:
                    pthread_cond_broadcast_wrapper(exec_env, &(mysync->ref));
                    break;
                case SYNC_OP_ATOMIC_WAIT:
                    wasm_runtime_atomic_wait(
                        exec_env->module_inst,
                        ((uint8_t *)((WASMModuleInstance *)
                                         exec_env->module_inst)
                             ->memories[0]
                             ->memory_data
                         + mysync->ref),
                        mysync->expected, -1, mysync->wait64);
                    break;
                case SYNC_OP_ATOMIC_NOTIFY:
                    break;
            }
            // wakeup everyone
            os_cond_signal(&syncop_cv);
        }
        else {
            os_cond_reltimedwait(&syncop_cv, &syncop_mutex, 10);
        }
    }
    os_mutex_unlock(&syncop_mutex);
}
// End Sync Op Specific Stuff
#endif
WAMRExecEnv *child_env;
// will call pthread create wrapper if needed?
void
WAMRInstance::recover(std::vector<std::unique_ptr<WAMRExecEnv>> *e_)
{
    execEnv.reserve(e_->size());
    std::transform(e_->begin(), e_->end(), std::back_inserter(execEnv),
                   [](const std::unique_ptr<WAMRExecEnv> &uniquePtr) {
                       return uniquePtr ? uniquePtr.get() : nullptr;
                   });
    // got this done tommorrow.
    // order threads by id (descending)
    std::sort(execEnv.begin(), execEnv.end(), [](const auto &a, const auto &b) {
        return a->frames.back()->function_index
               < b->frames.back()->function_index;
    });

    argptr = (ThreadArgs **)malloc(sizeof(void *) * execEnv.size());
    set_wasi_args(execEnv.front()->module_inst.wasi_ctx);
    instantiate();
    this->time = std::chrono::high_resolution_clock::now();
    invoke_init_c();

    restore(execEnv.front(), cur_env);
    if (tid_start_arg_map.find(execEnv.back()->cur_count)
        != tid_start_arg_map.end()) {
        std::sort(execEnv.begin() + 1, execEnv.end(),
                  [&](const auto &a, const auto &b) {
                      return tid_start_arg_map[a->cur_count].second
                             < tid_start_arg_map[b->cur_count].second;
                  });
    }

    auto main_env = cur_env;
    auto main_saved_call_chain = main_env->restore_call_chain;
    cur_thread = ((uint64_t)main_env->handle);

    fprintf(stderr, "main_env created %p %p\n\n", main_env,
            main_saved_call_chain);

    main_env->is_restore = true;

    main_env->restore_call_chain = nullptr;

#if WASM_ENABLE_LIB_PTHREAD != 0
    spawn_child(main_env, true);
#endif
    // restart main thread execution
    if (!is_aot) {
        wasm_interp_call_func_bytecode(get_module_instance(), get_exec_env(),
                                       get_exec_env()->cur_frame->function,
                                       get_exec_env()->cur_frame->prev_frame);
    }
    else {
        exec_env = cur_env = main_env;
        module_inst = main_env->module_inst;

        fprintf(stderr, "invoke_init_c\n");
        fprintf(stderr, "wakeup.release\n");
        wakeup.release(100);

        cur_env->is_restore = true;
        cur_env->restore_call_chain = main_saved_call_chain;
#if WASM_ENABLE_LIB_PTHREAD != 0
        fprintf(stderr, "invoke main %p %p\n", cur_env,
                cur_env->restore_call_chain);
        // replay sync ops to get OS state matching
        wamr_handle_map(execEnv.front()->cur_count,
                        ((uint64_t)main_env->handle));

        replay_sync_ops(true, main_env);
#endif
        if (this->time
            != std::chrono::time_point<std::chrono::high_resolution_clock>()) {
            auto end = std::chrono::high_resolution_clock::now();
            // get duration in us
            auto dur = std::chrono::duration_cast<std::chrono::microseconds>(
                end - this->time);
            this->time =
                std::chrono::time_point<std::chrono::high_resolution_clock>();
            LOG_DEBUG("Recover time: %f\n", dur.count() / 1000000.0);
            // put things back
        }
        invoke_main();
    }
}

#if WASM_ENABLE_LIB_PTHREAD != 0
void
WAMRInstance::spawn_child(WASMExecEnv *cur_env, bool main)
{
    static std::vector<WAMRExecEnv *>::iterator iter;
    static uint64 parent;
    if (main) {
        iter = ++(execEnv.begin());
        parent = 0;
    }
    //    invoke_init_c();
    //  Each thread needs it's own thread arg
    auto thread_arg = ThreadArgs{ cur_env };
    static std::mutex mtx;
    static std::condition_variable cv;
    std::unique_lock ul(mtx);

    while (iter != execEnv.end()) {
        // Get parent's virtual TID from child's OS TID
        if (parent == 0) {
            child_env = *iter;
            parent = child_env->cur_count;
            if (tid_start_arg_map.find(child_env->cur_count)
                != tid_start_arg_map.end()) {
                parent = tid_start_arg_map[parent].second;
            }
            parent = child_tid_map[parent];
            for (auto &[tid, vtid] : tid_start_arg_map) {
                if (vtid.second == (int)parent) {
                    parent = tid;
                    break;
                }
            }
            LOG_DEBUG("%lu %d", parent, child_env->cur_count);
        } // calculate parent TID once
        if (parent != ((uint64_t)cur_env->handle) && (parent != !main)) {
            cv.wait(ul);
            continue;
        }
        // requires to record the args and callback for the pthread.
        argptr[id] = &thread_arg;
        // restart thread execution
        LOG_DEBUG("pthread_create_wrapper, func %d\n", child_env->cur_count);
        // module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
        // error_buf, sizeof(error_buf));
        if (tid_start_arg_map.find(child_env->cur_count)
            != tid_start_arg_map.end()) {
            // find the parent env
            auto *saved_env = cur_env->restore_call_chain;
            cur_env->restore_call_chain = NULL;
            exec_env->is_restore = true;
            // main thread
            thread_spawn_wrapper(cur_env,
                                 tid_start_arg_map[child_env->cur_count].first);
            cur_env->restore_call_chain = saved_env;
            exec_env->is_restore = false;
        }
        else {
            exec_env->is_restore = true;
            pthread_create_wrapper(cur_env, nullptr, nullptr, id,
                                   id); // tid_map
        }
        fprintf(stderr, "child spawned %p %p\n\n", cur_env, child_env);
        // sleep(1);
        //        thread_init.acquire();
        // advance ptr
        ++iter;
        parent = 0;
        cv.notify_all();
    }
}
#endif

WASMFunction *
WAMRInstance::get_func()
{
    return static_cast<WASMFunction *>(func);
}
void
WAMRInstance::set_func(WASMFunction *f)
{
    func = static_cast<WASMFunction *>(f);
}
void
WAMRInstance::set_wasi_args(const std::vector<std::string> &dir_list,
                            const std::vector<std::string> &map_dir_list,
                            const std::vector<std::string> &env_list,
                            const std::vector<std::string> &arg_list,
                            const std::vector<std::string> &addr_list,
                            const std::vector<std::string> &ns_lookup_pool)
{

    dir_ = string_vec_to_cstr_array(dir_list);
    map_dir_ = string_vec_to_cstr_array(map_dir_list);
    env_ = string_vec_to_cstr_array(env_list);
    arg_ = string_vec_to_cstr_array(arg_list);
    addr_ = string_vec_to_cstr_array(addr_list);
    ns_pool_ = string_vec_to_cstr_array(ns_lookup_pool);

    wasm_runtime_set_wasi_args_ex(this->module_, dir_.data(), dir_.size(),
                                  map_dir_.data(), map_dir_.size(), env_.data(),
                                  env_.size(), const_cast<char **>(arg_.data()),
                                  arg_.size(), 0, 1, 2);

    wasm_runtime_set_wasi_addr_pool(module_, addr_.data(), addr_.size());
    wasm_runtime_set_wasi_ns_lookup_pool(module_, ns_pool_.data(),
                                         ns_pool_.size());
}
void
WAMRInstance::set_wasi_args(WAMRWASIArguments &context)
{
    set_wasi_args(context.dir, context.map_dir, context.env_list,
                  context.argv_list, context.addr_pool, context.ns_lookup_list);
}
extern WAMRInstance *wamr;
extern "C" { // stop name mangling so it can be linked externally
void
wamr_wait(wasm_exec_env_t exec_env)
{
    LOG_DEBUG("child getting ready to wait %p", ((void *)exec_env));
    thread_init.release(1);
#if WASM_ENABLE_LIB_PTHREAD != 0
    wamr->spawn_child(exec_env, false);
#endif
    LOG_DEBUG("finish child restore");
    wakeup.acquire();
#if WASM_ENABLE_LIB_PTHREAD != 0
    LOG_DEBUG("go child!! %lu", ((uint64_t)exec_env->handle));
    wamr->replay_sync_ops(false, exec_env);
    LOG_DEBUG("finish syncing");
#endif
    if (wamr->time
        != std::chrono::time_point<std::chrono::high_resolution_clock>()) {
        auto end = std::chrono::high_resolution_clock::now();
        // get duration in us
        auto dur = std::chrono::duration_cast<std::chrono::microseconds>(
            end - wamr->time);
        wamr->time =
            std::chrono::time_point<std::chrono::high_resolution_clock>();
        LOG_DEBUG("Recover time: %f\n", dur.count() / 1000000.0);
        // put things back
    }
    // finished restoring
    exec_env->is_restore = true;
    fprintf(stderr, "invoke side%p\n",
            ((WASMModuleInstance *)exec_env->module_inst)->global_data);
}

WASMExecEnv *
restore_env(WASMModuleInstanceCommon *module_inst)
{
    auto exec_env =
        wasm_exec_env_create_internal(module_inst, wamr->stack_size);
    restore(child_env, exec_env);

    auto s = exec_env->restore_call_chain;

    wamr->cur_thread = ((uint64_t)exec_env->handle);
    exec_env->is_restore = true;
    fprintf(stderr, "restore_env: %p %p\n", exec_env, s);

    return exec_env;
}
}

void
WAMRInstance::instantiate()
{
    module_inst = wasm_runtime_instantiate(module_, stack_size, heap_size,
                                           error_buf, sizeof(error_buf));
    if (!module_inst) {
        LOG_DEBUG("Instantiate wasm module failed. error: %s", error_buf);
        throw;
    }
    cur_env = exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
}

bool
is_ip_in_cidr(const char *base_ip, int subnet_mask_len, uint32_t ip)
{
    uint32_t base_ip_bin, subnet_mask, network_addr, broadcast_addr;
    LOG_DEBUG("base_ip: %d subnet_mask_len: %d", base_ip, subnet_mask_len);
    LOG_DEBUG("ip: %d.%d.%d.%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
              (ip >> 8) & 0xFF, ip & 0xFF);

    // Convert base IP to binary
    if (inet_pton(AF_INET, base_ip, &base_ip_bin) != 1) {
        fprintf(stderr, "Error converting base IP to binary\n");
        return false;
    }

    // Ensure that the subnet mask length is valid
    if (subnet_mask_len < 0 || subnet_mask_len > 32) {
        fprintf(stderr, "Invalid subnet mask length\n");
        return false;
    }

    // Calculate subnet mask in binary
    subnet_mask = htonl(~((1 << (32 - subnet_mask_len)) - 1));

    // Calculate network and broadcast addresses
    network_addr = base_ip_bin & subnet_mask;
    broadcast_addr = network_addr | ~subnet_mask;

    // Ensure ip is in network byte order
    uint32_t ip_net_order = htonl(ip);

    // Check if IP is within range
    return ip_net_order >= network_addr && ip_net_order <= broadcast_addr;
}
bool
is_ipv6_in_cidr(const char *base_ip_str, int subnet_mask_len,
                struct in6_addr *ip)
{
    struct in6_addr base_ip {
    }, subnet_mask{}, network_addr{}, ip_min{}, ip_max{};
    unsigned char mask;

    // Convert base IP to binary
    inet_pton(AF_INET6, base_ip_str, &base_ip);

    // Clear subnet_mask and network_addr
    memset(&subnet_mask, 0, sizeof(subnet_mask));
    memset(&network_addr, 0, sizeof(network_addr));

    // Create the subnet mask and network address
    for (int i = 0; i < subnet_mask_len / 8; i++) {
        subnet_mask.s6_addr[i] = 0xff;
    }
    if (subnet_mask_len % 8) {
        mask = (0xff << (8 - (subnet_mask_len % 8)));
        subnet_mask.s6_addr[subnet_mask_len / 8] = mask;
    }

    // Apply the subnet mask to the base IP to get the network address
    for (int i = 0; i < 16; i++) {
        network_addr.s6_addr[i] = base_ip.s6_addr[i] & subnet_mask.s6_addr[i];
    }

    // Calculate the first and last IPs in the range
    ip_min = network_addr;
    ip_max = network_addr;
    for (int i = 15; i >= subnet_mask_len / 8; i--) {
        ip_max.s6_addr[i] = 0xff;
    }

    // Check if IP is within range
    for (int i = 0; i < 16; i++) {
        if (ip->s6_addr[i] < ip_min.s6_addr[i]
            || ip->s6_addr[i] > ip_max.s6_addr[i]) {
            return false;
        }
    }
    return true;
}
void
serialize_to_file(WASMExecEnv *instance)
{
    // gateway
    auto start = std::chrono::high_resolution_clock::now();

#if WASM_ENABLE_LIB_PTHREAD != 0
    auto cluster = wasm_exec_env_get_cluster(instance);
    auto all_count = bh_list_length(&cluster->exec_env_list);
    // fill vector

    std::unique_lock as_ul(wamr->as_mtx);
    LOG_DEBUG("get lock");
    wamr->ready++;
    wamr->lwcp_list[((uint64_t)instance->handle)]++;
    if (wamr->ready == all_count) {
        wamr->should_snapshot = true;
    }
    // If we're not all ready
    LOG_DEBUG("thread %lu, with %d ready out of %d total",
              ((uint64_t)instance->handle), wamr->ready, all_count);
#endif
#if !defined(BH_PLATFORM_WINDOWS)
    if (!wamr->socket_fd_map_.empty() && wamr->should_snapshot) {
        // tell gateway to keep alive the server
        struct sockaddr_in addr {};
        int fd = 0;
        ssize_t rc;
        SocketAddrPool src_addr{};
        bool is_server = false;
        for (auto [tmp_fd, sock_data] : wamr->socket_fd_map_) {
            if (sock_data.is_server) {
                is_server = true;
                break;
            }
        }
        wamr->op_data.op =
            is_server ? MVVM_SOCK_SUSPEND_TCP_SERVER : MVVM_SOCK_SUSPEND;

        for (auto [tmp_fd, sock_data] : wamr->socket_fd_map_) {
            int idx = wamr->op_data.size;
            src_addr = sock_data.socketAddress;
            char tmp_ip4[INET_ADDRSTRLEN];
            char tmp_ip6[INET6_ADDRSTRLEN];
            snprintf(tmp_ip4, sizeof(tmp_ip4), "%u.%u.%u.%u", src_addr.ip4[0],
                     src_addr.ip4[1], src_addr.ip4[2], src_addr.ip4[3]);
            snprintf(tmp_ip6, sizeof(tmp_ip6), "%x:%x:%x:%x:%x:%x:%x:%x",
                     src_addr.ip6[0], src_addr.ip6[1], src_addr.ip6[2],
                     src_addr.ip6[3], src_addr.ip6[4], src_addr.ip6[5],
                     src_addr.ip6[6], src_addr.ip6[7]);
            if ((src_addr.is_4 && !strcmp(tmp_ip4, "0.0.0.0"))
                || (!src_addr.is_4 && !strcmp(tmp_ip6, "0:0:0:0:0:0:0:0"))) {
                src_addr = wamr->local_addr;
                src_addr.port = sock_data.socketAddress.port;
            }
            LOG_DEBUG("addr: %d %d.%d.%d.%d  port: %d", tmp_fd, src_addr.ip4[0],
                      src_addr.ip4[1], src_addr.ip4[2], src_addr.ip4[3],
                      src_addr.port);
            snprintf(tmp_ip4, sizeof(tmp_ip4), "%u.%u.%u.%u",
                     sock_data.socketSentToData.dest_addr.ip.ip4[0],
                     sock_data.socketSentToData.dest_addr.ip.ip4[1],
                     sock_data.socketSentToData.dest_addr.ip.ip4[2],
                     sock_data.socketSentToData.dest_addr.ip.ip4[3]);

            snprintf(tmp_ip6, sizeof(tmp_ip6), "%x:%x:%x:%x:%x:%x:%x:%x",
                     sock_data.socketSentToData.dest_addr.ip.ip6[0],
                     sock_data.socketSentToData.dest_addr.ip.ip6[1],
                     sock_data.socketSentToData.dest_addr.ip.ip6[2],
                     sock_data.socketSentToData.dest_addr.ip.ip6[3],
                     sock_data.socketSentToData.dest_addr.ip.ip6[4],
                     sock_data.socketSentToData.dest_addr.ip.ip6[5],
                     sock_data.socketSentToData.dest_addr.ip.ip6[6],
                     sock_data.socketSentToData.dest_addr.ip.ip6[7]);
            if (!strcmp(tmp_ip4, "0.0.0.0")
                || !strcmp(tmp_ip6, "0:0:0:0:0:0:0:0")) {
                if (!wamr->op_data.is_tcp) {
                    if ((sock_data.socketSentToData.dest_addr.ip.is_4
                         && !strcmp(tmp_ip4, "0.0.0.0"))
                        || (!sock_data.socketSentToData.dest_addr.ip.is_4
                            && !strcmp(tmp_ip6, "0:0:0:0:0:0:0:0"))) {
                        wamr->op_data.addr[idx][1].is_4 =
                            sock_data.socketRecvFromDatas[0].src_addr.ip.is_4;
                        std::memcpy(
                            wamr->op_data.addr[idx][1].ip4,
                            sock_data.socketRecvFromDatas[0].src_addr.ip.ip4,
                            sizeof(sock_data.socketRecvFromDatas[0]
                                       .src_addr.ip.ip4));
                        std::memcpy(
                            wamr->op_data.addr[idx][1].ip6,
                            sock_data.socketRecvFromDatas[0].src_addr.ip.ip6,
                            sizeof(sock_data.socketRecvFromDatas[0]
                                       .src_addr.ip.ip6));
                        wamr->op_data.addr[idx][1].port =
                            sock_data.socketRecvFromDatas[0].src_addr.port;
                    }
                    else {
                        wamr->op_data.addr[idx][1].is_4 =
                            sock_data.socketSentToData.dest_addr.ip.is_4;
                        std::memcpy(
                            wamr->op_data.addr[idx][1].ip4,
                            sock_data.socketSentToData.dest_addr.ip.ip4,
                            sizeof(
                                sock_data.socketSentToData.dest_addr.ip.ip4));
                        std::memcpy(
                            wamr->op_data.addr[idx][1].ip6,
                            sock_data.socketSentToData.dest_addr.ip.ip6,
                            sizeof(
                                sock_data.socketSentToData.dest_addr.ip.ip6));
                        wamr->op_data.addr[idx][1].port =
                            sock_data.socketSentToData.dest_addr.port;
                    }
                }
                else {
                    // if it's not socket
                    if (!is_server) {
                        int tmp_fd_ = 0;
                        unsigned int size_ = sizeof(sockaddr_in);
                        sockaddr_in *ss = (sockaddr_in *)malloc(size_);
                        wamr->invoke_sock_getsockname(tmp_fd_, (sockaddr **)&ss,
                                                      &size_);
                        if (ss->sin_family == AF_INET) {
                            auto *ipv4 = (struct sockaddr_in *)ss;
                            uint32_t ip = ntohl(ipv4->sin_addr.s_addr);
                            wamr->op_data.addr[idx][1].is_4 = true;
                            wamr->op_data.addr[idx][1].ip4[0] =
                                (ip >> 24) & 0xFF;
                            wamr->op_data.addr[idx][1].ip4[1] =
                                (ip >> 16) & 0xFF;
                            wamr->op_data.addr[idx][1].ip4[2] =
                                (ip >> 8) & 0xFF;
                            wamr->op_data.addr[idx][1].ip4[3] = ip & 0xFF;
                            wamr->op_data.addr[idx][1].port =
                                ntohs(ipv4->sin_port);
                        }
                        else {
                            auto *ipv6 = (struct sockaddr_in6 *)ss;
                            wamr->op_data.addr[idx][1].is_4 = false;
                            const auto *bytes =
                                (const uint8_t *)ipv6->sin6_addr.s6_addr;
                            for (int i = 0; i < 16; i += 2) {
                                wamr->op_data.addr[idx][1].ip6[i / 2] =
                                    (bytes[i] << 8) + bytes[i + 1];
                            }
                            wamr->op_data.addr[idx][1].port =
                                ntohs(ipv6->sin6_port);
                        }
                        free(ss);
                    }
                    else if (sock_data.is_server) {
                        wamr->op_data.size--;
                    }
                }
            }
            LOG_DEBUG("dest_addr: %d.%d.%d.%d:%d",
                      wamr->op_data.addr[idx][1].ip4[0],
                      wamr->op_data.addr[idx][1].ip4[1],
                      wamr->op_data.addr[idx][1].ip4[2],
                      wamr->op_data.addr[idx][1].ip4[3],
                      wamr->op_data.addr[idx][1].port);
            wamr->op_data.size += 1;
        }
        // Create a socket
        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            LOG_DEBUG("socket error");
            throw std::runtime_error("socket error");
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
            LOG_DEBUG("Connection Failed %d", errno);
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
#endif
#if WASM_ENABLE_LIB_PTHREAD != 0
    if (wamr->ready < all_count) {
        // Then wait for someone else to get here and finish the job
        std::condition_variable as_cv;
        as_cv.wait(as_ul);
    }
    wasm_cluster_suspend_all_except_self(cluster, instance);
    auto elem = (WASMExecEnv *)bh_list_first_elem(&cluster->exec_env_list);
    while (elem) {
        instance = elem;
#endif // windows has no threads so only does it once
        auto a = new WAMRExecEnv();
        dump(a, instance);
        as.emplace_back(a);
#if WASM_ENABLE_LIB_PTHREAD != 0
        elem = (WASMExecEnv *)bh_list_elem_next(elem);
    }
    // finish filling vector
#endif
    struct_pack::serialize_to(*writer, as);

    auto end = std::chrono::high_resolution_clock::now();
    // get duration in us
    auto dur =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    LOG_DEBUG("Snapshot time: %f s", dur.count() / 1000000.0);
    exit(EXIT_SUCCESS);
}

namespace fs = std::filesystem;

bool
isValidPath(char *path)
{
    try {
        fs::path p(path);
        fs::path parentPath = p.parent_path();
        return fs::exists(parentPath);
    } catch (const fs::filesystem_error &) {
        return false;
    }
}

WASM_RUNTIME_API_EXTERN bool
wasm_runtime_checkpoint(wasm_module_inst_t module_inst, char *file)
{
    wamr = new WAMRInstance(file,
                            wasm_runtime_get_exec_env_singleton(module_inst));
    char protocol[10];
    char address[16];
    int port;
    register_sigtrap();
    register_sigint();
    wamr->get_int3_addr();
    wamr->replace_int3_with_nop();
    wamr->replace_mfence_with_nop();
    if (isValidPath(file)) {
        writer = new FwriteStream(file);
    }
    else {
        // parse tcp://0.0.0.0:1234 from file
        if (sscanf(file, "%8[^:]://%15[^:]:%d", protocol, address, &port)) {
            writer = new SocketWriteStream(address, port);
        }
    }
    wamr->invoke_main();
    return true;
};
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_restore(wasm_module_inst_t module_inst, char *file, char *file1)
{
    wamr = new WAMRInstance(file,
                            wasm_runtime_get_exec_env_singleton(module_inst));
    register_sigtrap();
    register_sigint();
    wamr->get_int3_addr();
    wamr->replace_int3_with_nop();
    wamr->replace_mfence_with_nop();
    char protocol[10];
    char address[16];
    int port;
    if (isValidPath(file)) {
        reader = new FreadStream(file);
    }
    else {
        // parse tcp://0.0.0.0:1234 from file
        if (sscanf(file, "%8[^:]://%15[^:]:%d", protocol, address, &port)) {
            reader = new SocketReadStream(address, port);
        }
    }

    auto a =
        struct_pack::deserialize<std::vector<std::unique_ptr<WAMRExecEnv>>>(
            *reader)
            .value();

    if (isValidPath(file1)) {
        reader = new FreadStream(file1);
    }
    else {
        // parse tcp://0.0.0.0:1234 from file1
        if (sscanf(file1, "%8[^:]://%15[^:]:%d", protocol, address, &port)) {
            reader = new SocketReadStream(address, port);
        }
    }
#if defined(BH_PLATFORM_LINUX)
    if (!a[a.size() - 1]
             ->module_inst.wasi_ctx.socket_fd_map
             .empty()) { // new ip, old ip // only if tcp requires keepalive
        // tell gateway to stop keep alive the server
        struct sockaddr_in addr {};
        int fd = 0;
        bool is_tcp_server;
        SocketAddrPool src_addr = wamr->local_addr;
        LOG_DEBUG("new ip %d.%d.%d.%d:%d", src_addr.ip4[0], src_addr.ip4[1],
                  src_addr.ip4[2], src_addr.ip4[3], src_addr.port);
        // got from wamr
        for (auto &[_, socketMetaData] :
             a[a.size() - 1]->module_inst.wasi_ctx.socket_fd_map) {
            wamr->op_data.is_tcp |= socketMetaData.type;
            is_tcp_server |= socketMetaData.is_server;
        }
        is_tcp_server &= wamr->op_data.is_tcp;

        wamr->op_data.op =
            is_tcp_server ? MVVM_SOCK_RESUME_TCP_SERVER : MVVM_SOCK_RESUME;
        wamr->op_data.addr[0][0] = src_addr;

        // Create a socket
        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            LOG_ERROR("socket error");
            throw std::runtime_error("socket error");
        }
        addr.sin_family = AF_INET;
        addr.sin_port = htons(MVVM_SOCK_PORT);

        // Convert IPv4 and IPv6 addresses from text to binary form
        if (inet_pton(AF_INET, MVVM_SOCK_ADDR, &addr.sin_addr) <= 0) {
            LOG_ERROR("Invalid address/ Address not supported");
            return false;
        }

        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
            LOG_ERROR("Connection Failed %d", errno);
            return false;
        }
        // send the fd

        if (send(fd, &wamr->op_data, sizeof(struct mvvm_op_data), 0) == -1) {
            LOG_ERROR("Send Error");
            return false;
        }
        close(fd);
        LOG_ERROR("sent the resume signal");
    }
#endif
    wamr->recover(&a);
    return true;
};
