/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <unistd.h>
#define SDL_MAIN_HANDLED        /*To fix SDL's "undefined reference to WinMain" issue*/
#include <SDL2/SDL.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/monitor.h"
#include "lv_drivers/indev/mouse.h"
#include "lv_drivers/indev/mousewheel.h"
#include "lv_drivers/indev/keyboard.h"


/*********************
 *      DEFINES
 *********************/

/*On OSX SDL needs different handling*/
#if defined(__APPLE__) && defined(TARGET_OS_MAC)
# if __APPLE__ && TARGET_OS_MAC
#define SDL_APPLE
# endif
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void hal_init(void);
static void btn_event_cb(lv_obj_t * btn, lv_event_t event);

/**********************
 *  STATIC VARIABLES
 **********************/
uint32_t count = 0;
char count_str[11] = { 0 };
lv_obj_t *hello_world_label;
lv_obj_t *count_label;
lv_obj_t * btn1;
lv_obj_t * label_count1;
int label_count1_value = 0;
char label_count1_str[11] = { 0 };

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int main(int argc, char ** argv)
{
    (void) argc;    /*Unused*/
    (void) argv;    /*Unused*/

    /*Initialize LittlevGL*/
    lv_init();

    /*Initialize the HAL (display, input devices, tick) for LittlevGL*/
    hal_init();

    hello_world_label = lv_label_create(lv_disp_get_scr_act(NULL), NULL);
    lv_label_set_text(hello_world_label, "Hello world!");
    lv_obj_align(hello_world_label, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

    count_label = lv_label_create(lv_disp_get_scr_act(NULL), NULL);
    lv_obj_align(count_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
    btn1 = lv_btn_create(lv_disp_get_scr_act(NULL), NULL); /*Create a button on the currently loaded screen*/
    lv_obj_set_event_cb(btn1, btn_event_cb); /*Set function to be called when the button is released*/
    lv_obj_align(btn1, NULL, LV_ALIGN_CENTER, 0, 20); /*Align below the label*/

    /*Create a label on the button*/
    lv_obj_t * btn_label = lv_label_create(btn1, NULL);
    lv_label_set_text(btn_label, "Click ++");

    label_count1 = lv_label_create(lv_disp_get_scr_act(NULL), NULL);
    lv_label_set_text(label_count1, "0");
    lv_obj_align(label_count1, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    while(1) {
        /* Periodically call the lv_task handler.
         * It could be done in a timer interrupt or an OS task too.*/
        if ((count % 100) == 0) {
            sprintf(count_str, "%d", count/ 100);
            lv_label_set_text(count_label, count_str);
        }
        lv_task_handler();
        ++count;
        usleep(10 * 1000); /*Just to let the system breath*/
    }
    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the Littlev graphics library
 */
static void hal_init(void)
{
    /* Use the 'monitor' driver which creates window on PC's monitor to simulate a display*/
    monitor_init();

    /*Create a display buffer*/
    static lv_disp_buf_t disp_buf1;
    static lv_color_t buf1_1[320*10];
    lv_disp_buf_init(&disp_buf1, buf1_1, NULL, 320*10);

    /*Create a display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
    disp_drv.buffer = &disp_buf1;
    disp_drv.flush_cb = monitor_flush;    /*Used when `LV_VDB_SIZE != 0` in lv_conf.h (buffered drawing)*/
    //    disp_drv.hor_res = 200;
    //    disp_drv.ver_res = 100;
    lv_disp_drv_register(&disp_drv);

    /* Add the mouse as input device
     * Use the 'mouse' driver which reads the PC's mouse*/
    mouse_init();
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);          /*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = mouse_read;         /*This function will be called periodically (by the library) to get the mouse position and state*/
    lv_indev_drv_register(&indev_drv);
}

static void btn_event_cb(lv_obj_t * btn, lv_event_t event)
{
    if(event == LV_EVENT_RELEASED) {
        label_count1_value++;
        sprintf(label_count1_str, "%d", label_count1_value);
        lv_label_set_text(label_count1, label_count1_str);
    }
}
