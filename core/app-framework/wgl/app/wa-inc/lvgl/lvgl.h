/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef WAMR_GRAPHIC_LIBRARY_LVGL_COMPATIBLE_H
#define WAMR_GRAPHIC_LIBRARY_LVGL_COMPATIBLE_H

#ifdef __cplusplus
extern "C" {
#endif

//#include "bi-inc/wgl_shared_utils.h" /* shared types between app and native */
/*
#include "lvgl-compatible/lv_types.h"
#include "lvgl-compatible/lv_obj.h"
#include "lvgl-compatible/lv_btn.h"
#include "lvgl-compatible/lv_cb.h"
#include "lvgl-compatible/lv_label.h"
#include "lvgl-compatible/lv_list.h"
*/

#include "src/lv_version.h"

#include "src/lv_misc/lv_log.h"
#include "src/lv_misc/lv_task.h"
#include "src/lv_misc/lv_math.h"
//#include "src/lv_misc/lv_async.h"

//#include "src/lv_hal/lv_hal.h"

#include "src/lv_core/lv_obj.h"
#include "src/lv_core/lv_group.h"

#include "src/lv_core/lv_refr.h"
#include "src/lv_core/lv_disp.h"

#include "src/lv_themes/lv_theme.h"

#include "src/lv_font/lv_font.h"
#include "src/lv_font/lv_font_fmt_txt.h"

#include "src/lv_objx/lv_btn.h"
#include "src/lv_objx/lv_imgbtn.h"
#include "src/lv_objx/lv_img.h"
#include "src/lv_objx/lv_label.h"
#include "src/lv_objx/lv_line.h"
#include "src/lv_objx/lv_page.h"
#include "src/lv_objx/lv_cont.h"
#include "src/lv_objx/lv_list.h"
#include "src/lv_objx/lv_chart.h"
#include "src/lv_objx/lv_table.h"
#include "src/lv_objx/lv_cb.h"
#include "src/lv_objx/lv_bar.h"
#include "src/lv_objx/lv_slider.h"
#include "src/lv_objx/lv_led.h"
#include "src/lv_objx/lv_btnm.h"
#include "src/lv_objx/lv_kb.h"
#include "src/lv_objx/lv_ddlist.h"
#include "src/lv_objx/lv_roller.h"
#include "src/lv_objx/lv_ta.h"
#include "src/lv_objx/lv_canvas.h"
#include "src/lv_objx/lv_win.h"
#include "src/lv_objx/lv_tabview.h"
#include "src/lv_objx/lv_tileview.h"
#include "src/lv_objx/lv_mbox.h"
#include "src/lv_objx/lv_gauge.h"
#include "src/lv_objx/lv_lmeter.h"
#include "src/lv_objx/lv_sw.h"
#include "src/lv_objx/lv_kb.h"
#include "src/lv_objx/lv_arc.h"
#include "src/lv_objx/lv_preload.h"
#include "src/lv_objx/lv_calendar.h"
#include "src/lv_objx/lv_spinbox.h"

#include "src/lv_draw/lv_img_cache.h"

#ifdef __cplusplus
}
#endif

#endif /* WAMR_GRAPHIC_LIBRARY_LVGL_COMPATIBLE_H */
