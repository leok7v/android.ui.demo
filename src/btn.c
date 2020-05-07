#include "ui.h"
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
#include "app.h"

begin_c

static void btn_draw(ui_t* u) {
    assertion(false, "btn is abstract should not be used directly");
}

static bool btn_mouse(ui_t* u, int mouse_action, float x, float y) {
    btn_t* b = (btn_t*)ui;
    app_t* a = ui->a;
    bool consumed = false;
    if (mouse_action & MOUSE_LBUTTON_DOWN) {
        b->bitset |= BUTTON_STATE_PRESSED;
        a->invalidate(a);
        consumed = true;
    } else if (mouse_action & MOUSE_LBUTTON_UP) {
        // TODO: (Leo) if we need 3 (or more) state flip this is the place to do it. b->flip = (b->flip + 1) % b->checkbox_wrap_around;
        if (b->flip  != null) { *b->flip = !*b->flip; }
        if (b->click != null) { b->click(ui); }
        a->vibrate(a, EFFECT_CLICK);
        b->bitset &= ~BUTTON_STATE_PRESSED;
        a->invalidate(a);
        consumed = true;
    }
    return consumed;
}

static void btn_screen_mouse(ui_t* u, int mouse_action, float x, float y) {
    btn_t* b = (btn_t*)ui;
    pointf_t pt = ui->screen_xy(ui);
    bool inside = pt.x <= x && x < pt.x + ui->w && pt.y <= y && y < pt.y + ui->h;
    if (!inside && (b->bitset & (BUTTON_STATE_PRESSED|BUTTON_STATE_ARMED) != 0)) {
        b->bitset &= ~(BUTTON_STATE_PRESSED|BUTTON_STATE_ARMED); // disarm button
        ui->a->invalidate(ui->a);
    }
}

void btn_init(btn_t* b, ui_t* parent, void* that, int key_flags, int key,
              const char* mnemonic, const char* label,
              float x, float y, float w, float h) {
    b->ui = *parent;
    b->ui.that = that;
    b->ui.parent = null;
    b->ui.children = null;
    b->ui.next = null;
    b->ui.focusable = true; // buttons are focusable by default
    b->ui.hidden = false;
    b->click = null;
    b->key = key;
    b->key_flags = key_flags;
    b->mnemonic = mnemonic;
    b->label = label;
    b->bitset = 0;
    assert((void*)b == (void*)&b->ui);
    parent->add(parent, &b->ui, x, y, w, h);
    assert(b->ui.parent == parent);
    b->ui.kind = UI_KIND_BUTTON;
    b->ui.draw = btn_draw;
    b->ui.mouse = btn_mouse;
    b->ui.screen_mouse = btn_screen_mouse;
}

void btn_done(btn_t* b) {
    if (b != null) {
        b->ui.parent->remove(b->ui.parent, &b->ui);
        memset(b, 0, sizeof(*b));
    }
}

end_c
