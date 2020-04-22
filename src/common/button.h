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

BEGIN_C

typedef struct button_s button_t;

enum {
    BUTTON_STATE_PRESSED = 0x1,
    BUTTON_STATE_FOCUSED = 0x2,
    BUTTON_STATE_ARMED   = 0x4
};

typedef struct button_s {
    ui_t ui;
    ui_theme_t* theme;
    void (*click)(button_t* self);
    int key_flags; /* keyboard shortcut flags zero or KEYBOARD_ALT|KEYBOARD_CTRL|KEYBOARD_SHIFT */
    int key;       /* keyboard shortcut */
    const char* mnemonic; /* e.g. "Alt+F5" can be null */
    const char* label;
    int bitset; // button state
    bool* flip;  /* checkbox button *flip = !*flip; on each click */
    bool negate; /* negate *flip value drawing UI */
} button_t;

button_t* button_create(ui_t* parent, void* that, ui_theme_t* theme,
                        int key_flags, int key, const char* mnemonic,
                        const char* label,
                        float x, float y, float w, float h);

void button_dispose(button_t* b);

END_C
