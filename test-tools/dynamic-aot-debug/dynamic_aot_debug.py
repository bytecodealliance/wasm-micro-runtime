#!/usr/bin/env python3
#
# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

import os
import gdb

# get object file path from env，set default value "~/objects/"
# exp: export OBJ_PATH=~/work/data/debug/
path_objs = os.getenv("OBJ_PATH", "~/objects/")
# us os.path.expand user expand user path symbol（such as ~）
path_objs = os.path.expanduser(path_objs)
print(f"Object files will be loaded from: {path_objs} on localhost")

aot_module_info = {}


def add_symbol_with_aot_info(aot_module_info):
    """add symbol with aot info"""
    text_addr = aot_module_info["code"]
    file_name = aot_module_info["name"]

    file_name_without_extension, file_extension = os.path.splitext(file_name)

    if os.path.sep in file_name_without_extension:
        file_name = os.path.basename(file_name_without_extension)
    else:
        file_name = file_name_without_extension

    path_symfile = os.path.join(path_objs, file_name)

    cmd = f"add-symbol-file {path_symfile} {text_addr}"
    gdb.execute(cmd)

    breakpoints = gdb.execute("info breakpoints", to_string=True)
    print("Current breakpoints:", breakpoints)


class read_g_dynamic_aot_module(gdb.Command):
    """read_g_dynamic_aot_module"""

    def __init__(self):
        super(self.__class__, self).__init__("read_gda", gdb.COMMAND_USER)

    def invoke(self, args, from_tty):
        """invoke"""
        Aot_module = gdb.parse_and_eval("g_dynamic_aot_module")
        found_code = False
        found_name = False
        Aot_module_fields = []
        if Aot_module.type.code == gdb.TYPE_CODE_PTR:
            Aot_module = Aot_module.dereference()
            if Aot_module.type.strip_typedefs().code == gdb.TYPE_CODE_STRUCT:
                Aot_module_fields = [f.name for f in Aot_module.type.fields()]
                for field in Aot_module_fields:
                    var = Aot_module[field]
                    if field == "name":
                        aot_module_info["name"] = var.string()
                        found_name = True
                    elif field == "code":
                        aot_module_info["code"] = str(var)
                        found_code = True
                    if found_code == True and found_name == True:
                        break
            else:
                print("Aot_module not struct type!")
        else:
            print("Aot_module not struct point type!")

        add_symbol_with_aot_info(aot_module_info)


def init():
    """init"""
    # register the command to gdb
    read_g_dynamic_aot_module()
    # set a breakpoint at function __enable_dynamic_aot_debug
    breakpoint = gdb.Breakpoint("__enable_dynamic_aot_debug")
    # attach the self-defined command to the created breakpoint
    breakpoint.commands = "read_gda"


init()
