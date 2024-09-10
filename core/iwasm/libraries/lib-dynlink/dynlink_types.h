/*
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _DYNLINK_TYPES_H
#define _DYNLINK_TYPES_H

#include "platform_common.h"

typedef struct {
    uint32 memory_size;
    uint32 memory_alignment;
    uint32 table_size;
    uint32 table_alignment;
} DynLinkSectionMemInfo;

typedef struct {
    uint32 count;
    char **entries;
} DynLinkSectionNeeded;

typedef struct {
    char *name;
    uint32 flags;
} DynLinkSectionExportInfoEntry;

typedef struct {
    uint32 count;
    DynLinkSectionExportInfoEntry *entries;
} DynLinkSectionExportInfo;

typedef struct {
    char *module;
    char *field;
    uint32 flags;
} DynLinkSectionImportInfo;

typedef struct {
    DynLinkSectionMemInfo mem_info;
    DynLinkSectionNeeded needed;
    DynLinkSectionExportInfo export_info;
    DynLinkSectionImportInfo import_info;
} DynLinkSections;

void
dynlink_sections_deinit(DynLinkSections *sections);

#endif