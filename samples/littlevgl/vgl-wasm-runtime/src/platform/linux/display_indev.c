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

#include <stdio.h>
#include <stdbool.h>
#include "display_indev.h"
#include "SDL2/SDL.h"
#include "sys/time.h"
#include "wasm_export.h"

#define MONITOR_HOR_RES 320
#define MONITOR_VER_RES 240
#ifndef MONITOR_ZOOM
#define MONITOR_ZOOM        1
#endif
#define SDL_REFR_PERIOD     50
void monitor_sdl_init(void);
void monitor_sdl_refr_core(void);
void monitor_sdl_clean_up(void);

static uint32_t tft_fb[MONITOR_HOR_RES * MONITOR_VER_RES];



int time_get_ms()
{
    static struct timeval tv;
    gettimeofday(&tv, NULL);
    long long time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;

    return (int) time_in_mill;
}

SDL_Window * window;
SDL_Renderer * renderer;
SDL_Texture * texture;
static volatile bool sdl_inited = false;
static volatile bool sdl_refr_qry = false;
static volatile bool sdl_quit_qry = false;

void monitor_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
        const lv_color_t * color_p)
{
    /*Return if the area is out the screen*/
    if (x2 < 0 || y2 < 0 || x1 > MONITOR_HOR_RES - 1
            || y1 > MONITOR_VER_RES - 1) {
        return;
    }

    int32_t y;
    uint32_t w = x2 - x1 + 1;
    for (y = y1; y <= y2; y++) {
        memcpy(&tft_fb[y * MONITOR_HOR_RES + x1], color_p,
                w * sizeof(lv_color_t));

        color_p += w;
    }
    sdl_refr_qry = true;

    /*IMPORTANT! It must be called to tell the system the flush is ready*/

}

/**
 * Fill out the marked area with a color
 * @param x1 left coordinate
 * @param y1 top coordinate
 * @param x2 right coordinate
 * @param y2 bottom coordinate
 * @param color fill color
 */
void monitor_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
        lv_color_t color)
{
    /*Return if the area is out the screen*/
    if (x2 < 0)
        return;
    if (y2 < 0)
        return;
    if (x1 > MONITOR_HOR_RES - 1)
        return;
    if (y1 > MONITOR_VER_RES - 1)
        return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > MONITOR_HOR_RES - 1 ? MONITOR_HOR_RES - 1 : x2;
    int32_t act_y2 = y2 > MONITOR_VER_RES - 1 ? MONITOR_VER_RES - 1 : y2;

    int32_t x;
    int32_t y;
    uint32_t color32 = color.full; //lv_color_to32(color);

    for (x = act_x1; x <= act_x2; x++) {
        for (y = act_y1; y <= act_y2; y++) {
            tft_fb[y * MONITOR_HOR_RES + x] = color32;
        }
    }

    sdl_refr_qry = true;
}

/**
 * Put a color map to the marked area
 * @param x1 left coordinate
 * @param y1 top coordinate
 * @param x2 right coordinate
 * @param y2 bottom coordinate
 * @param color_p an array of colors
 */
void monitor_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
        const lv_color_t * color_p)
{
    /*Return if the area is out the screen*/
    if (x2 < 0)
        return;
    if (y2 < 0)
        return;
    if (x1 > MONITOR_HOR_RES - 1)
        return;
    if (y1 > MONITOR_VER_RES - 1)
        return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > MONITOR_HOR_RES - 1 ? MONITOR_HOR_RES - 1 : x2;
    int32_t act_y2 = y2 > MONITOR_VER_RES - 1 ? MONITOR_VER_RES - 1 : y2;

    int32_t x;
    int32_t y;

    for (y = act_y1; y <= act_y2; y++) {
        for (x = act_x1; x <= act_x2; x++) {
            tft_fb[y * MONITOR_HOR_RES + x] = color_p->full; //lv_color_to32(*color_p);
            color_p++;
        }

        color_p += x2 - act_x2;
    }

    sdl_refr_qry = true;
}


void display_init(void)
{
}

void display_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
        int32 color_p_offset)
{

    wasm_module_inst_t module_inst = wasm_runtime_get_current_module_inst();
    if (!wasm_runtime_validate_app_addr(module_inst, color_p_offset, 1))
        return;
    lv_color_t * color_p = wasm_runtime_addr_app_to_native(module_inst,
            color_p_offset);

    monitor_flush(x1, y1, x2, y2, color_p);
}
void display_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
        lv_color_t color_p)
{
    monitor_fill(x1, y1, x2, y2, color_p);
}
void display_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
        const lv_color_t * color_p)
{
    monitor_map(x1, y1, x2, y2, color_p);
}

