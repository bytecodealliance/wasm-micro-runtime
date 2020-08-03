// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT

#include <array>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>
#ifdef __unix__
#  include <dlfcn.h>
#  include <err.h>
#  include <fcntl.h>
#  include <libgen.h>
#  include <pthread.h>
#  include <stdio.h>
#  include <sys/mman.h>
#  include <sys/socket.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <sys/un.h>
#  include <sys/wait.h>
#  include <unistd.h>
#  ifdef USE_CAPSICUM
#    include <sys/capsicum.h>
#  endif
#  ifdef USE_KQUEUE_PROCDESC
#    include <sys/procdesc.h>
#  endif
#  if defined(USE_KQUEUE) || defined(USE_KQUEUE_PROCDESC)
#    include <sys/event.h>
#  endif
#  ifndef USE_KQUEUE
#    include <poll.h>
#    ifndef INFTIM
#      define INFTIM -1
#    endif
#  endif
#  ifdef __linux__
#    include <bsd/unistd.h>
#    define MAP_ALIGNED(x) 0
#    define MAP_NOCORE 0
#  endif
#endif
#include "sandbox.hh"

extern "C"
{
  /**
   * The `environ` symbol is exported by libc, but not exposed in any header.
   *
   * This should go away once we are constructing a properly sanitised
   * environment for the child.
   */
  extern char** environ;
}

#include "pal/pal_consts.h"
#include "pal/pal_freebsd.h"
#include "pal/pal_linux.h"
#include "pal/pal_windows.h"

namespace sandbox
{
  using DefaultPal =
#if defined(_WIN32)
    snmalloc::PALWindows;
#elif defined(__linux__)
    snmalloc::PALLinux;
#elif defined(__FreeBSD__)
    snmalloc::PALFreeBSD;
#else
#  error No platform abstraction layer available.
#endif

  struct MemoryProviderBumpPointerState
  {
    /**
     * Features that we provide.  We do alignment here rather than using the
     * more general code because this lets us pack the allocations a little bit
     * more densely.  Most of the time we'll be asked for the same sized
     * aligned chunks, so we don't need to insert padding.  The general code
     * assumes that address space is cheap so asking for twice as much as you
     * need and returning half of it is fine.
     */
    static constexpr uint64_t pal_features =
      snmalloc::AlignedAllocation | snmalloc::LazyCommit;
    /**
     * Flag indicating whether it's safe to use virtual memory functions for
     * zeroing.
     */
    static const bool UsePageZeroing = false;
    /**
     * Flag indicating whether our version of reserve should be used in place
     * of the default.
     */
    static const bool CustomReserve = true;
    /**
     * The pointer to the start of the shared heap range.  This is updated by
     * allocators inside and outside of the shared region and so must be
     * atomic.
     */
    std::atomic<uintptr_t> shared_heap_range_start;
    /**
     * The end of the shared heap range.
     *
     * FIXME: This is fixed when the shared memory region is created and so
     * shouldn't be mutable by the code in the sandbox, but is currently.
     */
    uintptr_t shared_heap_range_end;
    /**
     * Flag indicating that this memory provider is initialised.  If it is not
     * yet initialised, then it should request some memory for its buffer.
     *
     * Note: We depend on the fact that this is zero-initialised in globals.
     * The constructor for this class does nothing, but during process start
     * this can be called before the constructor for the enclosing class has
     * run.  We then fill in the value of the other fields.
     */
    bool isInitialised;
    /**
     * Inherit the default page size from the system PAL.
     */
    static constexpr size_t page_size = DefaultPal::page_size;
    /**
     * Initialise with a specific range.  If this has been called, then the
     */
    void set_range(uintptr_t start, uintptr_t end)
    {
      shared_heap_range_start = start;
      shared_heap_range_end = end;
      isInitialised = true;
    }
    /**
     * Custom replacement for reserve.  Allocates using a simple
     * bump-the-pointer algorithm and relies on the allocator code to handle
     * address-space reuse.
     */
    template<bool committed>
    void* reserve(size_t size, size_t align)
    {
      if (!isInitialised)
      {
        static size_t bootstrap_heap_size = 1024 * 1024 * 512;
        void* bootstrap_heap =
          DefaultPal().reserve<true>(bootstrap_heap_size, page_size);
        shared_heap_range_start = reinterpret_cast<uintptr_t>(bootstrap_heap);
        shared_heap_range_end =
          reinterpret_cast<uintptr_t>(bootstrap_heap) + bootstrap_heap_size;
        isInitialised = true;
      }
      size_t align1 = align - 1;
      size_t mask = ~align1;

      uintptr_t start = shared_heap_range_start.load(std::memory_order_relaxed);
      uintptr_t rounded_start;
      uintptr_t end;
      do
      {
        rounded_start = start;
        if ((rounded_start & align1) != 0)
        {
          rounded_start += align;
          rounded_start &= mask;
        }
        end = rounded_start + size;
      } while (!shared_heap_range_start.compare_exchange_strong(start, end));
      if (end > shared_heap_range_end)
      {
        return nullptr;
      }
      return reinterpret_cast<void*>(rounded_start);
    }
    /**
     * Zero some memory.  Ideally, we'd ask the kernel to replace these pages
     * with fresh zeroed ones, but unfortunately we can't do that with shared
     * memory pages.
     */
    template<bool page_aligned = false>
    void zero(void* p, size_t size) noexcept
    {
      bzero(p, size);
    }

