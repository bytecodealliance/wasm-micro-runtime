/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdlib.h>
#include "bh_platform.h"
#include "wasm_platform_log.h"
#include "wasm_assert.h"
#include "wasm_log.h"
#include "wasm_thread.h"
#include "wasm_memory.h"
#include "wasm_export.h"
#include "aot_export.h"

static int
print_help()
{
  wasm_printf("Usage: wamrc [options] -o output_file wasm_file\n");
  wasm_printf("  --target=<arch-name>      Set the target arch, which has the general format: <arch><sub>\n");
  wasm_printf("                            <arch> = x86_64, i386, arm, thumb, mips.\n");
  wasm_printf("                              Default is host arch, e.g. x86_64\n");
  wasm_printf("                            <sub> = for ex. on arm or thumb: v5, v6m, v7a, v7m, etc.\n");
  wasm_printf("                            Use --target=help to list supported targets\n");
  wasm_printf("  --cpu=<cpu>               Set the target CPU (default: host CPU, e.g. skylake)\n");
  wasm_printf("                            Use --target=help to list all the CPU supported\n");
  wasm_printf("  --cpu-features=<features> Enable or disable the CPU features\n");
  wasm_printf("                            Use +feature to enable a feature, or -feature to disable it\n");
  wasm_printf("                            For example, --cpu-features=+feature1,-feature2\n");
  wasm_printf("                            Use --cpu-features=+help to list all the features supported\n");
  wasm_printf("  --opt-level=n             Set the optimization level (0 to 3, default: 3)\n");
  wasm_printf("  --format=<format>         Specifies the format of the output file\n");
  wasm_printf("                            The format supported:\n");
  wasm_printf("                              aot (default)  AoT file\n");
  wasm_printf("                              object         Native object file\n");
  wasm_printf("                              llvmir-unopt   Unoptimized LLVM IR\n");
  wasm_printf("                              llvmir-opt     Optimized LLVM IR\n");
  wasm_printf("Examples: wamrc -o test.aot test.wasm\n");
  wasm_printf("          wamrc --target=i386 -o test.aot test.wasm\n");
  wasm_printf("          wamrc --target=i386 --format=object -o test.o test.wasm\n");
  return 1;
}

int
main(int argc, char *argv[])
{
  char *wasm_file_name = NULL, *out_file_name = NULL;
  uint8 *wasm_file = NULL;
  uint32 wasm_file_size;
  wasm_module_t wasm_module = NULL;
  aot_comp_data_t comp_data = NULL;
  aot_comp_context_t comp_ctx = NULL;
  AOTCompOption option = { 0 };
  char error_buf[128];
  int log_verbose_level = 1;

  option.opt_level = 3;
  option.output_format = AOT_FORMAT_FILE;

  /* Process options.  */
  for (argc--, argv++; argc > 0 && argv[0][0] == '-'; argc--, argv++) {
    if (!strcmp(argv[0], "-o")) {
      argc--, argv++;
      if (argc < 2)
        return print_help();
      out_file_name = argv[0];
    }
    else if (!strncmp(argv[0], "--target=", 9)) {
        if (argv[0][9] == '\0')
            return print_help();
        option.target_triple = argv[0] + 9;
    }
    else if (!strncmp(argv[0], "--cpu=", 6)) {
        if (argv[0][6] == '\0')
            return print_help();
        option.target_cpu = argv[0] + 6;
    }
    else if (!strncmp(argv[0], "--cpu-features=", 15)) {
        if (argv[0][15] == '\0')
            return print_help();
        option.cpu_features = argv[0] + 15;
    }
    else if (!strncmp(argv[0], "--opt-level=", 12)) {
        if (argv[0][12] == '\0')
            return print_help();
        option.opt_level = (uint32)atoi(argv[0] + 12);
        if (option.opt_level > 3)
            option.opt_level = 3;
    }
    else if (!strncmp(argv[0], "--format=", 9)) {
        if (argv[0][9] == '\0')
            return print_help();
        if (!strcmp(argv[0] + 9, "aot"))
          option.output_format = AOT_FORMAT_FILE;
        else if (!strcmp(argv[0] + 9, "object"))
          option.output_format = AOT_OBJECT_FILE;
        else if (!strcmp(argv[0] + 9, "llvmir-unopt"))
          option.output_format = AOT_LLVMIR_UNOPT_FILE;
        else if (!strcmp(argv[0] + 9, "llvmir-opt"))
          option.output_format = AOT_LLVMIR_OPT_FILE;
        else {
            wasm_printf("Invalid format %s.\n", argv[0] + 9);
            return print_help();
        }
    }
    else
      return print_help();
  }

  if (argc == 0)
    return print_help();

  wasm_file_name = argv[0];

  if (bh_memory_init_with_allocator(malloc, free)) {
      wasm_printf("Init memory with memory allocator failed.\n");
      return -1;
  }

  /* initialize runtime environment */
  if (!wasm_runtime_init())
    goto fail1;

  wasm_log_set_verbose_level(log_verbose_level);

  /* load WASM byte buffer from WASM bin file */
  if (!(wasm_file = (uint8*)
        bh_read_file_to_buffer(wasm_file_name, &wasm_file_size)))
    goto fail2;

  /* load WASM module */
  if (!(wasm_module = wasm_runtime_load(wasm_file, wasm_file_size,
                                        error_buf, sizeof(error_buf)))) {
    wasm_printf("%s\n", error_buf);
    goto fail3;
  }

  if (!(comp_data = aot_create_comp_data(wasm_module))) {
    wasm_printf("%s\n", aot_get_last_error());
    goto fail4;
  }

  if (!(comp_ctx = aot_create_comp_context(comp_data,
                                           &option))) {
    wasm_printf("%s\n", aot_get_last_error());
    goto fail5;
  }

  if (!aot_compile_wasm(comp_ctx)) {
    wasm_printf("%s\n", aot_get_last_error());
    goto fail6;
  }

  switch (option.output_format) {
      case AOT_LLVMIR_UNOPT_FILE:
      case AOT_LLVMIR_OPT_FILE:
          if (!aot_emit_llvm_file(comp_ctx, out_file_name)) {
              wasm_printf("%s\n", aot_get_last_error());
              goto fail6;
          }
          break;
      case AOT_OBJECT_FILE:
          if (!aot_emit_object_file(comp_ctx, out_file_name)) {
              wasm_printf("%s\n", aot_get_last_error());
              goto fail6;
          }
          break;
      case AOT_FORMAT_FILE:
          if (!aot_emit_aot_file(comp_ctx, comp_data, out_file_name)) {
              wasm_printf("%s\n", aot_get_last_error());
              goto fail6;
          }
          break;
      default:
          break;
  }

  wasm_printf("Compile success, file %s was generated.\n", out_file_name);

fail6:
  /* Destroy compiler context */
  aot_destroy_comp_context(comp_ctx);

fail5:
  /* Destroy compile data */
  aot_destroy_comp_data(comp_data);

fail4:
  /* Unload WASM module */
  wasm_runtime_unload(wasm_module);

fail3:
  /* free the file buffer */
  wasm_free(wasm_file);

fail2:
  /* Destroy runtime environment */
  wasm_runtime_destroy();

fail1:
  bh_memory_destroy();

  return 0;
}

