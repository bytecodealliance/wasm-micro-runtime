// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT

#include "sandbox.hh"
#include "shared.h"

#include <stdio.h>

using namespace sandbox;

/**
 * The structure that represents an instance of the demo sandbox.
 */
struct SandboxDemo
{
  /**
   * The library that defines the functions exposed by this sandbox.
   */
  SandboxedLibrary lib = {"example_lib.so"};
#define EXPORTED_FUNCTION(public_name, private_name) \
  decltype(make_sandboxed_function<decltype(private_name)>(lib)) public_name = \
    make_sandboxed_function<decltype(private_name)>(lib);
#include "functions.inc"
};

int main(int argc, char** argv)
{
  SandboxDemo sandbox;
  printf("With sandbox instance 1: 1 + 2 = %d\n", sandbox.sum(1, 2));
  SandboxDemo sb2;
  printf("With sandbox instance 2: 1 + 2 = %d\n", sb2.sum(1, 2));
  SandboxDemo sb3;
  printf("With sandbox instance 3: 1 + 2 = %d\n", sb3.sum(1, 2));
  try
  {
    if (argc > 1)
    {
      int fd = open(argv[1], O_RDONLY);
      if (fd >= 0)
      {
        char* outfn;
        asprintf(&outfn, "%s.Z", argv[1]);
        int outfd = open(outfn, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        static const size_t out_buffer_size = 1024;
        static const size_t in_buffer_size = 1024;
        char* in = sandbox.lib.alloc<char>(in_buffer_size);
        char* out = sandbox.lib.alloc<char>(out_buffer_size);
        // This is needed because deflateInit is a macro that implicitly passes
        // a string literal.
        char* version = sandbox.lib.strdup(ZLIB_VERSION);
#undef ZLIB_VERSION
#define ZLIB_VERSION version
        z_stream* zs = sandbox.lib.alloc<z_stream>();
        memset(zs, 0, sizeof(*zs));
        zs->zalloc = Z_NULL;
        zs->zfree = Z_NULL;
        int ret = sandbox.deflateInit(zs, Z_DEFAULT_COMPRESSION);
        if (ret != Z_OK)
        {
          printf("deflateInit failed %d\n", ret);
        }
        zs->next_out = reinterpret_cast<Bytef*>(out);
        zs->avail_out = out_buffer_size;

        while ((zs->avail_in = read(fd, in, in_buffer_size)))
        {
          zs->next_in = reinterpret_cast<Bytef*>(in);
          if (sandbox.deflate(zs, Z_PARTIAL_FLUSH) == Z_STREAM_ERROR)
          {
            printf("Deflate failed: %s\n", zs->msg);
            return 1;
          }
          size_t avail_out = std::min<size_t>(zs->avail_out, out_buffer_size);
          if (avail_out < out_buffer_size)
          {
            if (
              write(outfd, out, out_buffer_size - avail_out) <
              static_cast<ssize_t>(out_buffer_size - avail_out))
            {
              printf("Write failed\n");
            }
            zs->next_out = reinterpret_cast<Bytef*>(out);
            zs->avail_out = out_buffer_size;
          }
        }
        sandbox.deflate(zs, Z_FINISH);
        sandbox.deflateEnd(zs);
        size_t avail_out = std::min<size_t>(zs->avail_out, out_buffer_size);
        if (avail_out < out_buffer_size)
        {
          write(outfd, zs->next_out, out_buffer_size - avail_out);
        }
        free(zs);
        free(in);
        free(out);
        close(outfd);
        close(fd);
      }
    }
  }
  catch (std::runtime_error e)
  {
    printf("Sandbox exception: %s while running zlib compress\n", e.what());
    return -1;
  }
  try
  {
    printf(
      "Calling a function that should cause the sandboxed process to "
      "crash...\n");
    sandbox.crash();
  }
  catch (std::runtime_error e)
  {
    printf("Sandbox exception: %s\n", e.what());
    printf("Parent process continuing happily...\n");
  }

  return 0;
}