    /**
     * Report an error.
     */
    [[noreturn]] static void error(const char* const str)
    {
      puts(str);
      abort();
    }

    /**
     * Notify the kernel that we're not using these pages and don't care if it
     * discards the contents, until the next store to these pages.
     */
    void notify_not_using(void* p, size_t size) noexcept
    {
      madvise(p, size, MADV_FREE);
    }

    /**
     * Notify the kernel that we are using these pages.  This is a no-op on
     * FreeBSD - the next store provides the notification.
     */
    template<snmalloc::ZeroMem zero_mem>
    void notify_using(void* p, size_t size) noexcept
    {
      if (zero_mem == snmalloc::YesZero)
        zero(p, size);
    }
  };
}

#define SNMALLOC_MEMORY_PROVIDER sandbox::MemoryProviderBumpPointerState
#include "pal/pal.h"
#pragma mark -
#include "mem/allocconfig.h"
#include "snmalloc.h"

/**
 * Check that the superslab size is what we expect.  If this changes, then we
 * may need to modify how we interact with the pagemap.
 */
static_assert(snmalloc::SUPERSLAB_SIZE == 1 << 24);

namespace
{
  /**
   * The memory provider for the shared region.  This bump allocates from the
   * shared memory segment.
   */
  using SharedMemoryProvider = snmalloc::MemoryProviderStateMixin<
    snmalloc::PALPlainMixin<sandbox::MemoryProviderBumpPointerState>>;
  /**
   * Singleton class that handles pagemap updates from children.  This listens
   * on a socket for updates, validates that they correspond to the memory that
   * this child is responsible for, and if so updates both that child's shared
   * pagemap page and the parent process's pagemap.
   *
   * This class creates a new thread in the background that waits for pagemap
   * updates and processes them.
   */
  class PagemapOwner
  {
    snmalloc::DefaultChunkMap<snmalloc::ExternalGlobalPagemap> pm;
#ifdef USE_KQUEUE
    /**
     * The kqueue used to wait for pagemap updates from the child.  Multiple
     * threads can safely update a kqueue concurrently, so we don't need any
     * other synchronisation in this page.
     */
    int kq = kqueue();
    /**
     * Add a new socket that we'll wait for.  This can be called from any
     * thread without synchronisation.
     */
    void register_fd(int socket_fd)
    {
      struct kevent event;
      EV_SET(&event, socket_fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
      if (kevent(kq, &event, 1, nullptr, 0, nullptr) == -1)
      {
        err(1, "Setting up kqueue");
      }
    }
    /**
     * Wait for an update from a child process.  This blocks and returns true
     * if there is a message, false if an error occurred.  On success, `fd`
     * will be set to the file descriptor associated with the event and `eof`
     * will be set to true if the socket has been closed at the remote end,
     * false otherwise.
     *
     * This is called only by the thread spawned by this class.
     */
    bool poll(int& fd, bool& eof)
    {
      struct kevent event;
      if (kevent(kq, nullptr, 0, &event, 1, nullptr) == -1)
      {
        return false;
      }
      fd = static_cast<int>(event.ident);
      eof = (event.flags & EV_EOF) == EV_EOF;
      return true;
    }
#elif defined(__unix)
    /**
     * Mutex that protects the metadata about registered file descriptors.
     */
    std::mutex fds_lock;
    /**
     * Vector of all file descriptors that we're waiting for.
     */
    std::vector<int> fds;
    /**
     * Map from file descriptor number to index in the vector.  This is used to
     * quickly find the correct entry in the vector for deletion.
     */
    std::unordered_map<int, int> fd_indexes;
    /**
     * Wrapper around a pair of fie descriptors defining endpoints of a pipe.
     * We use this to fall out of a `poll` call when the file descriptors
     * vector has been updated.
     */
    struct pipepair
    {
      /**
       * The read end of the pipe (by convention - the pipe is bidirectional).
       */
      int in;
      /**
       * The write end of the pipe (by convention - the pipe is bidirectional).
       */
      int out;
      /**
       * Open the pipe and initialise the descriptors.
       */
      pipepair()
      {
        int p[2];
        pipe(p);
        in = p[1];
        out = p[0];
      }
      /**
       * Destroy the pipe by closing both descriptors.
       */
      ~pipepair()
      {
        close(in);
        close(out);
      }
    } pipes;
    /**
     * Register a file descriptor.  This may be called from any thread.
     */
    void register_fd(int socket_fd)
    {
      // With the lock held, add this to our bookkeeping metadata.
      {
        std::lock_guard g(fds_lock);
        fd_indexes[socket_fd] = fds.size();
        fds.push_back(socket_fd);
      }
      // Prod the polling end to wake up.
      write(pipes.out, " ", 1);
    }
    std::queue<pollfd> ready_fds;
    /**
     * Wait for one of the events to be ready.
     */
    bool poll(int& fd, bool& eof)
    {
      // Put everything in a nested scope so that all destructors are run
      // before the tail call.  This allows the compiler to perform tail-call
      // elimination.
      {
        // Check if there's a cached result from a previous poll call and
        // return it if so.
        auto check_ready = [&]() {
          if (!ready_fds.empty())
          {
            auto back = ready_fds.front();
            fd = back.fd;
            eof = (back.revents & POLLHUP);
            ready_fds.pop();
            return true;
          }
          return false;
        };
        if (check_ready())
        {
          return true;
        }
        // Construct the vector of pollfd structures.
        std::vector<pollfd> pfds;
        // Add the pipe so that we can be interrupted if the list of fds
        // changes.
        pfds.push_back({pipes.in, POLLRDNORM, 0});
        {
          std::lock_guard g(fds_lock);
          for (int i : fds)
          {
            pfds.push_back({i, POLLRDNORM, 0});
          }
        }
        if (::poll(pfds.data(), pfds.size(), INFTIM) == -1)
        {
          return false;
        }
        for (auto& pfd : pfds)
        {
          // If the pipe has some data, read it out so that we don't get woken
          // up again next time.
          if ((pfd.fd == pipes.in) && (pfd.revents & POLLRDNORM))
          {
            char buf[1];
            read(pipes.in, buf, 1);
            // Don't hand this one out to the caller!
            continue;
          }
          // If we found one, push it into the cached list.
          if (pfd.revents != 0)
          {
            ready_fds.push(pfd);
          }
          // If we found an fd that is closed, do cache it so we can send the
          // eof result to the caller, but also remove it so that we don't try
          // to poll for this one again.
          if (pfd.revents & POLLHUP)
          {
            std::lock_guard g(fds_lock);
            int idx = fd_indexes[pfd.fd];
            fd_indexes.erase(pfd.fd);
            assert(fds[idx] == pfd.fd);
            fds.erase(fds.begin() + idx);
          }
        }
        if (check_ready())
        {
          return true;
        }
      }
      // If we were woken up by the pipe and didn't have any other
      // notifications, try again.
      return poll(fd, eof);
    }
#else
#  error Event polling not implemented for this target.
#endif
    /**
     * Mutex that protects the `ranges` map.
     */
    std::mutex m;
    /**
     * Metadata about a sandbox for which we are updating the page map.
     */
    struct Sandbox
    {
      /**
       * The range of address space owned by this sandbox.
       */
      std::pair<size_t, size_t> range;
      /**
       * The shared pagemap page that we need to update on behalf of this
       * process.
       */
      uint8_t* shared_page = nullptr;
    };
    /**
     * A map from file descriptor over which we've received an update request
     * to the sandbox metadata.
     */
    std::unordered_map<int, Sandbox> ranges;
    /**
     * Run loop.  Wait for updates from the child.
     */
    void run()
    {
      int fd;
      bool eof;
      while (poll(fd, eof))
      {
        // If a child's socket closed, unmap its shared page and delete the
        // metadata that we have associated with it.
        if (eof)
        {
          std::lock_guard g(m);
          auto r = ranges.find(fd);
          munmap(r->second.shared_page, snmalloc::OS_PAGE_SIZE);
          ranges.erase(r);
          close(fd);
          continue;
        }
        uint64_t update;
        if (read(fd, static_cast<void*>(&update), sizeof(uint64_t)) != 8)
        {
          err(1, "Read from pagemap update socket %d failed", fd);
        }
        size_t position = update & ~0xffff;
        char value = update & 0xff;
        uint8_t isBig = (update & 0xff00) >> 8;
        validate_and_insert(fd, position, isBig, value);
        std::atomic_thread_fence(std::memory_order_seq_cst);
      }
      err(1, "Waiting for pagetable updates failed");
    }
    /**
     * Validate a request from the sandbox to update a pagemap and insert it if
     * allowed.  The `sender` parameter is the file descriptor over which the
     * message was sent. The `position` parameter is the address of the memory
     * for which the corresponding pagemap entry is to be updated.  For the
     * update to succeed, this must be within the range owned by the sandbox
     * identified by the sending socket.  The last two parameters indicate
     * whether this is a large allocation (one that spans multiple pagemap
     * entries) and the value.  If `isBig` is 0, then this is a simple update
     * of a single pagemap entry (typically a slab), specified in the `value`
     * parameter.  Otherwise, `value` is the base-2 logarithm of the size,
     * which is either being set or cleared (`isBig` values of 1 and 2,
     * respectively).
     */
    void validate_and_insert(
      int sender, size_t position, uint8_t isBig, uint8_t value)
    {
      decltype(ranges)::iterator r;
      Sandbox s;
      {
        std::lock_guard g(m);
        r = ranges.find(sender);
        if (r == ranges.end())
        {
          return;
        }
        s = r->second;
      }
      auto range = s.range;
      if ((position < range.first) || (position >= range.second))
      {
        return;
      }
      auto p = reinterpret_cast<uintptr_t>(position);
      snmalloc::ChunkmapPagemap& cpm =
        snmalloc::ExternalGlobalPagemap::pagemap();
      size_t index = cpm.index_for_address(p);
      size_t entries = 1;
      bool safe = true;
      auto check_large_update = [&]() {
        size_t alloc_size = (1ULL << value);
        entries = alloc_size / snmalloc::SUPERSLAB_SIZE;
        // FIXME: Check this for off-by-one errors!
        return (alloc_size + position <= range.second);
      };
      switch (isBig)
      {
        default:
          fprintf(
            stderr,
            "Invalid pagemap update received from sandbox %d\n",
            sender);
          break;
        case 0:
          // FIXME: Check that this is a valid small update size
          cpm.set(p, value);
          break;
        case 1:
          if ((safe = check_large_update()))
          {
            pm.set_large_size(reinterpret_cast<void*>(p), 1ULL << value);
          }
          break;
        case 2:
          if ((safe = check_large_update()))
          {
            pm.clear_large_size(reinterpret_cast<void*>(p), 1ULL << value);
          }
      }
      if (safe)
      {
        for (size_t i = 0; i < entries; i++)
        {
          s.shared_page[index + i] = pm.get(
            reinterpret_cast<void*>(position + (i * snmalloc::SUPERSLAB_SIZE)));
        }
      }
      else
      {
        printf("Rejecting page map update\n");
      }
    }

  public:
    /**
     * Constructor.  Spawns a background thread to run and process updates.
     */
    PagemapOwner()
    {
#ifdef USE_KQUEUE
      assert(kq >= 0);
#endif
      std::thread t([&]() { run(); });
      t.detach();
    }
    /**
     * Notify this class that a sandbox exists.  The `start` and `end`
     * parameters indicate the address range assigned to this sandbox.
     * `socket_fd` provides the file descriptor for the socket over which the
     * sandbox will send update requests.  `pagemap_fd` is the shared pagemap
     * page.
     */
    uint8_t* add_range(size_t start, size_t end, int socket_fd, int pagemap_fd)
    {
      uint8_t* shared_pagemap = static_cast<uint8_t*>(mmap(
        nullptr,
        snmalloc::OS_PAGE_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        pagemap_fd,
        0));
      if (shared_pagemap == MAP_FAILED)
      {
        err(1, "Failed to map shared pagemap page");
      }
      {
        std::lock_guard g(m);
        ranges[socket_fd] = {{start, end}, shared_pagemap};
      }
      register_fd(socket_fd);
      return shared_pagemap;
    }
  };
  /**
   * Return a singleton instance of the pagemap owner.
   */
  PagemapOwner& pagemap_owner()
  {
    // Leaks.  No need to run the destructor!
    static PagemapOwner* p = new PagemapOwner();
    return *p;
  }
}

namespace sandbox
{
  /**
   * Adaptor for allocators in the shared region to update the pagemap.
   * These treat the global pagemap in the process as canonical but also
   * update the pagemap in the child whenever the parent allocates within the
   * shared region.
   */
  struct SharedPagemapAdaptor
  {
    /**
     * Interface to the global pagemap.  Used to update the global pagemap and
     * to query values to propagate to the child process.
     */
    snmalloc::DefaultChunkMap<snmalloc::ExternalGlobalPagemap> global_pagemap;
    /**
     * The page in the child process that will be mapped into its pagemap.  Any
     * slab allocations by the parent must be propagated into this page.
     */
    uint8_t* shared_page;

