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
#include "btn.h"

begin_c

typedef struct checkbox_s checkbox_t;

typedef struct checkbox_s {
    btn_t btn;
} checkbox_t;

void checkbox_init(checkbox_t* b, ui_t* parent, void* that,
                 int key_flags, int key, const char* mnemonic,
                 const char* label,
                 float x, float y, float w, float h);

void checkbox_done(checkbox_t* b);

end_c
