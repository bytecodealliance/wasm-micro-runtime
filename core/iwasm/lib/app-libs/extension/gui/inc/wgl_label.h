/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WAMR_GRAPHIC_LIBRARY_LABEL_H
#define WAMR_GRAPHIC_LIBRARY_LABEL_H

#ifdef __cplusplus
extern "C" {
#endif

/** Long mode behaviors. Used in 'wgl_label_ext_t' */
enum {
    WGL_LABEL_LONG_EXPAND,    /**< Expand the object size to the text size*/
    WGL_LABEL_LONG_BREAK,     /**< Keep the object width, break the too long lines and expand the object
                                height*/
    WGL_LABEL_LONG_DOT,       /**< Keep the size and write dots at the end if the text is too long*/
    WGL_LABEL_LONG_SROLL,      /**< Keep the size and roll the text back and forth*/
    WGL_LABEL_LONG_SROLL_CIRC, /**< Keep the size and roll the text circularly*/
    WGL_LABEL_LONG_CROP,      /**< Keep the size and crop the text out of it*/
};
typedef uint8_t wgl_label_long_mode_t;

/** Label align policy*/
enum {
    WGL_LABEL_ALIGN_LEFT, /**< Align text to left */
    WGL_LABEL_ALIGN_CENTER, /**< Align text to center */
    WGL_LABEL_ALIGN_RIGHT, /**< Align text to right */
};
typedef uint8_t wgl_label_align_t;

/** Label styles*/
enum {
    WGL_LABEL_STYLE_MAIN,
};
typedef uint8_t wgl_label_style_t;

/* Create a label */
wgl_obj_t wgl_label_create(wgl_obj_t par, wgl_obj_t copy);

/* Set text for the label */
void wgl_label_set_text(wgl_obj_t label, const char * text);

/**
 * Get the length of the text of a label
 * @param label the label object
 * @return the length of the text of the label
 */
unsigned int wgl_label_get_text_length(wgl_obj_t label);

/**
 * Get the text of a label
 * @param label the label object
 * @param buffer buffer to save the text
 * @param buffer_len length of the buffer
 * @return the text of the label, note that the text will be truncated if buffer is not long enough
 */
char *wgl_label_get_text(wgl_obj_t label, char *buffer, int buffer_len);


#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_LABEL_H */
