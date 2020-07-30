/*
 * Copyright (C) 2020 Microsoft.  All rights reserved.
 */

#include "mem_alloc.h"

#define SNMALLOC_MEMORY_PROVIDER PALFixedRegion
#define USE_RESERVE_MULTIPLE 1
#define NO_BOOTSTRAP_ALLOCATOR
#define IS_ADDRESS_SPACE_CONSTRAINED
#define SNMALLOC_EXPOSE_PAGEMAP
#define SNMALLOC_NAME_MANGLE(a) wamr_sn_##a
// Redefine the namespace, so we can a different allocator from the system one.
#define snmalloc snmalloc_wamr

#include "../../../snmalloc/src/ds/address.h"
#include "../../../snmalloc/src/ds/flaglock.h"
#include <array>
#include <stdio.h>
#include <stdlib.h>

namespace snmalloc
{
  class PALFixedRegion
  {
    /// Base of OE heap
    static inline void* heap_base = nullptr;

    /// Size of OE heap
    static inline size_t heap_size;

    // This is infrequently used code, a spin lock simplifies the code
    // considerably, and should never be on the fast path.
    static inline std::atomic_flag spin_lock;

  public:
    /**
     * This will be called by oe_allocator_init to set up enclave heap bounds.
     */
    static void setup_initial_range(void* base, void* end)
    {
      heap_size = pointer_diff(base, end);
      heap_base = base;
    }

    /**
     * Bitmap of PalFeatures flags indicating the optional features that this
     * PAL supports.
     */
    static constexpr uint64_t pal_features = 0;

    static constexpr size_t page_size = 0x1000;

    [[noreturn]] static void error(const char* const str)
    {
      fprintf(stderr, "%s\n", str);
      abort();
    }

    static std::pair<void*, size_t>
    reserve_at_least(size_t request_size) noexcept
    {
      // First call returns the entire address space
      // subsequent calls return {nullptr, 0}
      FlagLock lock(spin_lock);
      if (request_size > heap_size)
        return {nullptr, 0};

      auto result = std::make_pair(heap_base, heap_size);
      heap_size = 0;
      return result;
    }

    template<bool page_aligned = false>
    void zero(void* p, size_t size) noexcept
    {
      memset(p, size, 0);
    }
  };
}

#include "../../../snmalloc/src/override/malloc.cc"

extern "C" {

mem_allocator_t mem_allocator_create(void *mem, uint32_t size)
{
    static bool called = false;
    if (called)
    {
        fprintf(stderr, "snmalloc creation called a second time with argument %p %d", mem, size);
        abort();
    }
    snmalloc_wamr::PALFixedRegion::setup_initial_range(mem, (char*)mem + size);
    called = true;
    return (mem_allocator_t)new snmalloc_wamr::Alloc(snmalloc_wamr::default_memory_provider());
}

void mem_allocator_destroy(mem_allocator_t allocator)
{
    delete (snmalloc_wamr::Alloc*)allocator;
    return;
}

void *
mem_allocator_malloc(mem_allocator_t allocator, uint32_t size)
{
    return ((snmalloc_wamr::Alloc*)allocator)->alloc(size);
}

void *
mem_allocator_realloc(mem_allocator_t allocator, void *ptr, uint32_t size)
{
    if (size == (uint32_t)-1)
    {
      errno = ENOMEM;
      return nullptr;
    }
    if (ptr == nullptr)
    {
      return mem_allocator_malloc(allocator, size);
    }
    if (size == 0)
    {
      mem_allocator_free(allocator, ptr);
      return nullptr;
    }
    uint32_t sz = (uint32_t)((snmalloc_wamr::Alloc*)allocator)->alloc_size(ptr);
    // Keep the current allocation if the given size is in the same sizeclass.
    if (sz == round_size(size))
    {
      return ptr;
    }

    void* p = mem_allocator_malloc(allocator, size);
    if (p != nullptr)
    {
      sz = bits::min(size, sz);
      memcpy(p, ptr, sz);
      mem_allocator_free(allocator, ptr);
    }

    return p;
}

void mem_allocator_free(mem_allocator_t allocator, void *ptr)
{
    if (ptr)
    {
        ((snmalloc_wamr::Alloc*)allocator)->dealloc(ptr);
    }
}

int
mem_allocator_migrate(mem_allocator_t allocator,
                      mem_allocator_t allocator_old)
{
    UNUSED(allocator);
    UNUSED(allocator_old);
    fprintf(stderr, "snmalloc migration not supported");
    abort();
}

int
mem_allocator_reinit_lock(mem_allocator_t allocator)
{
    UNUSED(allocator);
    return 0;
}

void
mem_allocator_destroy_lock(mem_allocator_t allocator)
{
    UNUSED(allocator);
    return;
}

}
