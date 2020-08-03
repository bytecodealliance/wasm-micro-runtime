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
  class PALPlainMixin;
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

namespace sandbox
{
  struct SharedMemoryRegion;
  struct SharedPagemapAdaptor;
  struct MemoryProviderBumpPointerState;
  /**
   * Class encapsulating an instance of a shared library in a sandbox.
   * Instances of this class will create a sandbox and load a specified library
   * into it, but are useless in isolation.  The `SandboxedFunction` class
   * provides a wrapper for calling an exported function in the specified
   * library.
   */
  class SandboxedLibrary
  {
#ifdef __unix__
    /**
     * `handle_t` is the type used by the OS for handles to operating system
     * resources.  On *NIX systems, file descriptors are represented as
     * `int`s.
     */
    using handle_t = int;
#  ifdef __FreeBSD__
    /**
     * A handle to a process.  On FreeBSD, we spawn child processes using
     * `pdfork` and so we have a process descriptor as the handle to a child
     * process.
     */
    using process_handle_t = handle_t;
#  else
    /**
     * A handle to a process.  On most *NIX systems, we use vfork to create the
     * child and then use a pid (an index in a global namespace, not a local
     * handle) to refer to it.
     */
    using process_handle_t = pid_t;
#  endif
#endif
    /**
     * The type of the memory provider that is used to manage the shared
     * memory.  This is a bump-the-pointer allocator that allocates from a
     * shared-memory region.
     */
    using SharedVirtual = snmalloc::MemoryProviderStateMixin<
      snmalloc::PALPlainMixin<MemoryProviderBumpPointerState>>;
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
     * memory region, and updates both the child and parent views of the
     * pagemap when allocating new slabs.
     */
    using SharedAlloc = snmalloc::Allocator<
      needs_initialisation,
      init_thread_allocator,
      SharedVirtual,
      SharedPagemapAdaptor,
      false>;
    /**
     * The allocator used for allocating memory inside this sandbox.
     */
    SharedAlloc* allocator;
    /**
     * The handle to the shared memory region that backs the sandboxed
     * process's heap.
     */
    handle_t shm_fd;
    /**
     * The handle to the socket that is used to pass file descriptors to the
     * sandboxed process.
     */
    handle_t socket_fd;
    /**
     * The process ID or handle of the child process.  This is either a process
     * descriptor (handle / capability) or a process ID in a global namespace.
     */
    process_handle_t child_proc;
#ifdef USE_KQUEUE_PROCDESC
    /**
     * The queue used to watch for child exit.
     */
    int kq;
#endif
    /**
     * A pointer to the shared-memory region.  The start of this is structured,
     * the rest is an untyped region of memory that can be used to allocate
     * slabs and large objects.
     */
    struct SharedMemoryRegion* shared_mem;
    /**
     * The first unused vtable entry.  When a sandboxed library is created, all
     * of the functions register themselves at a specific index.
     *
     * The first vtable entry is reserved for the function that returns the
     * type encoding of a specific vtable entry.  This is used to ensure that
     * the child and parent agree on the type signatures of all exported
     * functions.
     */
    int last_vtable_entry = 1;
    /**
     * A flag indicating whether the child has exited.  This is updated when a
     * message send fails.
     */
    bool child_exited = false;
    /**
     * The exit code of the child.  This is set when the child process exits.
     */
    int child_status;
    /**
     * The size of the shared-memory region.
     */
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
    /**
     * Start the child process.  On *NIX systems, this can be called within a
     * vfork context and so must not allocate or modify memory on the heap, or
     * read from the heap in a way that is not safe in the presence of
     * concurrency.
     *
     * The `library_name` parameter is the path to the library that should be
     * launched.
     *
     * The `librunnerpath` parameter is the full path to the `library_runner`
     * binary that runs as the child process, loads the library, and so on.
     *
     * The `sharedmem_addr` is the address at which the shared memory region
     * should be mapped in the child.
     *
     * The `pagemap_mem` parameter is the file descriptor for the pagemap
     * shared page.
     *
     * The `pagemap_pipe` parameter is the file descriptor for the pipe used to
     * send pagemap updates from the child to the parent.
     *
     * The `fd_socket` parameter is the file descriptor for a socket that can
     * be used to send file descriptors to the child process.
     */
    [[noreturn]] void start_child(
      const char* library_name,
      const char* librunnerpath,
      const void* sharedmem_addr,
      int pagemap_mem,
      int pagemap_pipe,
      int fd_socket);

