#pragma once
/* Copyright 2020 "Leo" Dmitry Kuznetsov https://leok7v.github.io/
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */
#include "btn.h"

begin_c

typedef struct button_s button_t;

typedef struct button_s {
    btn_t btn;
} button_t;

void button_init(button_t* b, ui_t* parent, void* that, int key_flags, int key,
                 const char* mnemonic, const char* label,
                 float x, float y, float w, float h);

void button_done(button_t* b);

end_c
