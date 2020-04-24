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
#include "ui.h"
#include "app.h"

BEGIN_C

typedef struct toast_s toast_t;

typedef struct toast_s {
    ui_t ui;
    uint64_t nanoseconds; // time to keep toast on screen
    void (*print)(toast_t* t, const char* format, ...);
    void (*cancel)(toast_t* t);
    // implementation:
    char text[1024];
    timer_callback_t toast_timer_callback;
    uint64_t toast_start_time; // time toast started to be shown
} toast_t;

toast_t* toast(app_t* a); // returns pointer to toast single instance

END_C
