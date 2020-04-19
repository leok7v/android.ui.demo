#pragma once
/* Copyright 2020 "Leo" Dmitry Kuznetsov
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software distributed
   under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied. See the License for the specific
   language governing permissions and limitations under the License.
*/

#include "glh.h"
#include "ui.h"
#include "font.h"
#include "linmath.h"

BEGIN_C

enum {
    APP_TRACE_MOUSE         = (1 << 0),
    APP_TRACE_KEYBOARD      = (2 << 0),
    APP_TRACE_ALL           = 0xFFFFFFFF
};

enum { // vibration_effect
    DEFAULT_AMPLITUDE = -1,
    EFFECT_CLICK = 0,
    EFFECT_DOUBLE_CLICK = 1,
    EFFECT_TICK = 2,
    EFFECT_HEAVY_TICK = 5
};

typedef struct app_s app_t;

typedef struct app_s {
    void*  that; // pointer to the application specific data
    void*  glue; // pointer to the platform glue data
    ui_t*  root;
    ui_t*  focused;  // ui that has keyboard focus or null
    float xdpi;
    float ydpi;
    int keyboard_flags; // last keyboard flags (CTRL, SHIFT, ALT, SYM, FN, NUMLOCK, CAPSLOCK)
    int mouse_flags;    // last mouse button flags
    int last_mouse_x;   // last mouse screen coordinates
    int last_mouse_y;
    int trace_flags;
    uint64_t time_in_nanoseconds; // since application start update on each event or animation
    // app callbacks (modeled after Android activity lifecycle):
    void (*init)(app_t* a);    // called on application/activity start up
    void (*shown)(app_t* a);   // called on when window has been shown (attached)
    void (*idle)(app_t* a);    // called once when there is no input events
    void (*hidden)(app_t* a);  // called when application is hidden (loses window attachment)
    void (*pause)(app_t* a);   // e.g. when "adb shell input keyevent KEYCODE_SLEEP|KEYCODE_POWER"
    void (*stop)(app_t* a);    // usually after pause()
    void (*resume)(app_t* a);  // e.g. when "adb shell input keyevent KEYCODE_WAKE"
    void (*destroy)(app_t* a); // after hidden() and stop()
    // actions that application code can call:
    void (*quit)(app_t* app);           // quit application/activity (on Android it will now exit process)
    void (*exit)(app_t* app, int code); // trying to exit application gracefully with specified return code
    void (*invalidate)(app_t* app);     // make application redraw once
    void (*animate)(app_t* app, int animating); // animating !=0 make application continuosly redraw
    // ui:
    void (*focus)(app_t* app, ui_t* ui); // set application keyboard focus on particular ui element or null
    // timers:
    int  (*timer_add)(app_t* a, timer_callback_t* tcb); // returns timer id > 0 or 0 if fails (too many timers)
    void (*timer_remove)(app_t* a, timer_callback_t* tcb);
    // assets/resources:
    void* (*asset_map)(app_t* a, const char* name, const void* *data, int *bytes);
    void  (*asset_unmap)(app_t* a, void* asset, const void* data, int bytes);
    void  (*vibrate)(app_t* a, int vibration_effect);
    void  (*show_keyboard)(app_t* a, bool on); // shows/hides soft keyboard
    mat4x4 projection;
    mat4x4 view;
} app_t;

// app_create() MUST be implemented by application. It is called before main()
// application must load main ui font (e.g. from resources) and assign to app_t.font field
// application is expected to fill some of the callbacks that will be called later.

void app_create(app_t* app);
void app_trace_key(int flags, int ch);
void app_trace_mouse(int mouse_flags, int mouse_action, int index, float x, float y);

enum { // logging level
    LOG_DEFAULT = 1,
    LOG_VERBOSE = 2,
    LOG_DEBUG   = 3,
    LOG_INFO    = 4,
    LOG_WARN    = 5,
    LOG_ERROR   = 6,
    LOG_FATAL   = 7,
    LOG_SILENT  = 8
};

extern int (*app_log)(int level, const char* tag, const char* location, const char* format, va_list vl); // may be null

END_C
