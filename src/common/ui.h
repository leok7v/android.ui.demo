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
#include "dc.h"

BEGIN_C

typedef struct font_s font_t;

enum {
    // mouse state (flags)
    MOUSE_LBUTTON_FLAG = 0x10000, // present when mouse/finger is down (see finger_index)
    MOUSE_MBUTTON_FLAG = 0x20000,
    MOUSE_RBUTTON_FLAG = 0x40000,

    // mouse actions not to be confused with flags
    MOUSE_MOVE         = 0x0001,
    MOUSE_LBUTTON_DOWN = 0x0002,
    MOUSE_LBUTTON_UP   = 0x0004,

    // keyboad state (flags) not to be confused with KEY_CODE_SHIFT, KEY_CODE_ALT ...
    KEYBOARD_SHIFT       = 0x0001,
    KEYBOARD_ALT         = 0x0002,
    KEYBOARD_CTRL        = 0x0004,
    KEYBOARD_SYM         = 0x0008, // Apple Meta, Windows Key, etc
    KEYBOARD_FN          = 0x0010, // Function key
    KEYBOARD_NUMPAD      = 0x0020,

    KEYBOARD_NUMLOCK    = 0x0040,
    KEYBOARD_CAPSLOCK   = 0x0080,

    KEYBOARD_KEY_PRESSED  = 0x0100,
    KEYBOARD_KEY_RELEASED = 0x0200,
    KEYBOARD_KEY_REPEAT   = 0x0400,

    KEY_CODE_ENTER      = 0x01000000,
    KEY_CODE_LEFT       = 0x02000000,
    KEY_CODE_RIGHT      = 0x03000000,
    KEY_CODE_UP         = 0x04000000,
    KEY_CODE_DOWN       = 0x05000000,
    KEY_CODE_BACKSPACE  = 0x06000000,
    KEY_CODE_PAGE_UP    = 0x07000000,
    KEY_CODE_PAGE_DOWN  = 0x08000000,
    KEY_CODE_HOME       = 0x09000000,
    KEY_CODE_END        = 0x0A000000,
    KEY_CODE_BACK       = 0x0B000000, // like browser BACK
    KEY_CODE_ESC        = 0x0C000000,
    KEY_CODE_DEL        = 0x0D000000,
    KEY_CODE_INS        = 0x0E000000,
    KEY_CODE_CENTER     = 0x0F000000, // numpad center

    KEY_CODE_F1         = 0x0F100000,
    KEY_CODE_F2         = 0x0F200000,
    KEY_CODE_F3         = 0x0F300000,
    KEY_CODE_F4         = 0x0F400000,
    KEY_CODE_F5         = 0x0F500000,
    KEY_CODE_F6         = 0x0F600000,
    KEY_CODE_F7         = 0x0F700000,
    KEY_CODE_F8         = 0x0F800000,
    KEY_CODE_F9         = 0x0F900000,
    KEY_CODE_F10        = 0x0FA00000,
    KEY_CODE_F11        = 0x0FB00000,
    KEY_CODE_F12        = 0x0FC00000,

    KEY_CODE_ALT        = 0x10000000,
    KEY_CODE_CTRL       = 0x20000000,
    KEY_CODE_SHIFT      = 0x30000000,
};

enum {
    UI_KIND_CONTAINER   = 0,
    UI_KIND_BUTTON      = 1,
    UI_KIND_SLIDER      = 2
};

typedef struct app_s app_t;

typedef struct ui_s ui_t;
typedef struct timer_callback_s timer_callback_t;

enum {
    NS_IN_MS = 1000000,
    NS_IN_SEC = NS_IN_MS * 1000
};

typedef struct timer_callback_s {
    void* that;
    void (*callback)(timer_callback_t* timer_callback);
    int id; // timer id is only valid after succesful time
    uint64_t ns; // period in nanoseconds
    uint64_t last_fired; // monotonic ns since app start_time, must be zero before calling set_timer()
} timer_callback_t;

/* theory of operations:
   1 Containers forward calls to children.
   2 Terminal leaves can impelement draw()
   3 Container may implement draw() but need to call draw_children() inside it
   4 screen_mouse() is called for all (even hidden components). Used to "disarm" pressed buttons
   5 keyboard is called on all containers and terminal leaves. Compare yourself to app.focus to accept input
*/

typedef struct ui_s {
    void* that;
    int kind;
    float x, y, w, h;
    bool hidden;
    bool focusable;
    bool decor; // draw this ui element on top of children
    ui_t* parent;
    app_t* a;
    ui_t* next; // next sibling
    ui_t* children; // linked list of children
    pointf_t (*screen_xy)(ui_t* self);
    void (*add)(ui_t* self, ui_t* child, float x, float y, float w, float h);
    void (*remove)(ui_t* self, ui_t* child);
    ui_t* (*create)(ui_t* parent, void* that, float x, float y, float w, float h); // create and dd container
    void (*dispose)(ui_t* self); // remove() ui from parent and dispose it
    void (*draw)(ui_t* self); // calls draw_children
    void (*draw_children)(ui_t* self);
    void (*mouse)(ui_t* self, int mouse_flags, float x, float y); // x,y in ui coordinates
    void (*screen_mouse)(ui_t* self, int mouse_flags, float screen_x, float screen_y); // x,y screen coordinates
    void (*keyboard)(ui_t* self, int flags, int ch);
    void (*focus)(ui_t* self, bool gain);
//  do not override dispatch_* use mouse() and screen_mouse() as listeners:
    void (*dispatch_mouse)(ui_t* self, int mouse_flags, float x, float y); // x,y in ui coordinates
    void (*dispatch_screen_mouse)(ui_t* self, int mouse_flags, float screen_x, float screen_y); // x,y screen coordinates
} ui_t;

typedef struct ui_theme_s { // UI theme attributes
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
} ui_theme_t;

extern ui_t* ui_root;

bool ui_set_focus(ui_t* ui, int x, int y); // returns true if focus was set

END_C