  public:
    /**
     * Returns the next vtable entry to use, incrementing the counter so
     * subsequent calls will always return a fresh value.
     */
    int next_vtable_entry()
    {
      return last_vtable_entry++;
    }
    /**
     * Destructor.  Cleans up the shared memory region.
     *
     * Note that all pointers into memory owned by the library are invalid
     * after this has been deallocated.
     */
    ~SandboxedLibrary();
    /**
     * Constructor.  Creates a new sandboxed instance of the library named by
     * `library_name`, with the heap size specified in GiBs.
     */
    SandboxedLibrary(const char* library_name, size_t heap_size_in_GiBs = 1);
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

  /**
   * The argument frame for a sandboxed function.  This contains space for the
   * return value and the arguments.
   */
  template<typename Ret, typename... Args>
  struct argframe
  {
    /**
     * The return value of this function.  If the return value is void, uses
     * char as an unused placeholder.
     */
    std::conditional_t<std::is_void_v<Ret>, char, Ret> ret;
    /**
     * The arguments to the function.
     */
    std::tuple<std::remove_reference_t<Args>...> args;
    // FIXME: This strips references, so we copy arguments, but should probably
    // turn references into pointers.
  };

  /**
   * A wrapper for invoking a function exported from a sandbox.
   */
  template<typename Ret, typename... Args>
  class SandboxedFunction
  {
    /**
     * When constructed with the public constructor, this function uses the
     * private constructor to create a sandboxed function that calls into the
     * child to confirm type signature agreement.  All instances of this
     * template must therefore be allowed to call the private constructor, so
     * are all friends.
     */
    template<typename R, typename... A>
    friend class SandboxedFunction;
    /**
     * The library that exports the function around which this is a wrapper.
     */
    SandboxedLibrary& lib;
    /**
     * The correct argument frame type for this specific instantiation.
     */
    using argframe = argframe<Ret, Args...>;
    /**
     * The index of this function in the library's vtable.
     */
    int vtable_index;
    /**
     * Designated constructor.  The public constructor both delegates to this
     * and calls it when construction a new temporary sandboxed function to
     * perform type checking.
     */
    SandboxedFunction(SandboxedLibrary& l, int idx) : lib(l), vtable_index(idx)
    {}

  public:
    /**
     * Public constructor.  Called with a sandboxed library as an argument and
     * registers itself as the function in the next vtable slot.  Instances of
     * this class should be created as fields of a structure representing an
     * interface to a sandboxed library, where each exported function field is
     * declared in the same order that it is exported from the library.
     */
    SandboxedFunction(SandboxedLibrary& l)
    : SandboxedFunction(l, l.next_vtable_entry())
    {
#ifndef NDEBUG
      SandboxedFunction<char*, int> get_type(l, 0);
      char* exported = get_type(vtable_index);
      Ret (*fn)(Args...);
      if ((exported == nullptr) || (strcmp(exported, typeid(fn).name()) != 0))
      {
        exit(EXIT_FAILURE);
      }
      free(exported);
#endif
    }
    /**
     * call operator.  Passes the arguments into the sandbox, signals it to
     * invoke the method, and waits for the return value to be propagated.
     */
    Ret operator()(Args... args)
    {
      argframe* callframe = lib.alloc<argframe>();
      callframe->args = std::forward_as_tuple(args...);
      lib.send(vtable_index, callframe);
      if constexpr (!std::is_void_v<Ret>)
      {
        Ret r = callframe->ret;
        lib.free(callframe);
        return r;
      }
      else
      {
        lib.free(callframe);
      }
    }
  };

  /**
   * Interface for a function exported from a sandbox.  This is used to provide
   * a single set of virtual functions that can invoke all of the specialised
   * versions defined in the templated subclass.
   */
  struct SandboxExportedFunctionBase
  {
    /**
     * Call the function.  The `callframe` parameter is a pointer to an
     * `argframe<Ret, Args...>` corresponding to the return and argument types
     * of the function.
     */
    virtual void operator()(void* callframe) = 0;
    /**
     * Virtual destructor.  This class and its subclasses have trivial
     * destructors, so this is completely unnecessary, but this silences some
     * compiler warnings.
     */
    virtual ~SandboxExportedFunctionBase() {}
    /**
     * Return the type encoding string of the function.  This is used to detect
     * type mismatches between exported and imported functions.
     */
    virtual char* type_encoding() = 0;
  };

  /**
   * Concrete implementation of an exported function.  This template generates
   * the trampoline required to call a function with the specified signature.
   */
  template<typename Ret, typename... Args>
  class SandboxExportedFunction : public SandboxExportedFunctionBase
  {
    /**
     * The type of the structure containing the arguments and the return
     * value for this function.
     */
    using argframe = argframe<Ret, Args...>;
    /**
     * A pointer to the function that will be called by this class.
     */
    Ret (*function)(Args...);
    /**
     * The type encoding string for this function.
     */
    char* type_encoding() override
    {
      return strdup(typeid(function).name());
    }
    /**
     * Call the function and, if it returns a non-void type, store the result
     * into the frame.
     */
    void operator()(void* callframe) override
    {
      assert(callframe != nullptr);
      auto frame = (static_cast<argframe*>(callframe));
      if constexpr (!std::is_void_v<Ret>)
      {
        frame->ret = std::apply(function, frame->args);
      }
      else
      {
        std::apply(function, frame->args);
      }
    }