    /**
     * Constructor.  Takes a shared pagemap page that this adaptor will update
     * in addition to updating the global pagemap.
     */
    SharedPagemapAdaptor(uint8_t* p) : shared_page(p) {}

    /**
     * Update the child, propagating `entries` entries from the global pagemap
     * into the shared pagemap region.
     */
    void update_child(uintptr_t p, size_t entries = 1)
    {
      snmalloc::ChunkmapPagemap& cpm =
        snmalloc::ExternalGlobalPagemap::pagemap();
      size_t index = cpm.index_for_address(p);
      for (size_t i = 0; i < entries; i++)
      {
        shared_page[index + i] =
          global_pagemap.get(p + (i * snmalloc::SUPERSLAB_SIZE));
      }
    }
    /**
     * Accessor.  We treat the global pagemap as canonical, so only look values
     * up here.
     */
    uint8_t get(uintptr_t p)
    {
      return global_pagemap.get(p);
    }
    /**
     * Set a superslab entry in the pagemap.  Inserts it into the global
     * pagemap and then propagates to the child.
     */
    void set_slab(snmalloc::Superslab* slab)
    {
      global_pagemap.set_slab(slab);
      update_child(reinterpret_cast<uintptr_t>(slab));
    }
    /**
     * Clear a superslab entry in the pagemap.  Removes it from the global
     * pagemap and then propagates to the child.
     */
    void clear_slab(snmalloc::Superslab* slab)
    {
      global_pagemap.clear_slab(slab);
      update_child(reinterpret_cast<uintptr_t>(slab));
    }
    /**
     * Clear a medium slab entry in the pagemap.  Removes it from the global
     * pagemap and then propagates to the child.
     */
    void clear_slab(snmalloc::Mediumslab* slab)
    {
      global_pagemap.clear_slab(slab);
      update_child(reinterpret_cast<uintptr_t>(slab));
    }
    /**
     * Set a medium slab entry in the pagemap.  Inserts it into the global
     * pagemap and then propagates to the child.
     */
    void set_slab(snmalloc::Mediumslab* slab)
    {
      global_pagemap.set_slab(slab);
      update_child(reinterpret_cast<uintptr_t>(slab));
    }
    /**
     * Set a large entry in the pagemap.  Inserts it into the global
     * pagemap and then propagates to the child.
     */
    void set_large_size(void* p, size_t size)
    {
      global_pagemap.set_large_size(p, size);
      size_t entries = size / snmalloc::SUPERSLAB_SIZE;
      update_child(reinterpret_cast<uintptr_t>(p), entries);
    }
    /**
     * Clear a large entry in the pagemap.  Removes it from the global
     * pagemap and then propagates to the child.
     */
    void clear_large_size(void* p, size_t size)
    {
      global_pagemap.clear_large_size(p, size);
      size_t entries = size / snmalloc::SUPERSLAB_SIZE;
      update_child(reinterpret_cast<uintptr_t>(p), entries);
    }
  };

