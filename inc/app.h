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

#include "theme.h"
#include "ui.h"

begin_c

enum { // vibration_effect
    DEFAULT_AMPLITUDE = -1,
    EFFECT_CLICK = 0,
    EFFECT_DOUBLE_CLICK = 1,
    EFFECT_TICK = 2,
    EFFECT_HEAVY_TICK = 5
};

typedef struct app_s app_t;

typedef struct app_s {
    // app callbacks (modeled after Android activity lifecycle):
    void (*init)(app_t* a);    // called on start of application/activity
    void (*shown)(app_t* a, int w, int h); // window (w x h) has been shown (attached)
    void (*draw)(app_t* a);    // called to paint content of the application
    void (*resized)(app_t* a, int x, int y, int w, int h); // viewport has been resized or rotated; needs new projection matrix
    void (*hidden)(app_t* a);  // called when application is hidden (loses window attachment)
    void (*pause)(app_t* a);   // e.g. when "adb shell input keyevent KEYCODE_SLEEP|KEYCODE_POWER"
    void (*stop)(app_t* a);    // after pause() -> stop() or resume()
    void (*resume)(app_t* a);  // e.g. when "adb shell input keyevent KEYCODE_WAKE"
    void (*done)(app_t* a);    // after pause() hidden() and stop()
    void (*key)(app_t* a, int flags, int keycode); // dispatch keyboard
    void (*touch)(app_t* a, int index, int action, int x, int y); // index == finger for multitouch
    // actions that application code can call:
    void (*quit)(app_t* app);           // quit application/activity (on Android it will now exit process)
    void (*exit)(app_t* app, int code); // trying to exit application gracefully with specified return code
    void (*invalidate)(app_t* app);     // make application redraw once
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
    int   (*logln)(int level, const char* tag, const char* location, const char* format, va_list vl); // may be null
    // application state:
    void* that; // pointer to the application specific data
    void* glue; // pointer to the platform glue data
    int   sw;   // screen width pixels
    int   sh;   // screen height
    float xdpi;
    float ydpi;
    ui_t  root;
    ui_t* focused; // ui that has keyboard focus or null
    int keyboard_flags; // last keyboard flags (CTRL, SHIFT, ALT, SYM, FN, NUMLOCK, CAPSLOCK)
    int mouse_flags;    // last mouse button flags (for finger index == 0)
    int last_mouse_x;   // last mouse screen coordinates
    int last_mouse_y;
    uint64_t time_in_nanoseconds; // since application start update on each event or animation
    theme_t theme;
} app_t;

extern app_t* app;

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

end_c