  public:
    /**
     * Construct a new sandbox-exported function.  This should be called only by
     * the `export_function` in `ExportedLibrary`, but that calls it indirectly
     * and so we can't sensibly do this via a friend definition.
     */
    SandboxExportedFunction(Ret (*fn)(Args...)) : function(fn) {}
  };

  /**
   * Class wrapping a set of functions that are exported from a library.  This
   * class should never be instantiated directly.  A library that is intended
   * to be used in a sandboxed context should implement a function with the
   * following name and signature:
   *
   * ```
   * extern "C" void sandbox_init(sandbox::ExportedLibrary* library);
   * ```
   *
   * The sandbox runner process will call this function, providing it with a
   * pointer to an instance of this class, during setup.
   */
  class ExportedLibrary
  {
    /**
     * Class that implements the runloop behaviour for this class and manages
     * the shared memory.
     */
    friend class ExportedLibraryPrivate;
    /**
     * The vtable for this library: map from integers to functions that can be
     * invoked.
     */
    std::vector<std::unique_ptr<SandboxExportedFunctionBase>> functions;

  public:
    /**
     * Returns the type encoding string (in a C++ ABI-specific format) of the
     * function at the specified vtable index.
     */
    char* type_encoding(int idx);
    /**
     * Export a function to consumers of this sandboxed library.  The function
     * is inserted in the next vtable entry so the order of calls to this
     * method from inside the sandbox must match the order of
     * `SandboxedFunction` objects created for a `SandboxedLibrary` that
     * corresponds to the outside of this sandbox.
     */
    template<typename Ret, typename... Args>
    void export_function(Ret (*fn)(Args...))
    {
      functions.emplace_back(new SandboxExportedFunction(fn));
    }
  };

  /**
   * Helpers that assist with type deduction for `SandboxedFunction` objects.
   *
   * Nothing in this namespace should be used outside of this library.
   */
  namespace internal
  {
    /**
     * Template that deduces the return type and argument types for a function
     * `signature<void(int, float)>::return_type` is `void` and
     * `signature<void(int, float)>::argument_type` is `std::tuple<int, float>`.
     */
    template<typename T>
    struct signature;
    /**
     * Specialisation for when the callee is a value.
     */
    template<typename R, typename... Args>
    struct signature<R(Args...)>
    {
      /**
       * The return type of the function whose type is being extracted.
       */
      using return_type = R;
      /**
       * A tuple type containing all of the argument types of the function
       * whose type is being extracted.
       */
      using argument_type = std::tuple<Args...>;
    };
    /**
     * Specification for when the callee is a reference.
     */
    template<typename R, typename... Args>
    struct signature<R (&)(Args...)>
    {
      /**
       * The return type of the function whose type is being extracted.
       */
      using return_type = R;
      /**
       * A tuple type containing all of the argument types of the function
       * whose type is being extracted.
       */
      using argument_type = std::tuple<Args...>;
    };

    /**
     * Given the types deduced by `signature`, construct the type of a
     * `SandboxedFunction` that corresponds to the type of the original
     * function.
     */
    template<typename Ret, typename T>
    struct extract_args;
    /**
     * Explicit specialisation.  This is the only version that actually exists,
     * but C++17 doesn't let us declare the generic version with these
     * constraints.
     */
    template<typename Ret, typename... T>
    struct extract_args<Ret, std::tuple<T...>>
    {
      /**
       * The wrapper function type for the exported function type provided by
       * the template arguments.
       */
      using wrapper = SandboxedFunction<Ret, T...>;
    };
  }

  /**
   * Construct a sandboxed function proxy that corresponds to the type
   * signature of the function given in the template parameter.
   *
   * Note that this intentionally does not take the function as an argument.
   * The code outside the sandbox does not need to be linked to the library
   * that implements the function (and, ideally, won't be!).  As such, this
   * function can't ever depend on a concrete definition of the function
   * existing, but can depend on its type signature.  This is intended to be
   * used with `decltype(someFunction)` as an explicit template argument.
   */
  template<typename Fn>
  inline auto make_sandboxed_function(SandboxedLibrary& l)
  {
    using sig = internal::signature<Fn>;
    return typename internal::extract_args<
      typename sig::return_type,
      typename sig::argument_type>::wrapper{l};
  }
}
