// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT

#include "shared.h"

#include <algorithm>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv)
{
  if (argc > 1)
  {
    int fd = open(argv[1], O_RDONLY);
    if (fd > 0)
    {
      char* outfn;
      asprintf(&outfn, "%s.Z", argv[1]);
      int outfd = open(outfn, O_WRONLY | O_CREAT | O_TRUNC, 0600);
      static const size_t out_buffer_size = 1024;
      static const size_t in_buffer_size = 1024;
      char* in = new char[in_buffer_size];
      char* out = new char[out_buffer_size];
      z_stream* zs = new z_stream();
      memset(zs, 0, sizeof(*zs));
      zs->zalloc = Z_NULL;
      zs->zfree = Z_NULL;
      int ret = deflateInit(zs, Z_DEFAULT_COMPRESSION);
      if (ret != Z_OK)
      {
        printf("deflateInit failed %d\n", ret);
      }
      zs->next_out = reinterpret_cast<Bytef*>(out);
      zs->avail_out = out_buffer_size;

      while ((zs->avail_in = read(fd, in, in_buffer_size)))
      {
        zs->next_in = reinterpret_cast<Bytef*>(in);
        if (deflate(zs, Z_PARTIAL_FLUSH) == Z_STREAM_ERROR)
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
      deflate(zs, Z_FINISH);
      deflateEnd(zs);
      size_t avail_out = std::min<size_t>(zs->avail_out, out_buffer_size);
      if (avail_out < out_buffer_size)
      {
        write(outfd, zs->next_out, out_buffer_size - avail_out);
      }
      free(zs);
      close(outfd);
    }
  }

  return 0;
}
