// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT

#include <assert.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef USE_CAPSICUM
#  include <sys/capsicum.h>
#endif
#include <aal/aal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

using address_t = snmalloc::Aal::address_t;

namespace snmalloc
{
  template<typename T>
  struct SuperslabMap;
  class Superslab;
  class Mediumslab;
}

namespace sandbox
{
  /**
   * The proxy pagemap.  In the sandboxed process, there are two parts of the
   * pagemap.  The process maintains a private pagemap for process-local
   * memory, the parent process maintains a fragment of this pagemap
   * corresponding to the shared memory region.
   *
   * This class is responsible for managing the composition of the two page
   * maps.  All queries are local but updates must be forwarded to either the
   * parent process or the private pagemap.
   */
  struct ProxyPageMap
  {
    /**
     * Singleton instance of this class.
     */
    static ProxyPageMap p;
    /**
     * Accessor, returns the singleton instance of this class.
     */
    static ProxyPageMap& pagemap()
    {
      return p;
    }
    /**
     * Helper function used by the set methods that are part of the page map
     * interface.  If the address (`p`) is not in the shared region then this
     * delegates to the default page map.  Otherwise, this writes the required
     * change as a message to the parent process and spins waiting for the
     * parent to make the required update.
     */
    void set(uintptr_t p, uint8_t x, uint8_t big);
    /**
     * Get the pagemap entry for a specific address. This queries the default
     * pagemap.
     */
    static uint8_t get(address_t p);
    /**
     * Get the pagemap entry for a specific address. This queries the default
     * pagemap.
     */
    static uint8_t get(void* p);
    /**
     * Type-safe interface for setting that a particular memory region contains
     * a superslab. This calls `set`.
     */
    void set_slab(snmalloc::Superslab* slab);
    /**
     * Type-safe interface for notifying that a region no longer contains a
     * superslab.  Calls `set`.
     */
    void clear_slab(snmalloc::Superslab* slab);
    /**
     * Type-safe interface for notifying that a region no longer contains a
     * medium slab.  Calls `set`.
     */
    void clear_slab(snmalloc::Mediumslab* slab);
    /**
     * Type-safe interface for setting that a particular memory region contains
     * a medium slab. This calls `set`.
     */
    void set_slab(snmalloc::Mediumslab* slab);
    /**
     * Type-safe interface for setting the pagemap values for a region to
     * indicate a large allocation.
     */
    void set_large_size(void* p, size_t size);
    /**
     * The inverse operation of `set_large_size`, updates a range to indicate
     * that it is not in use.
     */
    void clear_large_size(void* p, size_t size);
  };
}
sandbox::ProxyPageMap sandbox::ProxyPageMap::p;

#define SNMALLOC_DEFAULT_CHUNKMAP sandbox::ProxyPageMap
#define SNMALLOC_USE_THREAD_CLEANUP 1
#include "libsandbox.cc"
#include "override/malloc.cc"
#include "shared.h"

using namespace snmalloc;
using namespace sandbox;

namespace
{
#ifdef __linux__
  /**
   * Linux run-time linkers do not currently support fdlopen, but it can be
   * emulated with a wrapper that relies on procfs.  Each fd open in a
   * process exists as a file (typically a symlink) in /proc/{pid}/fd/{fd
   * number}, so we can open that.  This does depend on the running process
   * having access to its own procfs entries, which may be a problem for some
   * possible sandboxing approaches.
   */
  void* fdlopen(int fd, int flags)
  {
    char* str;
    asprintf(&str, "/proc/%d/fd/%d", (int)getpid(), fd);
    void* ret = dlopen(str, flags);
    free(str);
    return ret;
  }
  typedef void (*dlfunc_t)(void);

  /**
   * It is undefined behaviour in C to cast from a `void*` to a function
   * pointer, but POSIX only provides a single function to get a pointer from a
   * library.  BSD systems provide `dlfunc` to avoid this but glibc does not,
   * so we provide our own.
   */
  dlfunc_t dlfunc(void* handle, const char* symbol)
  {
    return (dlfunc_t)dlsym(handle, symbol);
  }
#endif

  /**
   * The file descriptor that will be used to communicate with the pagemap.
   * This is set to a constant after the proxy pagemap is safe to use, the
   * initial -1 value ensures that we don't try to use it before the memory
   * manager is set up.
   */
  int pagemap_socket = -1;

