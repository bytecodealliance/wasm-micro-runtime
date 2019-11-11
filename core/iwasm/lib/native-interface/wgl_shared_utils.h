/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WAMR_GRAPHIC_LIBRARY_SHARED_UTILS_H
#define WAMR_GRAPHIC_LIBRARY_SHARED_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "../3rdparty/lv_conf.h"

typedef lv_coord_t wgl_coord_t; /* lv_coord_t is defined in lv_conf.h */

/**
 * Represents a point on the screen.
 */
typedef struct
{
    lv_coord_t x;
    lv_coord_t y;
} wgl_point_t;

/** Represents an area of the screen. */
typedef struct
{
    lv_coord_t x1;
    lv_coord_t y1;
    lv_coord_t x2;
    lv_coord_t y2;
} wgl_area_t;


/** Describes the properties of a glyph. */
typedef struct
{
    uint16_t adv_w; /**< The glyph needs this space. Draw the next glyph after this width. 8 bit integer, 4 bit fractional */
    uint8_t box_w;  /**< Width of the glyph's bounding box*/
    uint8_t box_h;  /**< Height of the glyph's bounding box*/
    int8_t ofs_x;   /**< x offset of the bounding box*/
    int8_t ofs_y;  /**< y offset of the bounding box*/
    uint8_t bpp;   /**< Bit-per-pixel: 1, 2, 4, 8*/
}wgl_font_glyph_dsc_t;

/*Describe the properties of a font*/
typedef struct _wgl_font_struct
{
    /** Get a glyph's  descriptor from a font*/
    bool (*get_glyph_dsc)(const struct _wgl_font_struct *, wgl_font_glyph_dsc_t *, uint32_t letter, uint32_t letter_next);

    /** Get a glyph's bitmap from a font*/
    const uint8_t * (*get_glyph_bitmap)(const struct _wgl_font_struct *, uint32_t);

    /*Pointer to the font in a font pack (must have the same line height)*/
    uint8_t line_height;      /**< The real line height where any text fits*/
    uint8_t base_line;        /**< Base line measured from the top of the line_height*/
    void * dsc;               /**< Store implementation specific data here*/
#if LV_USE_USER_DATA
    wgl_font_user_data_t user_data; /**< Custom user data for font. */
#endif
} wgl_font_t;

#if LV_COLOR_DEPTH == 1
#define LV_COLOR_SIZE 8
#elif LV_COLOR_DEPTH == 8
#define LV_COLOR_SIZE 8
#elif LV_COLOR_DEPTH == 16
#define LV_COLOR_SIZE 16
#elif LV_COLOR_DEPTH == 32
#define LV_COLOR_SIZE 32
#else
#error "Invalid LV_COLOR_DEPTH in lv_conf.h! Set it to 1, 8, 16 or 32!"
#endif

/**********************
 *      TYPEDEFS
 **********************/

typedef union
{
    uint8_t blue : 1;
    uint8_t green : 1;
    uint8_t red : 1;
    uint8_t full : 1;
} wgl_color1_t;

typedef union
{
    struct
    {
        uint8_t blue : 2;
        uint8_t green : 3;
        uint8_t red : 3;
    } ch;
    uint8_t full;
} wgl_color8_t;

typedef union
{
    struct
    {
#if LV_COLOR_16_SWAP == 0
        uint16_t blue : 5;
        uint16_t green : 6;
        uint16_t red : 5;
#else
        uint16_t green_h : 3;
        uint16_t red : 5;
        uint16_t blue : 5;
        uint16_t green_l : 3;
#endif
    } ch;
    uint16_t full;
} wgl_color16_t;

typedef union
{
    struct
    {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
        uint8_t alpha;
    } ch;
    uint32_t full;
} wgl_color32_t;

#if LV_COLOR_DEPTH == 1
typedef uint8_t wgl_color_int_t;
typedef wgl_color1_t wgl_color_t;
#elif LV_COLOR_DEPTH == 8
typedef uint8_t wgl_color_int_t;
typedef wgl_color8_t wgl_color_t;
#elif LV_COLOR_DEPTH == 16
typedef uint16_t wgl_color_int_t;
typedef wgl_color16_t wgl_color_t;
#elif LV_COLOR_DEPTH == 32
typedef uint32_t wgl_color_int_t;
typedef wgl_color32_t wgl_color_t;
#else
#error "Invalid LV_COLOR_DEPTH in lv_conf.h! Set it to 1, 8, 16 or 32!"
#endif

typedef uint8_t wgl_opa_t;



/*Border types (Use 'OR'ed values)*/
enum {
    WGL_BORDER_NONE     = 0x00,
    WGL_BORDER_BOTTOM   = 0x01,
    WGL_BORDER_TOP      = 0x02,
    WGL_BORDER_LEFT     = 0x04,
    WGL_BORDER_RIGHT    = 0x08,
    WGL_BORDER_FULL     = 0x0F,
    WGL_BORDER_INTERNAL = 0x10, /**< FOR matrix-like objects (e.g. Button matrix)*/
};
typedef uint8_t wgl_border_part_t;

/*Shadow types*/
enum {
    WGL_SHADOW_BOTTOM = 0, /**< Only draw bottom shadow */
    WGL_SHADOW_FULL,       /**< Draw shadow on all sides */
};
typedef uint8_t wgl_shadow_type_t;

/**
 * Objects in LittlevGL can be assigned a style - which holds information about
 * how the object should be drawn.
 *
 * This allows for easy customization without having to modify the object's design
 * function.
 */
