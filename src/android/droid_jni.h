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
#include "rt.h"
#include <android/native_activity.h>

BEGIN_C // interfacce to unavoidable Android Java code via jni

bool droid_jni_hide_navigation_bar(ANativeActivity* na);

bool droid_jni_vibrate_milliseconds(ANativeActivity* na, int milliseconds);

// "DEFAULT_AMPLITUDE", "EFFECT_CLICK", "EFFECT_DOUBLE_CLICK" "EFFECT_TICK", "EFFECT_HEAVY_TICK"
bool droid_jni_vibrate_with_effect(ANativeActivity* na, const char* effect);

bool droid_jni_show_keyboard(ANativeActivity* na, bool on, int flags);

uint64_t droid_jni_get_unicode_char(ANativeActivity* na, AInputEvent* input_event);

END_C
