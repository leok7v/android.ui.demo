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
#include "c.h"
#include <stdint.h>
#include <stdbool.h>
#include <android/native_activity.h>

begin_c // interfacce to unavoidable Android Java code via jni

typedef struct droid_display_metrics_s {
     int w; // pixels
     int h; // pixels
     int dpi;
     float xdpi;
     float ydpi;
     float density;
     float scaled_density;
} droid_display_metrics_t;

void droid_jni_get_display_real_size(ANativeActivity* na, droid_display_metrics_t* m);

bool droid_jni_hide_navigation_bar(ANativeActivity* na);

bool droid_jni_vibrate_milliseconds(ANativeActivity* na, int milliseconds);

// "DEFAULT_AMPLITUDE", "EFFECT_CLICK", "EFFECT_DOUBLE_CLICK" "EFFECT_TICK", "EFFECT_HEAVY_TICK"
bool droid_jni_vibrate_with_effect(ANativeActivity* na, const char* effect);

bool droid_jni_show_keyboard(ANativeActivity* na, bool on, int flags);

end_c