bool display_input_read(int32 data_p_offset)
{
    bool ret;
    wasm_module_inst_t module_inst = wasm_runtime_get_current_module_inst();
    if (!wasm_runtime_validate_app_addr(module_inst, data_p_offset, 1))
        return false;

    struct {
        lv_point_t point;
        int32 user_data_offset;
        uint8 state;
    } *data_app;

    lv_indev_data_t data;

    ret = mouse_read(&data);

    data_app = wasm_runtime_addr_app_to_native(module_inst,
                                               data_p_offset);

    data_app->point = data.point;
    data_app->user_data_offset = (int32_t)data.user_data;
    data_app->state = data.state;

    return ret;
}

void display_deinit(void)
{

}

void display_vdb_write(int32 buf_offset, lv_coord_t buf_w, lv_coord_t x,
        lv_coord_t y, int32 color_p_offset, lv_opa_t opa)
{
    wasm_module_inst_t module_inst = wasm_runtime_get_current_module_inst();
    if (!wasm_runtime_validate_app_addr(module_inst, color_p_offset, 1))
        return;
    lv_color_t *color = wasm_runtime_addr_app_to_native(module_inst,
            color_p_offset);

    void *buf = wasm_runtime_addr_app_to_native(module_inst, buf_offset);

    unsigned char *buf_xy = buf + 4 * x + 4 * y * buf_w;
    lv_color_t * temp = (lv_color_t *) buf_xy;
    *temp = *color;
    /*
     if (opa != LV_OPA_COVER) {
     lv_color_t mix_color;

     mix_color.red = *buf_xy;
     mix_color.green = *(buf_xy+1);
     mix_color.blue = *(buf_xy+2);
     color = lv_color_mix(color, mix_color, opa);
     }
     */
    /*
     *buf_xy = color->red;
     *(buf_xy + 1) = color->green;
     *(buf_xy + 2) = color->blue;
     */
}

int monitor_sdl_refr_thread(void * param)
{
    (void) param;

    /*If not OSX initialize SDL in the Thread*/
    monitor_sdl_init();
    /*Run until quit event not arrives*/
    while (sdl_quit_qry == false) {
        /*Refresh handling*/
        monitor_sdl_refr_core();
    }

    monitor_sdl_clean_up();
    exit(0);

    return 0;
}
extern void mouse_handler(SDL_Event *event);
void monitor_sdl_refr_core(void)
{
    if (sdl_refr_qry != false) {
        sdl_refr_qry = false;

        SDL_UpdateTexture(texture, NULL, tft_fb,
        MONITOR_HOR_RES * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        /*Update the renderer with the texture containing the rendered image*/
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {

        mouse_handler(&event);

        if ((&event)->type == SDL_WINDOWEVENT) {
            switch ((&event)->window.event) {
#if SDL_VERSION_ATLEAST(2, 0, 5)
            case SDL_WINDOWEVENT_TAKE_FOCUS:
#endif
            case SDL_WINDOWEVENT_EXPOSED:

                SDL_UpdateTexture(texture, NULL, tft_fb,
                MONITOR_HOR_RES * sizeof(uint32_t));
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
                break;
            default:
                break;
            }
        }
    }

    /*Sleep some time*/
    SDL_Delay(SDL_REFR_PERIOD);

}
int quit_filter(void * userdata, SDL_Event * event)
{
    (void) userdata;

    if (event->type == SDL_QUIT) {
        sdl_quit_qry = true;
    }

    return 1;
}

void monitor_sdl_clean_up(void)
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void monitor_sdl_init(void)
{
    /*Initialize the SDL*/
    SDL_Init(SDL_INIT_VIDEO);

    SDL_SetEventFilter(quit_filter, NULL);

    window = SDL_CreateWindow("TFT Simulator", SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            MONITOR_HOR_RES * MONITOR_ZOOM, MONITOR_VER_RES * MONITOR_ZOOM, 0); /*last param. SDL_WINDOW_BORDERLESS to hide borders*/

    renderer = SDL_CreateRenderer(window, -1, 0);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STATIC, MONITOR_HOR_RES, MONITOR_VER_RES);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    /*Initialize the frame buffer to gray (77 is an empirical value) */
    memset(tft_fb, 0x44, MONITOR_HOR_RES * MONITOR_VER_RES * sizeof(uint32_t));
    SDL_UpdateTexture(texture, NULL, tft_fb,
    MONITOR_HOR_RES * sizeof(uint32_t));
    sdl_refr_qry = true;
    sdl_inited = true;
}

void display_SDL_init()
{
    SDL_CreateThread(monitor_sdl_refr_thread, "sdl_refr", NULL);
    while (sdl_inited == false)
        ; /*Wait until 'sdl_refr' initializes the SDL*/
}