  /**
   * Class representing a view of a shared memory region.  This provides both
   * the parent and child views of the region.
   */
  struct SharedMemoryRegion
  {
    // FIXME: The parent process can currently blindly follow pointers in these
    // regions.  We should explicitly mask all pointers against the size of the
    // allocation when we use them from outside.
    /**
     * The memory provider associated with this region.  This is responsible
     * for allocating pages within the shared range.  It lives within the
     * shared region because it can be called from both inside and outside of
     * the sandbox.
     */
    SharedMemoryProvider memory_provider;
    /**
     * A flag indicating that the parent has instructed the sandbox to exit.
     */
    std::atomic<bool> should_exit = false;
    /**
     * The index of the function currently being called.  This interface is not
     * currently reentrant.
     */
    int function_index;
    /**
     * A pointer to the tuple (in the shared memory range) that contains the
     * argument frame provided by the sandbox caller.
     */
    void* msg_buffer = nullptr;
    /**
     * The message queue for the parent's allocator.  This is stored in the
     * shared region because the child must be able to free memory allocated by
     * the parent.
     */
    snmalloc::RemoteAllocator allocator_state;
#ifdef __unix__
    /**
     * Mutex used to protect `cv`.
     */
    pthread_mutex_t mutex;
    /**
     * The condition variable that the child sleeps on when waiting for
     * messages from the parent.
     */
    pthread_cond_t cv;
    /**
     * Flag indicating whether the child is executing.  Set on startup and
     */
    std::atomic<bool> is_child_executing = false;
#endif
    /**
     * Waits until the `is_child_executing` flag is in the `expected` state.
     * This is used to wait for the child to start and to stop.
     */
    void wait(bool expected);
    /**
     * Wait until the `is_child_executing` flag is in the `expected` state.
     * Returns true if the condition was met or false if the timeout was
     * exceeded before the child entered the desired state.
     */
    bool wait(bool expected, struct timespec timeout);
    /**
     * Update the `is_child_executing` flag and wake up any waiters.  Note that
     * the `wait` functions will only unblock if `is_child_executing` is
     * modified using this function.
     */
    void signal(bool new_state);
    /**
     * Constructor.  Initialises the mutex and condition variables.
     */
    SharedMemoryRegion();
    /**
     * Destroy this shared memory region.  Unmaps the region.  Nothing in the
     * `SharedMemoryRegion` structure is trusted and so this takes the `size`
     * of the region as an explicit argument.  The parent is responsible for
     * tracking this value in trusted memory.
     */
    void destroy(size_t size);
  };

#ifdef __unix__
  void SharedMemoryRegion::wait(bool expected)
  {
    pthread_mutex_lock(&mutex);
    while (expected != is_child_executing)
    {
      pthread_cond_wait(&cv, &mutex);
    }
    pthread_mutex_unlock(&mutex);
  }

