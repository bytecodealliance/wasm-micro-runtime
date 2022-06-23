#ifndef WASM_H
#define WASM_H

#include "wasm_export.h"
#include "bh_read_file.h"

void init_wasm();

void call_wasm_function();

void deInit_wasm();

void thread_function();

#endif