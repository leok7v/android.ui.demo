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

begin_c

typedef struct font_s font_t;
typedef struct colorf_s colorf_t;

typedef struct theme_s { // UI theme attributes
    font_t* font;
    float ui_height; // 1.75 means 175% of font height in pixels for button, labels and other UI elements
    const colorf_t* color_text;
    const colorf_t* color_background;
    const colorf_t* color_mnemonic; // color for mnemonic charachter highlight
    const colorf_t* color_focused;
    const colorf_t* color_background_focused;
    const colorf_t* color_armed;
    const colorf_t* color_background_armed;
    const colorf_t* color_pressed;
    const colorf_t* color_background_pressed;
    const colorf_t* color_slider;
    const colorf_t* color_toast_text;
    const colorf_t* color_toast_background;
} theme_t;

end_c