typedef struct
{
    uint8_t glass : 1; /**< 1: Do not inherit this style*/

    /** Object background. */
    struct
    {
        wgl_color_t main_color; /**< Object's main background color. */
        wgl_color_t grad_color; /**< Second color. If not equal to `main_color` a gradient will be drawn for the background. */
        wgl_coord_t radius; /**< Object's corner radius. You can use #WGL_RADIUS_CIRCLE if you want to draw a circle. */
        wgl_opa_t opa; /**< Object's opacity (0-255). */

        struct
        {
            wgl_color_t color; /**< Border color */
            wgl_coord_t width; /**< Border width */
            wgl_border_part_t part; /**< Which borders to draw */
            wgl_opa_t opa; /**< Border opacity. */
        } border;


        struct
        {
            wgl_color_t color;
            wgl_coord_t width;
            wgl_shadow_type_t type; /**< Which parts of the shadow to draw */
        } shadow;

        struct
        {
            wgl_coord_t top;
            wgl_coord_t bottom;
            wgl_coord_t left;
            wgl_coord_t right;
            wgl_coord_t inner;
        } padding;
    } body;

    /** Style for text drawn by this object. */
    struct
    {
        wgl_color_t color; /**< Text color */
        wgl_color_t sel_color; /**< Text selection background color. */
        const wgl_font_t * font;
        wgl_coord_t letter_space; /**< Space between letters */
        wgl_coord_t line_space; /**< Space between lines (vertical) */
        wgl_opa_t opa; /**< Text opacity */
    } text;

    /**< Style of images. */
    struct
    {
        wgl_color_t color; /**< Color to recolor the image with */
        wgl_opa_t intense; /**< Opacity of recoloring (0 means no recoloring) */
        wgl_opa_t opa; /**< Opacity of whole image */
    } image;

    /**< Style of lines (not borders). */
    struct
    {
        wgl_color_t color;
        wgl_coord_t width;
        wgl_opa_t opa;
        uint8_t rounded : 1; /**< 1: rounded line endings*/
    } line;
} wgl_style_t;



/* Object native function IDs */
enum {
    OBJ_FUNC_ID_DEL,
    OBJ_FUNC_ID_DEL_ASYNC,
    OBJ_FUNC_ID_CLEAN,
    OBJ_FUNC_ID_SET_EVT_CB,
    OBJ_FUNC_ID_ALIGN,

    /* Number of functions */
    _OBJ_FUNC_ID_NUM,
};

/* Button native function IDs */
enum {
    BTN_FUNC_ID_CREATE,
    BTN_FUNC_ID_SET_TOGGLE,
    BTN_FUNC_ID_SET_STATE,
    BTN_FUNC_ID_TOGGLE,
    BTN_FUNC_ID_SET_INK_IN_TIME,
    BTN_FUNC_ID_SET_INK_WAIT_TIME,
    BTN_FUNC_ID_SET_INK_OUT_TIME,
    BTN_FUNC_ID_GET_STATE,
    BTN_FUNC_ID_GET_TOGGLE,
    BTN_FUNC_ID_GET_INK_IN_TIME,
    BTN_FUNC_ID_GET_INK_WAIT_TIME,
    BTN_FUNC_ID_GET_INK_OUT_TIME,
    /* Number of functions */
    _BTN_FUNC_ID_NUM,
};

/* Check box native function IDs */
enum {
    CB_FUNC_ID_CREATE,
    CB_FUNC_ID_SET_TEXT,
    CB_FUNC_ID_SET_STATIC_TEXT,
    CB_FUNC_ID_GET_TEXT,
    CB_FUNC_ID_GET_TEXT_LENGTH,

    /* Number of functions */
    _CB_FUNC_ID_NUM,
};

/* List native function IDs */
enum {
    LIST_FUNC_ID_CREATE,
    LIST_FUNC_ID_ADD_BTN,

    /* Number of functions */
    _LIST_FUNC_ID_NUM,
};

/* Label native function IDs */
enum {
    LABEL_FUNC_ID_CREATE,
    LABEL_FUNC_ID_SET_TEXT,
    LABEL_FUNC_ID_SET_ARRAY_TEXT,
    LABEL_FUNC_ID_SET_STATIC_TEXT,
    LABEL_FUNC_ID_SET_LONG_MODE,
    LABEL_FUNC_ID_SET_ALIGN,
    LABEL_FUNC_ID_SET_RECOLOR,
    LABEL_FUNC_ID_SET_BODY_DRAW,
    LABEL_FUNC_ID_SET_ANIM_SPEED,
    LABEL_FUNC_ID_SET_TEXT_SEL_START,
    LABEL_FUNC_ID_SET_TEXT_SEL_END,
    LABEL_FUNC_ID_GET_TEXT,
    LABEL_FUNC_ID_GET_TEXT_LENGTH,
    LABEL_FUNC_ID_GET_LONG_MODE,
    LABEL_FUNC_ID_GET_ALIGN,
    LABEL_FUNC_ID_GET_RECOLOR,
    LABEL_FUNC_ID_GET_BODY_DRAW,
    LABEL_FUNC_ID_GET_ANIM_SPEED,
    LABEL_FUNC_ID_GET_LETTER_POS,
    LABEL_FUNC_ID_GET_TEXT_SEL_START,
    LABEL_FUNC_ID_GET_TEXT_SEL_END,
    LABEL_FUNC_ID_INS_TEXT,
    LABEL_FUNC_ID_CUT_TEXT,
    /* Number of functions */
    _LABEL_FUNC_ID_NUM,
};

#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_SHARED_UTILS_H */
