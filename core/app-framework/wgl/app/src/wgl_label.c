/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */


#include "wa-inc/wgl.h"
#include "gui_api.h"
#include <string.h>


#define ARGC sizeof(argv)/sizeof(uint32)
#define CALL_LABEL_NATIVE_FUNC(id) wasm_label_native_call(id, argv, ARGC)

wgl_obj_t wgl_label_create(wgl_obj_t par, wgl_obj_t copy)
{
    uint32 argv[2] = {0};

    argv[0] = (uint32)par;
    argv[1] = (uint32)copy;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_CREATE);
    return (wgl_obj_t)argv[0];
}

void wgl_label_set_text(wgl_obj_t label, const char * text)
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)label;
    argv[1] = (uint32)text;
    argv[2] = strlen(text) + 1;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_TEXT);
}


void wgl_label_set_array_text(wgl_obj_t label, const char * array, uint16_t size)
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)label;
    argv[1] = (uint32)array;
    argv[2] = size;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_ARRAY_TEXT);
}


void wgl_label_set_static_text(wgl_obj_t label, const char * text)
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)label;
    argv[1] = (uint32)text;
    argv[2] = strlen(text) + 1;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_STATIC_TEXT);
}


void wgl_label_set_long_mode(wgl_obj_t label, wgl_label_long_mode_t long_mode)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)label;
    argv[1] = long_mode;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_LONG_MODE);
}


void wgl_label_set_align(wgl_obj_t label, wgl_label_align_t align)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)label;
    argv[1] = align;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_ALIGN);
}


void wgl_label_set_recolor(wgl_obj_t label, bool en)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)label;
    argv[1] = en;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_RECOLOR);
}


void wgl_label_set_body_draw(wgl_obj_t label, bool en)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)label;
    argv[1] = en;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_BODY_DRAW);
}


void wgl_label_set_anim_speed(wgl_obj_t label, uint16_t anim_speed)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)label;
    argv[1] = anim_speed;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_ANIM_SPEED);
}


void wgl_label_set_text_sel_start(wgl_obj_t label, uint16_t index)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)label;
    argv[1] = index;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_TEXT_SEL_START);
}


void wgl_label_set_text_sel_end(wgl_obj_t label, uint16_t index)
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)label;
    argv[1] = index;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_TEXT_SEL_END);
}

unsigned int wgl_label_get_text_length(wgl_obj_t label)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_TEXT_LENGTH);
    return argv[0];
}

char * wgl_label_get_text(wgl_obj_t label, char *buffer, int buffer_len)
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)label;
    argv[1] = (uint32)buffer;
    argv[2] = buffer_len;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_TEXT);
    return (char *)argv[0];
}


wgl_label_long_mode_t wgl_label_get_long_mode(const wgl_obj_t label)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_LONG_MODE);
    return (wgl_label_long_mode_t)argv[0];
}


wgl_label_align_t wgl_label_get_align(const wgl_obj_t label)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_ALIGN);
    return (wgl_label_align_t)argv[0];
}


bool wgl_label_get_recolor(const wgl_obj_t label)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_RECOLOR);
    return (bool)argv[0];
}


bool wgl_label_get_body_draw(const wgl_obj_t label)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_BODY_DRAW);
    return (bool)argv[0];
}


uint16_t wgl_label_get_anim_speed(const wgl_obj_t label)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_ANIM_SPEED);
    return (uint16_t)argv[0];
}


void wgl_label_get_letter_pos(const wgl_obj_t label, uint16_t index, wgl_point_t * pos)
{
    uint32 argv[4] = {0};
    argv[0] = (uint32)label;
    argv[1] = index;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_LETTER_POS);
    pos->x = argv[2];
    pos->y = argv[3];
}


uint16_t wgl_label_get_letter_on(const wgl_obj_t label, wgl_point_t * pos)
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)label;
    argv[1] = pos->x;
    argv[2] = pos->y;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_LETTER_POS);
    return (uint16_t)argv[0];
}


bool wgl_label_is_char_under_pos(const wgl_obj_t label, wgl_point_t * pos)
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)label;
    argv[1] = pos->x;
    argv[2] = pos->y;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_LETTER_POS);
    return (bool)argv[0];
}


uint16_t wgl_label_get_text_sel_start(const wgl_obj_t label)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_TEXT_SEL_START);
    return (uint16_t)argv[0];
}


uint16_t wgl_label_get_text_sel_end(const wgl_obj_t label)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_TEXT_SEL_END);
    return (uint16_t)argv[0];
}


void wgl_label_ins_text(wgl_obj_t label, uint32_t pos, const char * txt)
{
    uint32 argv[4] = {0};
    argv[0] = (uint32)label;
    argv[1] = pos;
    argv[2] = (uint32)txt;
    argv[3] = strlen(txt) + 1;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_INS_TEXT);
}


void wgl_label_cut_text(wgl_obj_t label, uint32_t pos, uint32_t cnt)
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)label;
    argv[1] = pos;
    argv[2] = cnt;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_CUT_TEXT);
}