  /**
   * A pointer to the object that manages the vtable exported by this library.
   */
  ExportedLibrary* library;

  /**
   * The start of the shared memory region.  Passed as a command-line argument.
   */
  void* shared_memory_start = 0;

  /**
   * The end of the shared memory region.  Passed as a command-line argument.
   */
  void* shared_memory_end = 0;

  /**
   * The exported function that returns the type encoding of an exported
   * function.  This is used by the library caller for type checking.
   */
  char* exported_types(int idx)
  {
    return library->type_encoding(idx);
  }
}

namespace sandbox
{
  void ProxyPageMap::set(uintptr_t p, uint8_t x, uint8_t big = 0)
  {
    if (
      (p >= reinterpret_cast<uintptr_t>(shared_memory_start)) &&
      (p < reinterpret_cast<uintptr_t>(shared_memory_end)))
    {
      assert(pagemap_socket > 0);
      auto msg = static_cast<uint64_t>(p);
      // Make sure that the low 16 bytes are clear
      assert((msg & 0xffff) == 0);
      msg &= ~0xffff;
      msg |= x;
      msg |= big << 8;
      write(pagemap_socket, static_cast<void*>(&msg), sizeof(msg));
      while (GlobalPagemap::pagemap().get(p) != x)
      {
        // write(fileno(stderr), "Child spinning\n", 15);
        std::atomic_thread_fence(std::memory_order_seq_cst);
      }
    }
    else
    {
      GlobalPagemap::pagemap().set(p, x);
    }
  }

  uint8_t ProxyPageMap::get(address_t p)
  {
    return GlobalPagemap::pagemap().get(p);
  }

  uint8_t ProxyPageMap::get(void* p)
  {
    return GlobalPagemap::pagemap().get(address_cast(p));
  }

  void ProxyPageMap::set_slab(snmalloc::Superslab* slab)
  {
    set(reinterpret_cast<uintptr_t>(slab), (size_t)CMSuperslab);
  }

  void ProxyPageMap::clear_slab(snmalloc::Superslab* slab)
  {
    set(reinterpret_cast<uintptr_t>(slab), (size_t)CMNotOurs);
  }

  void ProxyPageMap::clear_slab(snmalloc::Mediumslab* slab)
  {
    set(reinterpret_cast<uintptr_t>(slab), (size_t)CMNotOurs);
  }

  void ProxyPageMap::set_slab(snmalloc::Mediumslab* slab)
  {
    set(reinterpret_cast<uintptr_t>(slab), (size_t)CMMediumslab);
  }

  void ProxyPageMap::set_large_size(void* p, size_t size)
  {
    size_t size_bits = bits::next_pow2_bits(size);
    if ((p >= shared_memory_start) && (p < shared_memory_end))
    {
      set(reinterpret_cast<uintptr_t>(p), (uint8_t)size_bits, 1);
      return;
    }
    set(reinterpret_cast<uintptr_t>(p), size_bits);
    // Set redirect slide
    uintptr_t ss = (uintptr_t)((size_t)p + SUPERSLAB_SIZE);
    for (size_t i = 0; i < size_bits - SUPERSLAB_BITS; i++)
    {
      size_t run = 1ULL << i;
      GlobalPagemap::pagemap().set_range(
        ss, (uint8_t)(64 + i + SUPERSLAB_BITS), run);
      ss = ss + SUPERSLAB_SIZE * run;
    }
  }

  void ProxyPageMap::clear_large_size(void* p, size_t size)
  {
    if ((p >= shared_memory_start) && (p < shared_memory_end))
    {
      size_t size_bits = bits::next_pow2_bits(size);
      set(reinterpret_cast<uintptr_t>(p), (uint8_t)size_bits, 2);
      return;
    }
    auto range = (size + SUPERSLAB_SIZE - 1) >> SUPERSLAB_BITS;
    GlobalPagemap::pagemap().set_range(
      reinterpret_cast<uintptr_t>(p), CMNotOurs, range);
  }

  /**
   * The class that represents the internal side of an exported library.  This
   * manages a run loop that waits for calls, invokes them, and returns the
   * result.
   */
  class ExportedLibraryPrivate
  {
    friend class ExportedLibrary;
#ifdef __unix__
    /**
     * The type used for handles to operating system resources.  On POSIX
     * systems, file descriptors are `int`s (and everything is a file).
     */
    using handle_t = int;
#endif

