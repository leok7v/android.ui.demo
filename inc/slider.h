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

begin_c

typedef struct slider_s slider_t;

typedef struct slider_s {
    ui_t u; // if u.focusable - slider has [+]/[-] buttons, otherwise it is buttonless progress indicator
    void (*notify)(slider_t* u); // on [+]/[-] click, Ctrl+Click (+/- 10), Shift+Click  (+/- 100), Ctrl+Shift+Click  (+/- 1000)
    int* minimum; // may be changed by caller on the fly, thus pointer
    int* maximum;
    int* current;
    const char*  label;           // can be null
    // internal state:
    int timer_id;
    timer_callback_t timer_callback;
} slider_t;

void slider_init(slider_t* s, ui_t* parent, void* that,
                 const char* label, float x, float y, float w, float h,
                 int* minimum, int* maximum, int* current);

void slider_done(slider_t* s);

end_c