  bool SharedMemoryRegion::wait(bool expected, struct timespec timeout)
  {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    long nsec;
    time_t carry = __builtin_add_overflow(now.tv_nsec, timeout.tv_nsec, &nsec);
    timeout.tv_nsec = nsec;
    timeout.tv_sec += now.tv_sec + carry;
    pthread_mutex_lock(&mutex);
    pthread_cond_timedwait(&cv, &mutex, &timeout);
    bool ret = expected == is_child_executing;
    pthread_mutex_unlock(&mutex);
    return ret;
  }

  void SharedMemoryRegion::signal(bool new_state)
  {
    pthread_mutex_lock(&mutex);
    is_child_executing = new_state;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&mutex);
  }

  SharedMemoryRegion::SharedMemoryRegion()
  {
    pthread_mutexattr_t mattrs;
    pthread_mutexattr_init(&mattrs);
    pthread_mutexattr_setpshared(&mattrs, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex, &mattrs);
    pthread_condattr_t cvattrs;
    pthread_condattr_init(&cvattrs);
    pthread_condattr_setpshared(&cvattrs, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setclock(&cvattrs, CLOCK_MONOTONIC);
    pthread_cond_init(&cv, &cvattrs);
  }

  void SharedMemoryRegion::destroy(size_t size)
  {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cv);
    munmap(static_cast<void*>(this), size);
  }
#else
#  error Missing implementation of SharedMemoryRegion
#endif

#ifdef __unix__
  SandboxedLibrary::~SandboxedLibrary()
  {
    wait_for_child_exit();
#  if 0
    void* shared_pagemap_chunk =
      snmalloc::global_pagemap.page_for_address(shared_mem);
    // Reset our pagemap entries to be on-shared zeroes.  This is done before
    // destroying the sandbox, because at this point we should not have any
    // pointers into the shared memory region, but we can potentially assign to
    // this pagemap region as soon as the main region is unmapped.
    void* shared_pagemap = mmap(
      shared_pagemap_chunk,
      4096,
      PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANON | MAP_FIXED | MAP_NOCORE,
      -1,
      0);
    assert(shared_pagemap != MAP_FAILED);
    (void)shared_pagemap;
#  endif
    shared_mem->destroy(shared_size);
    close(shm_fd);
    close(socket_fd);
#  ifdef USE_KQUEUE_PROCDESC
    close(kq);
#  endif
  }

