# Copyright (C) 2019 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
)

all_compile_actions = [
    ACTION_NAMES.c_compile,
    ACTION_NAMES.cpp_compile,
]

all_link_actions = [
    ACTION_NAMES.cpp_link_executable,
    ACTION_NAMES.cpp_link_dynamic_library,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

def _impl(ctx):
    tool_paths = [
        tool_path(
            name = "gcc",
            path = "/opt/emsdk/upstream/emscripten/emcc",
        ),
        tool_path(
            name = "ld",
            path = "/opt/emsdk/upstream/emscripten/emcc",
        ),
        tool_path(
            name = "ar",
            path = "/opt/emsdk/upstream/emscripten/emar",
        ),
        tool_path(
            name = "cpp",
            path = "/opt/emsdk/upstream/emscripten/em++",
        ),
        tool_path(
            name = "gcov",
            path = "/bin/false",
        ),
        tool_path(
            name = "nm",
            path = "/bin/false",
        ),
        tool_path(
            name = "objdump",
            path = "/bin/false",
        ),
        tool_path(
            name = "strip",
            path = "/bin/false",
        ),
    ]

    features = [ # NEW
        feature(
            name = "default_compile_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_compile_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                "-O3",
                                "-msimd128",
                                "-s",
                                "USE_PTHREADS=0",
                                "-s",
                                "ERROR_ON_UNDEFINED_SYMBOLS=0",
                                "-s",
                                "STANDALONE_WASM=1",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
        feature(
            name = "default_linker_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_link_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                "-O3",
                                "-msimd128",
                                "-s",
                                "USE_PTHREADS=0",
                                "-s",
                                "ERROR_ON_UNDEFINED_SYMBOLS=0",
                                "-s",
                                "STANDALONE_WASM=1",
                                "-Wl,--export=__heap_base",
                                "-Wl,--export=__data_end",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
    ]

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features, # NEW
        cxx_builtin_include_directories = [
            "/opt/emsdk/upstream/emscripten/system/include/libcxx",
            "/opt/emsdk/upstream/emscripten/system/lib/libcxxabi/include",
            "/opt/emsdk/upstream/emscripten/system/include",
            "/opt/emsdk/upstream/emscripten/system/include/libc",
            "/opt/emsdk/upstream/emscripten/system/lib/libc/musl/arch/emscripten",
            "/opt/emsdk/upstream/lib/clang/12.0.0/include/",
        ],
        toolchain_identifier = "wasm-emsdk",
        host_system_name = "i686-unknown-linux-gnu",
        target_system_name = "wasm32-unknown-emscripten",
        target_cpu = "wasm32",
        target_libc = "unknown",
        compiler = "emsdk",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
    )

emsdk_toolchain_config = rule(
  implementation = _impl,
  attrs = {},
  provides = [CcToolchainConfigInfo],
)
