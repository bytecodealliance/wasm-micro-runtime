#
# Copyright (c) 2021, RT-Thread Development Team
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

# for module compiling
import os

from building import *

objs = []
cwd  = GetCurrentDir()
list = os.listdir(cwd)

if GetDepend(['PKG_USING_WAMR']):
    wamr_entry_sconscript = os.path.join(cwd, "product-mini", "platforms", "rt-thread", 'SConscript')

    if os.path.isfile(wamr_entry_sconscript):
        objs = objs + SConscript(wamr_entry_sconscript)
    else:
        print("[WAMR] entry script wrong:", wamr_entry_sconscript)
        Return('objs')

    wamr_runlib_sconsript = os.path.join(cwd, "build-scripts", 'SConscript')

    if os.path.isfile(wamr_runlib_sconsript):
        objs = objs + SConscript(wamr_runlib_sconsript)
    else:
        print("[WAMR] runtime lib script wrong:", wamr_runlib_sconsript)

Return('objs')

