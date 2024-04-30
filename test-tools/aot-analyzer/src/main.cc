/*
 * Copyright (C) 2024 Xiaomi Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "aot_file.h"
#include "common.h"
#include "option_parser.h"
#include "wasm_file.h"

using namespace analyzer;

static const char s_description[] =
    R"(  Print information about the contents of AoT binaries.

examples:
  $ aot-analyzer example.aot
)";

struct func_info {
    uint32_t idx;
    void *ptr;
};

static int compare_func_ptrs(const void *f1, const void *f2) {
    return (intptr_t)((struct func_info *)f1)->ptr -
           (intptr_t)((struct func_info *)f2)->ptr;
}

static struct func_info *sort_func_ptrs(const AOTModule *module) {
    uint64_t content_len;
    struct func_info *sorted_func_ptrs;
    unsigned i;

    content_len = (uint64_t)sizeof(struct func_info) * module->func_count;
    sorted_func_ptrs = (struct func_info *)wasm_runtime_malloc(content_len);
    if (!sorted_func_ptrs) {
        return NULL;
    }

    for (i = 0; i < module->func_count; i++) {
        sorted_func_ptrs[i].idx = i;
        sorted_func_ptrs[i].ptr = module->func_ptrs[i];
    }

    qsort(sorted_func_ptrs, module->func_count, sizeof(struct func_info),
          compare_func_ptrs);
    return sorted_func_ptrs;
}

static ObjdumpOptions s_objdump_options;

static std::vector<const char *> s_infiles;

static void ParseOptions(int argc, char **argv) {
    OptionParser parser("aot-analyzer", s_description);

    parser.AddOption('i', "info", "Print basic information about AoT",
                     []() { s_objdump_options.info = true; });
    parser.AddOption('t', "text-size", "Print text size",
                     []() { s_objdump_options.text_size = true; });
    parser.AddOption('x', "details", "Show AoT details",
                     []() { s_objdump_options.details = true; });
    parser.AddOption('c', "compare",
                     "Show the differences between AoT and WASM",
                     []() { s_objdump_options.compare = true; });
    parser.AddArgument(
        "filename", OptionParser::ArgumentCount::OneOrMore,
        [](const char *argument) { s_infiles.push_back(argument); });
    parser.Parse(argc, argv);
}

void InitStdio() {
#if COMPILER_IS_MSVC
    int result = _setmode(_fileno(stdout), _O_BINARY);
    if (result == -1) {
        perror("Cannot set mode binary to stdout");
    }
    result = _setmode(_fileno(stderr), _O_BINARY);
    if (result == -1) {
        perror("Cannot set mode binary to stderr");
    }
#endif
}

void DumpInfo(AoTFile *aot) {
    const AOTTargetInfo target_info = aot->GetTargetInfo();
    printf("AOT File Information: \n\n");
    printf("Binary type: %s \n",
           aot->GetBinTypeName(target_info.bin_type).c_str());
    printf("ABI type: %d \n", target_info.abi_type);
    printf("Exectuion type: %s \n",
           aot->GetExectuionTypeName(target_info.e_type).c_str());
    printf("Exectuion machine: %s \n",
           aot->GetExectuionMachineName(target_info.e_machine).c_str());
    printf("Exectuion version: %d \n", target_info.e_version);
    printf("Exectuion flags: %d \n", target_info.e_flags);
    printf("Feature flags: %ld \n", target_info.feature_flags);
    printf("Reserved: %ld \n", target_info.reserved);
    printf("Arch: %s \n", target_info.arch);
}

void DumpTextSize(AoTFile *aot) {
    const AOTTargetInfo target_info = aot->GetTargetInfo();
    printf("%s:       file format <%s> \n\n", aot->GetFileName(),
           aot->GetBinTypeName(target_info.bin_type).c_str());
    printf("Text size: \n");

    const uint32_t literal_size =
        ((AOTModule *)(aot->GetModule()))->literal_size;
    const uint32_t code_size = ((AOTModule *)(aot->GetModule()))->code_size;
    printf("   literal size= %d Bytes \n", literal_size);
    printf("      code size= %d Bytes \n", code_size);
}

void DumpDetails(AoTFile *aot) {
    const AOTTargetInfo target_info = aot->GetTargetInfo();
    printf("%s:          file format <%s> \n\n", aot->GetFileName(),
           aot->GetBinTypeName(target_info.bin_type).c_str());
    printf("Details: \n\n");

    // Types
    const uint32_t type_count = ((AOTModule *)(aot->GetModule()))->type_count;
    AOTType **types = ((AOTModule *)(aot->GetModule()))->types;
    printf("Types[%d] \n", type_count);

    const char *wasm_type[] = {"function", "struct", "array"};
    for (uint32_t index = 0; index < type_count; index++) {
        AOTType *type = types[index];
        const uint16_t type_flag = type->type_flag;
        if (type_flag == WASM_TYPE_FUNC) {
            printf("  -[%d] %s    param_count:%d    result_count:%d \n", index,
                   wasm_type[type_flag], ((AOTFuncType *)type)->param_count,
                   ((AOTFuncType *)type)->result_count);
        } else if (type_flag == WASM_TYPE_STRUCT) {
            printf("  -[%d] %s    field_count:%d \n", index,
                   wasm_type[type_flag], ((AOTStructType *)type)->field_count);
        } else if (type_flag == WASM_TYPE_ARRAY) {
            printf("  -[%d] %s    elem_type:%d \n", index, wasm_type[type_flag],
                   ((AOTArrayType *)type)->elem_type);
        } else {
            printf("  -[%d] unknown type \n", index);
        }
    }
    printf("\n\n");

    // Imports
    const uint32_t import_memory_count =
        ((AOTModule *)(aot->GetModule()))->import_memory_count;
    AOTImportMemory *import_memories =
        ((AOTModule *)(aot->GetModule()))->import_memories;
    const uint32_t import_table_count =
        ((AOTModule *)(aot->GetModule()))->import_table_count;
    AOTImportTable *import_tables =
        ((AOTModule *)(aot->GetModule()))->import_tables;
    const uint32_t import_global_count =
        ((AOTModule *)(aot->GetModule()))->import_global_count;
    AOTImportGlobal *import_globals =
        ((AOTModule *)(aot->GetModule()))->import_globals;
    const uint32_t import_func_count =
        ((AOTModule *)(aot->GetModule()))->import_func_count;
    AOTImportFunc *import_funcs =
        ((AOTModule *)(aot->GetModule()))->import_funcs;
    printf("Imports[%d] \n", import_memory_count + import_table_count +
                                 import_global_count + import_func_count);

    // import memory
    printf("  -memories[%d] \n", import_memory_count);
    for (uint32_t index = 0; index < import_memory_count; index++) {
        AOTImportMemory memory = import_memories[index];
        printf(
            "    -[%d] num_bytes_per_page:%5d    init_page_count:%5d    "
            "max_page_count:%5d    module_name: %s    memory_name: %s \n",
            index, memory.num_bytes_per_page, memory.mem_init_page_count,
            memory.mem_max_page_count, memory.module_name, memory.memory_name);
    }
    printf("\n");

    // import table
    printf("  -tables[%d] \n", import_table_count);
    for (uint32_t index = 0; index < import_table_count; index++) {
        AOTImportTable table = import_tables[index];
        printf(
            "    -[%d] elem_type:%5d    init_size:%5d    max_size:%5d    "
            "module_name: %s    table_name: %s \n",
            index, table.elem_type, table.table_init_size, table.table_max_size,
            table.module_name, table.table_name);
    }
    printf("\n");

    // import global
    printf("  -globals[%d] \n", import_global_count);
    for (uint32_t index = 0; index < import_global_count; index++) {
        AOTImportGlobal global = import_globals[index];
        printf("    -[%d] type:%5d    module_name: %s    global_name: %s \n",
               index, global.type, global.module_name, global.global_name);
    }
    printf("\n");

    // import function
    printf("  -functions[%d] \n", import_func_count);
    for (uint32_t index = 0; index < import_func_count; index++) {
        AOTImportFunc func = import_funcs[index];
        printf("    -[%d] module_name: %s    function_name: %s \n", index,
               func.module_name, func.func_name);
    }
    printf("\n\n");

    // Functions
    const uint32_t func_count = ((AOTModule *)(aot->GetModule()))->func_count;
    const uint32_t code_size = ((AOTModule *)(aot->GetModule()))->code_size;
    struct func_info *sorted_func_ptrs = NULL;
    sorted_func_ptrs = sort_func_ptrs(((AOTModule *)(aot->GetModule())));

    if (sorted_func_ptrs) {
        printf("Function[%d] \n", func_count);
        for (uint32_t index = 0; index < func_count; index++) {
            const uint32_t func_size =
                index + 1 < func_count
                    ? (uintptr_t)(sorted_func_ptrs[index + 1].ptr) -
                          (uintptr_t)(sorted_func_ptrs[index].ptr)
                    : code_size +
                          (uintptr_t)(((AOTModule *)(aot->GetModule()))->code) -
                          (uintptr_t)(sorted_func_ptrs[index].ptr);
            printf("  -[%d] code_size= %d Bytes \n", index, func_size);
        }
        printf("\n\n");
    }

    // Tables
    const uint32_t table_count = ((AOTModule *)(aot->GetModule()))->table_count;
    AOTTable *tables = ((AOTModule *)(aot->GetModule()))->tables;
    printf("Tables[%d] \n", table_count);

    for (uint32_t index = 0; index < table_count; index++) {
        AOTTable table = tables[index];
        printf("  -[%d] elem_type:5%d    init_size:%5d    max_size:%5d \n",
               index, table.elem_type, table.table_init_size,
               table.table_max_size);
    }
    printf("\n\n");

    // Memories
    const uint32_t memory_count =
        ((AOTModule *)(aot->GetModule()))->memory_count;
    AOTMemory *memories = ((AOTModule *)(aot->GetModule()))->memories;
    printf("Memories[%d] \n", memory_count);

    for (uint32_t index = 0; index < memory_count; index++) {
        AOTMemory memory = memories[index];
        printf(
            "  -[%d] memory_flags:%5d    bytes_per_page:%5d    "
            "init_page_count:%5d    max_page_count:%5d \n",
            index, memory.memory_flags, memory.num_bytes_per_page,
            memory.mem_init_page_count, memory.mem_max_page_count);
    }
    printf("\n\n");

    // Globals
    const uint32_t global_count =
        ((AOTModule *)(aot->GetModule()))->global_count;
    AOTGlobal *globals = ((AOTModule *)(aot->GetModule()))->globals;
    printf("Globals[%d] \n", global_count);

    for (uint32_t index = 0; index < global_count; index++) {
        AOTGlobal global = globals[index];
        printf("  -[%d] type:%5d    mutable:%5d    offset:%5d \n", index,
               global.type, global.is_mutable, global.data_offset);
    }
    printf("\n\n");

    // Exports
    const uint32_t export_count =
        ((AOTModule *)(aot->GetModule()))->export_count;
    AOTExport *exports = ((AOTModule *)(aot->GetModule()))->exports;
    printf("Exports[%d] \n", export_count);

    for (uint32_t index = 0; index < export_count; index++) {
        AOTExport expt = exports[index];
        printf("  -[%d] kind:%5d    index:%5d    name: %s \n", index, expt.kind,
               expt.index, expt.name);
    }
    printf("\n\n");

    // Code
    const uint32_t aot_code_size = (aot->GetMemConsumption()).aot_code_size;
    const uint32_t literal_size =
        ((AOTModule *)(aot->GetModule()))->literal_size;
    const uint32_t data_section_count =
        ((AOTModule *)(aot->GetModule()))->data_section_count;

    printf("Codes[%d] \n", aot_code_size);
    printf("  -code \n");
    printf("    -code_size: %d Bytes \n", code_size);
    printf("\n");
    printf("  -literal \n");
    printf("    -literal_size: %d Bytes \n", literal_size);
    printf("\n");
    printf("  -data section \n");
    for (uint32_t index = 0; index < data_section_count; index++) {
        AOTObjectDataSection *obj_data =
            ((AOTModule *)(aot->GetModule()))->data_sections + index;
        printf("    -[%d] code_size:%5d Bytes    name: %s \n", index,
               obj_data->size, obj_data->name);
    }
    printf("\n\n");
}

void DumpCompare(AoTFile *aot, WasmFile *wasm) {
    const AOTTargetInfo target_info = aot->GetTargetInfo();
    AOTModule *aot_module = (AOTModule *)(aot->GetModule());
    WASMModuleMemConsumption aot_mem_conspn = aot->GetMemConsumption();

    WASMModule *wasm_module = (WASMModule *)(wasm->GetModule());
    WASMModuleMemConsumption wasm_mem_conspn = wasm->GetMemConsumption();

    const uint32_t aot_func_count = aot_module->func_count;
    const uint32_t aot_code_size = aot_module->code_size;
    struct func_info *sorted_func_ptrs = NULL;
    sorted_func_ptrs = sort_func_ptrs(((AOTModule *)(aot->GetModule())));

    const uint32_t wasm_func_count = wasm_module->function_count;
    WASMFunction **wasm_functions = wasm_module->functions;

    if (aot_func_count != wasm_func_count) {
        printf(
            "The number of AoT functions does not match the number of Wasm "
            "functions. \n");
        return;
    }

    uint32_t wasm_code_size = 0;
    // print function Comparison Details
    printf(
        "|--------------------------------------------------------------------"
        "-------------------| \n");
    printf(
        "|                             Function Code Size Compare             "
        "                   | \n");
    printf(
        "|--------------------------------------------------------------------"
        "-------------------| \n");
    printf(
        "|   ID   |  AoT Function Code Size   |  Wasm Function Code Size   |  "
        "expansion multiple | \n");
    printf(
        "|--------------------------------------------------------------------"
        "-------------------| \n");

    for (uint32_t index = 0; index < aot_func_count; index++) {
        const uint32_t aot_func_size =
            index + 1 < aot_func_count
                ? (uintptr_t)(sorted_func_ptrs[index + 1].ptr) -
                      (uintptr_t)(sorted_func_ptrs[index].ptr)
                : aot_code_size + (uintptr_t)(aot_module->code) -
                      (uintptr_t)(sorted_func_ptrs[index].ptr);
        const uint32_t wasm_func_size = wasm_functions[index]->code_size;
        wasm_code_size += wasm_func_size;
        printf(
            "|  %4d  |    %10d Bytes       |    %10d Bytes        |  %10.2f    "
            " "
            "    | \n",
            index, aot_func_size, wasm_func_size,
            (aot_func_size * 1.0) / wasm_func_size);
        printf(
            "|-----------------------------------------------------------------"
            "-"
            "---------------------| \n");
    }
    printf("\n\n");

    printf(
        "|--------------------------------------------------------------------"
        "---| \n");
    printf(
        "|                        Total Code Size Compare                     "
        "   | \n");
    printf(
        "|--------------------------------------------------------------------"
        "---| \n");
    printf("|  AoT code size= %10d Bytes  |  Wasm code size= %10d Bytes | \n",
           aot_code_size, wasm_code_size);
    printf(
        "|--------------------------------------------------------------------"
        "---| \n");
}

int ProgramMain(int argc, char **argv) {
    InitStdio();

    ParseOptions(argc, argv);
    if (!s_objdump_options.info && !s_objdump_options.text_size &&
        !s_objdump_options.details && !s_objdump_options.compare) {
        fprintf(stderr,
                "At least one of the following switches must be given: \n");
        fprintf(stderr, " -i/ --info \n");
        fprintf(stderr, " -t/ --text-size \n");
        fprintf(stderr, " -x/ --details \n");
        fprintf(stderr, " -c/ --compare \n");
        return 1;
    }

    std::vector<BinaryFile *> readers;
    for (const char *filename : s_infiles) {
        BinaryFile *reader = NULL;
        const char *dot = strrchr(filename, '.');
        if (!dot) {
            printf("bad file name: %s \n", filename);
            continue;
        }

        if (strncmp(dot, ".aot", 4) == 0) {
            reader = new AoTFile(filename);
        } else if (strncmp(dot, ".wasm", 4) == 0) {
            reader = new WasmFile(filename);
        } else {
            printf("unkown file extension: %s \n", dot);
            continue;
        }

        if (reader && reader->ReadModule() == Result::Error) {
            printf("read module failed. \n");
            continue;
        }

        CHECK_RESULT(reader->Scan());
        readers.push_back(reader);
    }

    // -i/ --info
    if (s_objdump_options.info == 1) {
        for (size_t i = 0; i < readers.size(); ++i) {
            printf("\n");

            BinaryFile *reader = readers[i];
            const uint32_t module_type = reader->GetModule()->module_type;
            if (module_type == Wasm_Module_AoT) {
                AoTFile *aot = dynamic_cast<AoTFile *>(reader);
                if (!aot) {
                    printf("[DumpInfo]: Reader cast failed. \n");
                    continue;
                }
                DumpInfo(aot);
            } else {
                printf("[DumpInfo]: Wrong file format, not an AoT file. \n");
            }
        }
    }

    // -t/ --text-size
    if (s_objdump_options.text_size == 1) {
        for (size_t i = 0; i < readers.size(); ++i) {
            printf("\n");

            BinaryFile *reader = readers[i];
            const uint32_t module_type = reader->GetModule()->module_type;
            if (module_type == Wasm_Module_AoT) {
                AoTFile *aot = dynamic_cast<AoTFile *>(reader);
                if (!aot) {
                    printf("[DumpTextSize]: Reader cast failed. \n");
                    continue;
                }
                DumpTextSize(aot);
            } else {
                printf(
                    "[DumpTextSize]: Wrong file format, not an AoT file. \n");
            }
        }
    }

    // -x/ --details
    if (s_objdump_options.details == 1) {
        for (size_t i = 0; i < readers.size(); ++i) {
            printf("\n");

            BinaryFile *reader = readers[i];
            const uint32_t module_type = reader->GetModule()->module_type;
            if (module_type == Wasm_Module_AoT) {
                AoTFile *aot = dynamic_cast<AoTFile *>(reader);
                if (!aot) {
                    printf("[DumpDetails]: Reader cast failed. \n");
                    continue;
                }
                DumpDetails(aot);
            } else {
                printf("[DumpDetails]: Wrong file format, not an AoT file. \n");
            }
        }
    }

    // -c/ --compare
    if (s_objdump_options.compare == 1) {
        printf("\n");

        if (readers.size() != 2) {
            printf("[DumpCompare]: Illegal number of file parameters. \n");
            return 1;
        }

        AoTFile *aot = NULL;
        WasmFile *wasm = NULL;
        for (size_t i = 0; i < readers.size(); ++i) {
            BinaryFile *reader = readers[i];
            const uint32_t module_type = reader->GetModule()->module_type;
            if (module_type == Wasm_Module_AoT) {
                aot = dynamic_cast<AoTFile *>(reader);
            } else if (module_type == Wasm_Module_Bytecode) {
                wasm = dynamic_cast<WasmFile *>(reader);
            }
        }
        if (!aot) {
            printf("[DumpCompare]: an aot file is required for comparison. \n");
            return 1;
        }
        if (!wasm) {
            printf("[DumpCompare]: a wasm file is required for comparison. \n");
            return 1;
        }
        DumpCompare(aot, wasm);
    }
    return 0;
}

int main(int argc, char **argv) {
    ANALYZER_TRY
    return ProgramMain(argc, argv);
    ANALYZER_CATCH_BAD_ALLOC_AND_EXIT
}
