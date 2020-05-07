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

// "Abstract" button. draw() is not impelemented. Use as base for variety of buttons.

typedef struct btn_s btn_t;

enum {
    BUTTON_STATE_PRESSED = 0x1,
    BUTTON_STATE_FOCUSED = 0x2,
    BUTTON_STATE_ARMED   = 0x4
};

typedef struct btn_s {
    ui_t u;
    void (*click)(ui_t* u);
    int key_flags; /* keyboard shortcut flags zero or KEYBOARD_ALT|KEYBOARD_CTRL|KEYBOARD_SHIFT */
    int key;       /* keyboard shortcut */
    const char* mnemonic; /* e.g. "Alt+F5" can be null */
    const char* label;
    int bitset;    /* button state */
    bool* flip;    /* checkbox button *flip = !*flip; on each click */
    bool inverse;  /* inverse *flip value drawing UI */
} btn_t;

void btn_init(btn_t* b, ui_t* parent, void* that, int key_flags, int key,
              const char* mnemonic, const char* label,
              float x, float y, float w, float h);

void btn_done(btn_t* b);

end_c
