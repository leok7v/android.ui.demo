#pragma once
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
    ui_expo_t* expo;
    void (*click)(button_t* self);
    int key_flags; /* keyboard shortcut flags zero or KEYBOARD_ALT|KEYBOARD_CTRL|KEYBOARD_SHIFT */
    int key;       /* keyboard shortcut */
    const char* mnemonic; /* e.g. "Alt+F5" can be null */
    const char* label;
    int bitset; // button state
    bool* flip; /* checkbox button *flip = !*flip; on each click */
} button_t;

button_t* button_create(ui_t* parent, void* that, ui_expo_t* expo,
                        int key_flags, int key, const char* mnemonic,
                        const char* label,
                        float x, float y, float w, float h);

void button_dispose(button_t* b);

END_C
