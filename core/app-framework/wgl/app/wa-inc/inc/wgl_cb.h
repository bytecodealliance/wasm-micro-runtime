/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WAMR_GRAPHIC_LIBRARY_CB_H
#define WAMR_GRAPHIC_LIBRARY_CB_H

#ifdef __cplusplus
extern "C" {
#endif

/** Checkbox styles. */
enum {
    WGL_CB_STYLE_BG, /**< Style of object background. */
    WGL_CB_STYLE_BOX_REL, /**< Style of box (released). */
    WGL_CB_STYLE_BOX_PR, /**< Style of box (pressed). */
    WGL_CB_STYLE_BOX_TGL_REL, /**< Style of box (released but checked). */
    WGL_CB_STYLE_BOX_TGL_PR, /**< Style of box (pressed and checked). */
    WGL_CB_STYLE_BOX_INA, /**< Style of disabled box */
};
typedef uint8_t wgl_cb_style_t;


/**
 * Create a check box objects
 * @param par pointer to an object, it will be the parent of the new check box
 * @param copy pointer to a check box object, if not NULL then the new object will be copied from it
 * @return pointer to the created check box
 */
wgl_obj_t wgl_cb_create(wgl_obj_t par, const wgl_obj_t copy);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the text of a check box. `txt` will be copied and may be deallocated
 * after this function returns.
 * @param cb pointer to a check box
 * @param txt the text of the check box. NULL to refresh with the current text.
 */
void wgl_cb_set_text(wgl_obj_t cb, const char * txt);

/**
 * Set the text of a check box. `txt` must not be deallocated during the life
 * of this checkbox.
 * @param cb pointer to a check box
 * @param txt the text of the check box. NULL to refresh with the current text.
 */
void wgl_cb_set_static_text(wgl_obj_t cb, const char * txt);


/*=====================
 * Getter functions
 *====================*/


/**
 * Get the length of the text of a check box
 * @param label the check box object
 * @return the length of the text of the check box
 */
unsigned int wgl_cb_get_text_length(wgl_obj_t cb);

/**
 * Get the text of a check box
 * @param label the check box object
 * @param buffer buffer to save the text
 * @param buffer_len length of the buffer
 * @return the text of the check box, note that the text will be truncated if buffer is not long enough
 */
char *wgl_cb_get_text(wgl_obj_t cb, char *buffer, int buffer_len);

#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_CB_H */
