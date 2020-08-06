// this handels pagemap operations for interactions with wasm app 
// look at verona's process-based sandbox code/comments for more details

#include <array>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <aal/aal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "wasm_pagemap.hh"


using address_t = snmalloc::Aal::address_t;
int pagemap_socket = -1;
void* shared_memory_start = 0;
void* shared_memory_end = 0;


namespace snmalloc
{
  template<typename T>
  struct SuperslabMap;
  class Superslab;
  class Mediumslab;
}

namespace wasm_sandbox
{
    //look at snmalloc/src/mem/chunkmap.h for details
  struct ProxyPageMap 
  {

    static ProxyPageMap p;

    static ProxyPageMap& pagemap()
    {
      return p;
    }

    void set(uintptr_t p, uint8_t x, uint8_t big);

    static uint8_t get(address_t p);

    static uint8_t get(void* p);

    void set_slab(snmalloc::Superslab* slab);

    void clear_slab(snmalloc::Superslab* slab);

    void clear_slab(snmalloc::Mediumslab* slab);

    void set_slab(snmalloc::Mediumslab* slab);

    void set_large_size(void* p, size_t size);

    void clear_large_size(void* p, size_t size);

  };
}

wasm_sandbox::ProxyPageMap wasm_sandbox::ProxyPageMap::p;
#define SNMALLOC_DEFAULT_CHUNKMAP wasm_sandbox::ProxyPageMap
//#define SNMALLOC_USE_THREAD_CLEANUP 1
#include "override/malloc.cc"
using namespace snmalloc;
using namespace wasm_sandbox;


namespace wasm_sandbox
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


}


