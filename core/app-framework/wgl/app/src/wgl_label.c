/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */



#include "wa-inc/lvgl/lvgl.h"
#include "gui_api.h"
#include <string.h>


#define ARGC sizeof(argv)/sizeof(uint32)
#define CALL_LABEL_NATIVE_FUNC(id) wasm_label_native_call(id, argv, ARGC)

lv_obj_t * lv_label_create(lv_obj_t * par, const lv_obj_t * copy);
{
    uint32 argv[2] = {0};

    argv[0] = (uint32)par;
    argv[1] = (uint32)copy;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_CREATE);
    return (wgl_obj_t)argv[0];
}

void lv_label_set_text(lv_obj_t * label, const char * text);
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)label;
    argv[1] = (uint32)text;
    argv[2] = strlen(text) + 1;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_TEXT);
}


void lv_label_set_array_text(lv_obj_t * label, const char * array, uint16_t size);
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)label;
    argv[1] = (uint32)array;
    argv[2] = size;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_ARRAY_TEXT);
}


void lv_label_set_static_text(lv_obj_t * label, const char * text);
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)label;
    argv[1] = (uint32)text;
    argv[2] = strlen(text) + 1;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_STATIC_TEXT);
}


void lv_label_set_long_mode(lv_obj_t * label, lv_label_long_mode_t long_mode);
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)label;
    argv[1] = long_mode;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_LONG_MODE);
}


void lv_label_set_align(lv_obj_t * label, lv_label_align_t align);
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)label;
    argv[1] = align;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_ALIGN);
}


void lv_label_set_recolor(lv_obj_t * label, bool en);
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)label;
    argv[1] = en;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_RECOLOR);
}


void lv_label_set_body_draw(lv_obj_t * label, bool en);
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)label;
    argv[1] = en;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_BODY_DRAW);
}


void lv_label_set_anim_speed(lv_obj_t * label, uint16_t anim_speed);
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)label;
    argv[1] = anim_speed;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_ANIM_SPEED);
}


void lv_label_set_text_sel_start(lv_obj_t * label, uint16_t index);
{
    uint32 argv[2] = {0};
    argv[0] = (uint32)label;
    argv[1] = index;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_SET_TEXT_SEL_START);
}


void lv_label_set_text_sel_end(lv_obj_t * label, uint16_t index);
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

// TODO: 
char * lv_label_get_text(const lv_obj_t * label)
{

}


wgl_label_long_mode_t wgl_label_get_long_mode(const wgl_obj_t label)
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_LONG_MODE);
    return (wgl_label_long_mode_t)argv[0];
}


lv_label_long_mode_t lv_label_get_long_mode(const lv_obj_t * label);
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_ALIGN);
    return (wgl_label_align_t)argv[0];
}


bool lv_label_get_recolor(const lv_obj_t * label);
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_RECOLOR);
    return (bool)argv[0];
}


bool lv_label_get_body_draw(const lv_obj_t * label);
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_BODY_DRAW);
    return (bool)argv[0];
}


uint16_t lv_label_get_anim_speed(const lv_obj_t * label);
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_ANIM_SPEED);
    return (uint16_t)argv[0];
}


void lv_label_get_letter_pos(const lv_obj_t * label, uint16_t index, lv_point_t * pos);
{
    uint32 argv[4] = {0};
    argv[0] = (uint32)label;
    argv[1] = index;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_LETTER_POS);
    pos->x = argv[2];
    pos->y = argv[3];
}


uint16_t lv_label_get_letter_on(const lv_obj_t * label, lv_point_t * pos);
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)label;
    argv[1] = pos->x;
    argv[2] = pos->y;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_LETTER_POS);
    return (uint16_t)argv[0];
}


bool lv_label_is_char_under_pos(const lv_obj_t * label, lv_point_t * pos);
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)label;
    argv[1] = pos->x;
    argv[2] = pos->y;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_LETTER_POS);
    return (bool)argv[0];
}


uint16_t lv_label_get_text_sel_start(const lv_obj_t * label);
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_TEXT_SEL_START);
    return (uint16_t)argv[0];
}


uint16_t lv_label_get_text_sel_end(const lv_obj_t * label);
{
    uint32 argv[1] = {0};
    argv[0] = (uint32)label;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_GET_TEXT_SEL_END);
    return (uint16_t)argv[0];
}


void lv_label_ins_text(lv_obj_t * label, uint32_t pos, const char * txt);
{
    uint32 argv[4] = {0};
    argv[0] = (uint32)label;
    argv[1] = pos;
    argv[2] = (uint32)txt;
    argv[3] = strlen(txt) + 1;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_INS_TEXT);
}


void lv_label_cut_text(lv_obj_t * label, uint32_t pos, uint32_t cnt);
{
    uint32 argv[3] = {0};
    argv[0] = (uint32)label;
    argv[1] = pos;
    argv[2] = cnt;
    CALL_LABEL_NATIVE_FUNC(LABEL_FUNC_ID_CUT_TEXT);
}