  /**
   * The numbers for file descriptors passed into the child.  These must match
   * between libsandbox and the library runner child process.
   */
  enum SandboxFileDescriptors
  {
    /**
     * The file descriptor used for the shared memory object that contains the
     * shared heap.
     */
    SharedMemRegion = 3,
    /**
     * The file descriptor for the shared memory object that contains the
     * shared pagemap page.  This is mapped read-only in the child and updated
     * in the parent.
     */
    PageMapPage,
    /**
     * The file descriptor for the socket used to pass file descriptors into the
     * child.
     */
    FDSocket,
    /**
     * The file descriptor used for the main library.  This is passed to
     * `fdlopen` in the child.
     */
    MainLibrary,
    /**
     * The file descriptor for the pipe used to send pagemap updates to the
     * parent process.
     */
    PageMapUpdates,
    /**
     * The first file descriptor number used for directory descriptors of
     * library directories.  These are used by rtld in the child to locate
     * libraries that the library identified by `MainLibrary`depends on.
     */
    OtherLibraries
  };

  void SandboxedLibrary::start_child(
    const char* library_name,
    const char* librunnerpath,
    const void* sharedmem_addr,
    int pagemap_mem,
    int pagemap_pipe,
    int fd_socket)
  {
    // The library load paths.  We're going to pass all of these to the
    // child as open directory descriptors for the run-time linker to use.
    std::array<const char*, 3> libdirs = {"/lib", "/usr/lib", "/usr/local/lib"};
    // The file descriptors for the directories in libdirs
    std::array<int, libdirs.size()> libdirfds;
    // The last file descriptor that we're going to use.  The `move_fd`
    // lambda will copy all file descriptors above this line so they can then
    // be copied into their desired location.
    static const int last_fd = OtherLibraries + libdirs.size();
    auto move_fd = [](int x) {
      assert(x >= 0);
      while (x < last_fd)
      {
        x = dup(x);
      }
      return x;
    };
    // Move all of the file descriptors that we're going to use out of the
    // region that we're going to populate.
    shm_fd = move_fd(shm_fd);
    pagemap_mem = move_fd(pagemap_mem);
    fd_socket = move_fd(fd_socket);
    pagemap_pipe = move_fd(pagemap_pipe);
    // Open the library binary.  If this fails, kill the child process.  Note
    // that we do this *before* dropping privilege - we don't have to give
    // the child the right to look in the directory that contains this
    // binary.
    int library = open(library_name, O_RDONLY);
    if (library < 0)
    {
      _exit(-1);
    }
    library = move_fd(library);
    for (size_t i = 0; i < libdirs.size(); i++)
    {
      libdirfds.at(i) = move_fd(open(libdirs.at(i), O_DIRECTORY));
    }
    // The child process expects to find these in fixed locations.
    shm_fd = dup2(shm_fd, SharedMemRegion);
    pagemap_mem = dup2(pagemap_mem, PageMapPage);
    fd_socket = dup2(fd_socket, FDSocket);
    assert(library);
    library = dup2(library, MainLibrary);
    assert(library == MainLibrary);
    pagemap_pipe = dup2(pagemap_pipe, PageMapUpdates);
    // These are passed in by environment variable, so we don't need to put
    // them in a fixed place, just after all of the others.
    int rtldfd = OtherLibraries;
    for (auto& libfd : libdirfds)
    {
      libfd = dup2(libfd, rtldfd++);
    }
#  ifdef USE_CAPSICUM
    // If we're compiling with Capsicum support, then restrict the permissions
    // on all of the file descriptors that are available to untrusted code.
    auto limit_fd = [&](int fd, auto... permissions) {
      cap_rights_t rights;
      if (cap_rights_limit(fd, cap_rights_init(&rights, permissions...)) != 0)
      {
        err(1, "Failed to limit rights on file descriptor %d", fd);
      }
    };
    // Standard in is read only
    limit_fd(STDIN_FILENO, CAP_READ);
    // Standard out and error are write only
    limit_fd(STDOUT_FILENO, CAP_WRITE);
    limit_fd(STDERR_FILENO, CAP_WRITE);
    // The pagemap socket is used only to send pagemap updates to the parent
    limit_fd(pagemap_pipe, CAP_WRITE);
    // The shared heap can be mapped read-write, but can't be truncated.
    limit_fd(shm_fd, CAP_MMAP_RW);
    limit_fd(pagemap_mem, CAP_MMAP_R);
    // The library must be parseable and mappable by rtld
    limit_fd(library, CAP_READ, CAP_FSTAT, CAP_SEEK, CAP_MMAP_RX);
    // The libraries implicitly opened from the library directories inherit
    // the permissions from the parent directory descriptors.  These need the
    // permissions required to map a library and also the permissions
    // required to search the directory to find the relevant libraries.
    for (auto libfd : libdirfds)
    {
      limit_fd(libfd, CAP_READ, CAP_FSTAT, CAP_LOOKUP, CAP_MMAP_RX);
    }
#  endif
    closefrom(last_fd);
    // Prepare the arguments to main.  These are going to be the binary name,
    // the address of the shared memory region, the length of the shared
    // memory region, and a null terminator.  We have to pass the two
    // addresses as strings because the kernel will assume that all arguments
    // to main are null-terminated strings and will copy them into the
    // process initialisation structure.
    // Note that we create these strings on the stack, rather than calling
    // asprintf, because (if we used vfork) we're still in the same address
    // space as the parent, so if we allocate memory here then it will leak in
    // the parent.
    char* args[4];
    args[0] = (char*)"library_runner";
    char address[24];
    char length[24];
    snprintf(address, sizeof(address), "%zd", (size_t)sharedmem_addr);
    args[1] = address;
    snprintf(length, sizeof(length), "%zd", shared_size);
    args[2] = length;
    args[3] = 0;
    static_assert(
      OtherLibraries == 8, "First entry in LD_LIBRARY_PATH_FDS is incorrect");
    static_assert(
      libdirfds.size() == 3,
      "Number of entries in LD_LIBRARY_PATH_FDS is incorrect");
    const char* const env[] = {"LD_LIBRARY_PATH_FDS=8:9:10", nullptr};
    execve(librunnerpath, args, const_cast<char* const*>(env));
    // Should be unreachable, but just in case we failed to exec, don't return
    // from here (returning from a vfork context is very bad!).
    _exit(EXIT_FAILURE);
  }

