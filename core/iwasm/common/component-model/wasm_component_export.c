/*
 * Copyright (C) 2026 Airbus Defence and Space Romania SRL. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_component_export.h"
#include "wasm_component.h"

static bool is_component = false;

bool is_component_runtime() {
    return is_component;
}

void set_component_runtime(bool type) {
    is_component = type;
}
