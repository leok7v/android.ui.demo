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
#include "button.h"
#include "app.h"

BEGIN_C

static float checkbox_draw(ui_t* ui, pointf_t pt, bool on) {
    button_t* b = (button_t*)ui;
    font_t* f = b->ui.a->theme.font;
    const float em = f->em;
    const float R = em * 3 / 4;
    const float r = em / 2;
    const colorf_t* light = on ? colors_dk.light_blue : colors_dk.light_gray;
    const colorf_t* dark  = on ? colors_dk.dark_blue  : colors_dk.dark_gray;
    dc.fill(&dc, dark, pt.x, pt.y - em, em, em);
    float y = pt.y - em + r;
    if (on) {
        dc.ring(&dc, dark, pt.x, y, r, 0);
        dc.ring(&dc, light, pt.x + em, y, R, 0);
    } else {
        dc.ring(&dc, dark, pt.x + em, y, r, 0);
        dc.ring(&dc, light, pt.x, y, R, 0);
    }
    return pt.x + 2 * em;
}

static void button_draw(ui_t* ui) {
    button_t* b = (button_t*)ui;
    theme_t* theme = &ui->a->theme;
    const colorf_t* color = b->bitset & BUTTON_STATE_PRESSED ? theme->color_background_pressed : theme->color_background;
    pointf_t pt = ui->screen_xy(ui);
    dc.fill(&dc, color, pt.x, pt.y, ui->w, ui->h);
    int k = (int)strlen(b->label) + 1;
    const char* mn = b->mnemonic;
    char letter[2] = {};
    char copy[k];
    strncpy(copy, b->label, k);
    int m = -1;
    if (b->mnemonic != null) {
        const char* p = strstr(copy, b->mnemonic);
        if (p != null) { m = (int)(uintptr_t)(p - copy); }
    } else if (m < 0 && isalpha(b->key)) {
        int lck = tolower(b->key);
        for (int i = 0; i < k - 1 && m < 0; i++) {
            if (lck == tolower(copy[i])) { letter[0] = copy[i]; mn = letter; m = i; }
        }
    }
    font_t* f = theme->font;
    float fh = f->height;
    float em = f->em;
    float baseline = f->baseline;
    pt.y += (int)(baseline + (ui->h - fh) / 2);
    pt.x += em;
    if (b->flip != null) { pt.x = checkbox_draw(ui, pt, (*b->flip != 0) ^ (b->inverse != 0)); }
    dc.text(&dc, theme->color_text, f, pt.x, pt.y, b->label, (int)strlen(b->label));
    if (m >= 0) { // draw highlighted mnemonic
        copy[m] = 0;
        float mx = pt.x + font_text_width(f, copy, m);
        dc.text(&dc, theme->color_mnemonic, f, mx, pt.y, mn, (int)strlen(mn));
    }
}

static void button_mouse(ui_t* ui, int mouse_action, float x, float y) {
    button_t* b = (button_t*)ui;
    app_t* a = ui->a;
    if (mouse_action & MOUSE_LBUTTON_DOWN) {
        b->bitset |= BUTTON_STATE_PRESSED;
        a->invalidate(a);
    }
    if (mouse_action & MOUSE_LBUTTON_UP) {
        // TODO: (Leo) if we need 3 (or more) state flip this is the place to do it. b->flip = (b->flip + 1) % b->checkbox_wrap_around;
        if (b->flip != null) { *b->flip = !*b->flip; }
        if (b->click != null) { b->click(b); }
        a->vibrate(a, EFFECT_CLICK);
        b->bitset &= ~BUTTON_STATE_PRESSED;
        a->invalidate(a);
    }
}

static void button_screen_mouse(ui_t* ui, int mouse_action, float x, float y) {
    button_t* b = (button_t*)ui;
    pointf_t pt = ui->screen_xy(ui);
    bool inside = pt.x <= x && x < pt.x + ui->w && pt.y <= y && y < pt.y + ui->h;
    if (!inside && (b->bitset & (BUTTON_STATE_PRESSED|BUTTON_STATE_ARMED) != 0)) {
        b->bitset &= ~(BUTTON_STATE_PRESSED|BUTTON_STATE_ARMED); // disarm button
        ui->a->invalidate(ui->a);
    }
}

button_t* button_create(ui_t* parent, void* that,
                        int key_flags, int key, const char* mnemonic,
                        const char* label,
                        float x, float y, float w, float h) {
    button_t* b = (button_t*)allocate(sizeof(button_t));
    if (b != null) {
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
        b->ui.draw = button_draw;
        b->ui.mouse = button_mouse;
        b->ui.screen_mouse = button_screen_mouse;
    }
    return b;
}

void button_dispose(button_t* b) {
    if (b != null) {
        b->ui.parent->remove(b->ui.parent, &b->ui);
        deallocate(b);
    }
}

END_C
