// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT

#include <assert.h>
#include <memory>
#include <string.h>
#include <tuple>
#include <vector>
#ifdef __unix__
#  include <fcntl.h>
#  include <pthread.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

#include <ds/helpers.h>

#ifndef SANDBOX_PAGEMAP
#  ifdef SNMALLOC_DEFAULT_PAGEMAP
#    define SANDBOX_PAGEMAP SNMALLOC_DEFAULT_PAGEMAP
#  else
#    define SANDBOX_PAGEMAP snmalloc::SuperslabMap
#  endif
#endif

namespace snmalloc
{
  template<class T>
  class MemoryProviderStateMixin;
  template<class T>
  class PALFixedRegion;
  template<
    bool (*NeedsInitialisation)(void*),
    void* (*InitThreadAllocator)(function_ref<void*(void*)>),
    class MemoryProvider,
    class ChunkMap,
    bool IsQueueInline>
  class Allocator;
  template<typename T>
  struct SuperslabMap;
  void* no_replacement(void*);
}

namespace wasm_sandbox
{
  struct SharedMemoryRegion;
  struct SharedPagemapAdaptor;
  struct MemoryProviderBumpPointerState;
  /**
   * this class is for interactions with wasm sandbox
   */
  class WasmSandbox
  {
    using handle_t = int;

    /**
     * The type of the memory provider that is used to manage the shared
     * memory.  This is a bump-the-pointer allocator that allocates from a
     * shared-memory region.
     */
    using SharedVirtual = snmalloc::MemoryProviderStateMixin<
      snmalloc::PALFixedRegion<MemoryProviderBumpPointerState>>;
    SNMALLOC_FAST_PATH static bool needs_initialisation(void*)
    {
      return false;
    }
    SNMALLOC_FAST_PATH static void*
      init_thread_allocator(snmalloc::function_ref<void*(void*)>)
    {
      return nullptr;
    }
    /**
     * The type of the allocator that allocates within the shared region from
     * outside.  This has an out-of-line message queue, allocated in the shared
     * memory region, and updates both views of the pagemap when allocating new slabs.
     */
    using SharedAlloc = snmalloc::Allocator<
      needs_initialisation,
      init_thread_allocator,
      SharedVirtual,
      SharedPagemapAdaptor,
      false>;
    /**
     * The allocator used for allocating memory inside this wasm sandbox.
     */
    SharedAlloc* allocator;
    /**
     * The handle to the shared memory region that backs the wasm app heap.
     */
    handle_t shm_fd;
    /**
     * The handle to the socket that is used to pass file descriptors to the wasm sandbox
     */
    handle_t socket_fd;
  
    /**
     * A pointer to the shared-memory region.  The start of this is structured,
     * the rest is an untyped region of memory that can be used to allocate
     * slabs and large objects.
     */
    struct SharedMemoryRegion* shared_mem;
 
    size_t shared_size;
    /**
     * Allocate some memory in the sandbox.  Returns `nullptr` if the
     * allocation failed.
     */
    void* alloc_in_sandbox(size_t bytes, size_t count);
    /**
     * Deallocate an allocation in the sandbox.
     */
    void dealloc_in_sandbox(void* ptr);


  public:

    /**
     * Destructor.  Cleans up the shared memory region.
     *
     * Note that all pointers into memory owned by the library are invalid
     * after this has been deallocated.
     */
    ~WasmSandbox();
    /**
     * Constructor.  Creates a new sandboxed instance of the library named by
     * `library_name`, with the heap size specified in GiBs.
     */
    WasmSandbox(size_t heap_size_in_GiBs = 1);
    /**
     * Allocate space for an array of `count` instances of `T`.  Objects in the
     * array will be default constructed.
     *
     * Only POD types may be allocated in the sandbox - anything with a vtable
     * would have its vtable incorrectly initialised.
     */
    template<typename T>
    T* alloc(size_t count)
    {
      static_assert(
        std::is_standard_layout_v<T> && std::is_trivial_v<T>,
        "Arrays allocated in sandboxes must be POD types");
      T* array = static_cast<T*>(alloc_in_sandbox(sizeof(T), count));
      for (size_t i = 0; i < count; i++)
      {
        new (&array[i]) T();
      }
      return array;
    }
    /**
     * Allocate space for a fixed-sized array of `Count` instances of `T`.
     * Objects in the array will be default constructed.
     *
     * Only POD types may be allocated in the sandbox - anything with a vtable
     * would have its vtable incorrectly initialised.
     */
    template<typename T, size_t Count>
    T* alloc()
    {
      static_assert(
        std::is_standard_layout_v<T> && std::is_trivial_v<T>,
        "Arrays allocated in sandboxes must be POD types");
      T* array = static_cast<T*>(alloc_in_sandbox(sizeof(T), Count));
      for (size_t i = 0; i < Count; i++)
      {
        new (&array[i]) T();
      }
      return array;
    }
    /**
     * Allocate an object in the sandbox and call its constructor with the
     * specified arguments.
     *
     * Only types without vtables may be allocated in the sandbox - anything
     * with a vtable would have its vtable incorrectly initialised.
     */
    template<typename T, typename... Args>
    T* alloc(Args&&... args)
    {
      static_assert(
        !std::is_polymorphic_v<T>,
        "Classes with vtables cannot be safely allocated in sandboxes from "
        "outside (nor can virtual functions be safely called).");
      return new (alloc_in_sandbox(sizeof(T), 1))
        T(std::forward<Args>(args)...);
    }
    /**
     * Free an object allocated in the sandbox.
     */
    template<typename T>
    void free(T* obj)
    {
      dealloc_in_sandbox(static_cast<void*>(obj));
    }
    /**
     * Helper function to copy a string into a sandbox.  The caller is
     * responsible for freeing the returned memory by calling the `free` method
     * on this class.
     */
    char* strdup(const char* str)
    {
      auto len = strlen(str);
      char* ptr = alloc<char>(len);
      memcpy(ptr, str, len);
      return ptr;
    }

  private:
    /**
     * SandboxedFunction is allowed to call the following methods in this class.
     */
    template<typename Ret, typename... Args>
    friend class SandboxedFunction;
    /**
     * Sends a message to the child process, containing a vtable index and a
     * pointer to the argument frame (a tuple of arguments and space for the
     * return value).
     */
    void send(int idx, void* ptr);
    /**
     * Instruct the child to exit and block until it does.  The return value is
     * the exit code of the child process.  If the child has already exited,
     * then this return immediately.
     */
    int wait_for_child_exit();
    /**
     * Pool to determine if the child has exited.  This interface is inherently
     * racy: If it returns `false` there is no guarantee that the child hasn't
     * exited immediately after the call.
     */
    bool has_child_exited();
  };



}