    /**
     * The socket that should be used for passing new file descriptors into
     * this process.
     *
     * Not implemented yet.
     */
    __attribute__((unused)) handle_t socket_fd;

    /**
     * The shared memory region owned by this sandboxed library.
     */
    struct SharedMemoryRegion* shared_mem;

  public:
    /**
     * Constructor.  Takes the socket over which this process should receive
     * additional file descriptors and the shared memory region.
     */
    ExportedLibraryPrivate(handle_t sock, SharedMemoryRegion* region)
    : socket_fd(sock), shared_mem(region)
    {}

    /**
     * The run loop.  Takes the public interface of this library (effectively,
     * the library's vtable) as an argument.
     */
    void runloop(ExportedLibrary* library)
    {
      while (1)
      {
        shared_mem->wait(true);
        if (shared_mem->should_exit)
        {
          exit(0);
        }
        assert(shared_mem->is_child_executing);
        try
        {
          (*library->functions[shared_mem->function_index])(
            shared_mem->msg_buffer);
        }
        catch (...)
        {
          // FIXME: Report error in some useful way.
          printf("Exception!\n");
        }
        shared_mem->signal(false);
      }
    }
  };
}

char* ExportedLibrary::type_encoding(int idx)
{
  return functions.at(idx)->type_encoding();
}

int main(int, char** argv)
{
#ifdef USE_CAPSICUM
  cap_enter();
#endif
  void* addr = (void*)strtoull(argv[1], nullptr, 0);
  size_t length = strtoull(argv[2], nullptr, 0);
  // fprintf(stderr, "Child starting\n");
  // printf(
  //"Child trying to map fd %d at addr %p (0x%zx)\n", SharedMemRegion, addr,
  // length);
  void* ptr = mmap(
    addr,
    length,
    PROT_READ | PROT_WRITE,
    MAP_FIXED | MAP_ALIGNED(35) | MAP_SHARED | MAP_NOCORE,
    SharedMemRegion,
    0);
  // printf("%p\n", ptr);
  if (ptr == MAP_FAILED)
  {
    err(1, "Mapping shared heap failed");
  }
  // Close the shared memory region file descriptor before we call untrusted
  // code.
  close(SharedMemRegion);

  auto shared = reinterpret_cast<SharedMemoryRegion*>(ptr);
  // Splice the pagemap page inherited from the parent into the pagemap.
  void* pagemap_chunk =
    GlobalPagemap::pagemap().page_for_address(reinterpret_cast<uintptr_t>(ptr));
  munmap(pagemap_chunk, 4096);
  void* shared_pagemap = mmap(
    pagemap_chunk, 4096, PROT_READ, MAP_SHARED | MAP_FIXED, PageMapPage, 0);
  if (shared_pagemap == MAP_FAILED)
  {
    err(1, "Mapping shared pagemap page failed");
  }
  SharedMemoryProvider* global_virtual = &shared->memory_provider;
  shared_memory_start =
    reinterpret_cast<void*>(global_virtual->shared_heap_range_start.load());
  shared_memory_end =
    reinterpret_cast<void*>(global_virtual->shared_heap_range_end);
  assert(shared_pagemap == pagemap_chunk);
  (void)shared_pagemap;
  close(PageMapPage);
  pagemap_socket = PageMapUpdates;

  // Replace the current thread allocator with a new one in the shared region.
  // After this point, all new memory allocations are shared with the parent.
  current_alloc_pool() =
    snmalloc::make_alloc_pool<GlobalVirtual, Alloc>(*global_virtual);
  ThreadAlloc::get_reference() = current_alloc_pool()->acquire();

  void* handle = fdlopen(MainLibrary, RTLD_GLOBAL);
  if (handle == nullptr)
  {
    fprintf(stderr, "dlopen failed: %s\n", dlerror());
    return 1;
  }
  void (*sandbox_init)(ExportedLibrary*) =
    reinterpret_cast<void (*)(ExportedLibrary*)>(
      dlfunc(handle, "sandbox_init"));
  if (sandbox_init == nullptr)
  {
    fprintf(stderr, "dlfunc failed: %s\n", dlerror());
    return 1;
  }
  ExportedLibraryPrivate* libPrivate;
  libPrivate = new ExportedLibraryPrivate(FDSocket, shared);
  library = new ExportedLibrary();
  library->export_function(exported_types);
  sandbox_init(library);

  libPrivate->runloop(library);

  return 0;
}
