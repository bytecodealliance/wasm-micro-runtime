/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WAMR_GRAPHIC_LIBRARY_BTN_H
#define WAMR_GRAPHIC_LIBRARY_BTN_H

#ifdef __cplusplus
extern "C" {
#endif

/** Possible states of a button.
 * It can be used not only by buttons but other button-like objects too*/
enum {
    /**Released*/
    WGL_BTN_STATE_REL,

    /**Pressed*/
    WGL_BTN_STATE_PR,

    /**Toggled released*/
    WGL_BTN_STATE_TGL_REL,

    /**Toggled pressed*/
    WGL_BTN_STATE_TGL_PR,

    /**Inactive*/
    WGL_BTN_STATE_INA,

    /**Number of states*/
    _WGL_BTN_STATE_NUM,
};
typedef uint8_t wgl_btn_state_t;

/**Styles*/
enum {
    /** Release style */
    WGL_BTN_STYLE_REL,

    /**Pressed style*/
    WGL_BTN_STYLE_PR,

    /** Toggle released style*/
    WGL_BTN_STYLE_TGL_REL,

    /** Toggle pressed style */
    WGL_BTN_STYLE_TGL_PR,

    /** Inactive style*/
    WGL_BTN_STYLE_INA,
};
typedef uint8_t wgl_btn_style_t;


/* Create a button */
wgl_obj_t wgl_btn_create(wgl_obj_t par, wgl_obj_t copy);

/*=====================
 * Setter functions
 *====================*/

/**
 * Enable the toggled states. On release the button will change from/to toggled state.
 * @param btn pointer to a button object
 * @param tgl true: enable toggled states, false: disable
 */
void wgl_btn_set_toggle(wgl_obj_t btn, bool tgl);

/**
 * Set the state of the button
 * @param btn pointer to a button object
 * @param state the new state of the button (from wgl_btn_state_t enum)
 */
void wgl_btn_set_state(wgl_obj_t btn, wgl_btn_state_t state);

/**
 * Toggle the state of the button (ON->OFF, OFF->ON)
 * @param btn pointer to a button object
 */
void wgl_btn_toggle(wgl_obj_t btn);

/**
 * Set time of the ink effect (draw a circle on click to animate in the new state)
 * @param btn pointer to a button object
 * @param time the time of the ink animation
 */
void wgl_btn_set_ink_in_time(wgl_obj_t btn, uint16_t time);

/**
 * Set the wait time before the ink disappears
 * @param btn pointer to a button object
 * @param time the time of the ink animation
 */
void wgl_btn_set_ink_wait_time(wgl_obj_t btn, uint16_t time);

/**
 * Set time of the ink out effect (animate to the released state)
 * @param btn pointer to a button object
 * @param time the time of the ink animation
 */
void wgl_btn_set_ink_out_time(wgl_obj_t btn, uint16_t time);

/**
 * Set a style of a button.
 * @param btn pointer to button object
 * @param type which style should be set
 * @param style pointer to a style
 *  */
//void wgl_btn_set_style(wgl_obj_t btn, wgl_btn_style_t type, const wgl_style_t * style);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the current state of the button
 * @param btn pointer to a button object
 * @return the state of the button (from wgl_btn_state_t enum)
 */
wgl_btn_state_t wgl_btn_get_state(wgl_obj_t btn);

/**
 * Get the toggle enable attribute of the button
 * @param btn pointer to a button object
 * @return true: toggle enabled, false: disabled
 */
bool wgl_btn_get_toggle(wgl_obj_t btn);

/**
 * Get time of the ink in effect (draw a circle on click to animate in the new state)
 * @param btn pointer to a button object
 * @return the time of the ink animation
 */
uint16_t wgl_btn_get_ink_in_time(wgl_obj_t btn);

/**
 * Get the wait time before the ink disappears
 * @param btn pointer to a button object
 * @return the time of the ink animation
 */
uint16_t wgl_btn_get_ink_wait_time(wgl_obj_t btn);

/**
 * Get time of the ink out effect (animate to the releases state)
 * @param btn pointer to a button object
 * @return the time of the ink animation
 */
uint16_t wgl_btn_get_ink_out_time(wgl_obj_t btn);

/**
 * Get style of a button.
 * @param btn pointer to button object
 * @param type which style should be get
 * @return style pointer to the style
 *  */
//const wgl_style_t * wgl_btn_get_style(const wgl_obj_t btn, wgl_btn_style_t type);
#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_BTN_H */