  SandboxedLibrary::SandboxedLibrary(const char* library_name, size_t size)
  : shared_size(1024ULL * 1024ULL * 1024ULL * size)
  {
#  ifdef __FreeBSD__
    auto mk_shm = [](const char*) {
      return shm_open(SHM_ANON, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    };
#  elif defined(__linux__)
    auto mk_shm = [](const char* debug_name) {
      int ret = memfd_create(debug_name, 0);
      // WSL doesn't support memfd, so do something ugly with temp files.
      if (ret == -1)
      {
        char name[] = "/tmp/sandbox.shmXXXXXX";
        ret = mkstemp(name);
        // unlink(name);
        assert(ret != -1);
      }
      return ret;
    };
#  else
#    error Anonymous shared memory not implemented for your platform
#  endif
    shm_fd = mk_shm("sandbox");
    // Set the size of the shared memory region.
    ftruncate(shm_fd, shared_size);
    // FIXME: Linux version will need to repeatedly try MAP_FIXED and
    // MAP_EXCL until it finds a gap of the correct size, because Linux doesn't
    // have MAP_ALIGNED
    void* ptr = mmap(
      0,
      shared_size,
      PROT_READ | PROT_WRITE,
      MAP_ALIGNED(35) | MAP_SHARED | MAP_NOCORE,
      shm_fd,
      0);
    if (ptr == MAP_FAILED)
    {
      err(1, "Map failed");
    }

    // Create a single page for the shared pagemap page.  The parent process
    // will write to this directly, the child will send messages through a pipe
    // to ask the parent to update it, but will read it directly.
    int pagemap_fd = mk_shm("pagemap");
    ftruncate(pagemap_fd, snmalloc::OS_PAGE_SIZE);

    // Allocate the shared memory region and set its memory provider to use all
    // of the space after the end of the header for subsequent allocations.
    shared_mem = new (ptr) SharedMemoryRegion();
    shared_mem->memory_provider.set_range(
      reinterpret_cast<uintptr_t>(ptr) + sizeof(SharedMemoryRegion),
      reinterpret_cast<uintptr_t>(ptr) + shared_size);

    // Create a pair of sockets that we can use to
    int pagemap_pipes[2];
    // socketpair(AF_UNIX, SOCK_STREAM, 0, pagemap_pipes);
    pipe(pagemap_pipes);
    uint8_t* shared_pagemap_page = pagemap_owner().add_range(
      reinterpret_cast<size_t>(ptr),
      reinterpret_cast<size_t>(ptr) + shared_size,
      pagemap_pipes[0],
      pagemap_fd);
    // Construct a UNIX domain socket.  This will eventually be used to send
    // file descriptors from the parent to the child, but isn't yet.
    int socks[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, socks);
    int pid;
    std::string path = ".";
    std::string lib;
    // Use dladdr to find the path of the libsandbox shared library.  For now,
    // we assume that the library runner is in the same place and so is the
    // library that we're going to open.  Eventually we should look for
    // library_runner somewhere else (e.g. ../libexec) and search
    // LD_LIBRARY_PATH for the library that we're going to open.
    Dl_info info;
    static char x;
    if (dladdr(&x, &info))
    {
      char* libpath = ::strdup(info.dli_fname);
      path = dirname(libpath);
      ::free(libpath);
    }
    if (library_name[0] == '/')
    {
      lib = library_name;
    }
    else
    {
      lib = path;
      lib += '/';
      lib += library_name;
    }
    library_name = lib.c_str();
    path += "/library_runner";
    const char* librunnerpath = path.c_str();
    // We shouldn't do anything that modifies the heap (or reads the heap in
    // a way that is not concurrency safe) between vfork and exec.
    child_proc = -1;
#  ifdef USE_KQUEUE_PROCDESC
    pid = pdfork(&child_proc, PD_DAEMON | PD_CLOEXEC);
#  else
    child_proc = pid = vfork();
    assert(child_proc != -1);
#  endif
    if (pid == 0)
    {
      // In the child process.
      start_child(
        library_name,
        librunnerpath,
        ptr,
        pagemap_fd,
        pagemap_pipes[1],
        socks[1]);
    }
    // Only reachable in the parent process
#  ifdef USE_KQUEUE_PROCDESC
    // If we're using kqueue to monitor for child failure, construct a kqueue
    // now and add this as the event that we'll monitor.  Otherwise, we'll use
    // waitpid with the pid and don't need to maintain any in-kernel state.
    kq = kqueue();
    struct kevent event;
    EV_SET(&event, child_proc, EVFILT_PROCDESC, EV_ADD, NOTE_EXIT, 0, nullptr);
    if (kevent(kq, &event, 1, nullptr, 0, nullptr) == -1)
    {
      err(1, "Setting up kqueue");
    }
#  endif
    // Close all of the file descriptors that only the child should have.
    close(socks[1]);
    close(pagemap_pipes[1]);
    close(pagemap_fd);
    socket_fd = socks[0];
    // Allocate an allocator in the shared memory region.
    allocator = new SharedAlloc(
      // FIXME: Add adaptor that does range checking on memory_provider.
      shared_mem->memory_provider,
      SharedPagemapAdaptor(shared_pagemap_page),
      &shared_mem->allocator_state);
  }
  void SandboxedLibrary::send(int idx, void* ptr)
  {
    shared_mem->function_index = idx;
    shared_mem->msg_buffer = ptr;
    shared_mem->signal(true);
    // Wait for a second, see if the child has exited, if it's still going, try
    // again.
    // FIXME: We should probably allow the user to specify a maxmimum execution
    // time for all calls and kill the sandbox and raise an exception if it's
    // taking too long.
    while (!shared_mem->wait(false, {0, 100000}))
    {
      if (has_child_exited())
      {
        throw std::runtime_error("Sandboxed library terminated abnormally");
      }
    }
  }
#  ifndef USE_KQUEUE_PROCDESC
  namespace
  {
    std::pair<pid_t, int> waitpid(pid_t child_proc, int options)
    {
      pid_t ret;
      int status;
      bool retry = false;
      do
      {
        ret = ::waitpid(child_proc, &status, options);
        retry = (ret == -1) && (errno == EINTR);
      } while (retry);
      return {ret, status};
    }
  }
#  endif
  bool SandboxedLibrary::has_child_exited()
  {
#  ifdef USE_KQUEUE_PROCDESC
    // If we're using kqueue and process descriptors then we
    struct kevent event;
    shared_mem->signal(true);
    struct timespec timeout = {0, 0};
    int ret = kevent(kq, nullptr, 0, &event, 1, &timeout);
    if (ret == -1)
    {
      err(1, "Waiting for child failed");
    }
    if (ret == 1)
    {
      child_status = event.data;
      child_exited = true;
    }
    return (ret == 1);
#  else
    auto [ret, status] = waitpid(child_proc, WEXITED | WNOHANG);
    if (ret == -1)
    {
      err(1, "Waiting for child failed");
    }
    if (ret == child_proc)
    {
      child_status = WEXITSTATUS(status);
      child_exited = true;
      return true;
    }
    return false;
#  endif
  }
  int SandboxedLibrary::wait_for_child_exit()
  {
    if (child_exited)
    {
      return child_status;
    }
    shared_mem->should_exit = true;
    shared_mem->signal(true);
#  ifdef USE_KQUEUE_PROCDESC
    struct kevent event;
    // FIXME: Timeout and increase the aggression with which we kill the child
    // process (SIGTERM, SIGKILL)
    if (kevent(kq, nullptr, 0, &event, 1, nullptr) == -1)
    {
      err(1, "Waiting for child failed");
      abort();
    }
    return event.data;
#  else
    // FIXME: Timeout and increase the aggression with which we kill the child
    // process (SIGTERM, SIGKILL)
    auto [ret, status] = waitpid(child_proc, WEXITED);
    if (ret == -1)
    {
      err(1, "Waiting for child failed");
      abort();
    }
    if (ret == child_proc && (WIFEXITED(status) || WIFSIGNALED(status)))
    {
      child_status = WEXITSTATUS(status);
      child_exited = true;
      return true;
    }
    return false;
#  endif
  }
#else
#  error Missing implementation of SandboxedLibrary
#endif

  void* SandboxedLibrary::alloc_in_sandbox(size_t bytes, size_t count)
  {
    bool overflow = false;
    size_t sz = snmalloc::bits::umul(bytes, count, overflow);
    if (overflow)
    {
      return nullptr;
    }
    return allocator->alloc(sz);
  }
  void SandboxedLibrary::dealloc_in_sandbox(void* ptr)
  {
    allocator->dealloc(ptr);
  }
}
