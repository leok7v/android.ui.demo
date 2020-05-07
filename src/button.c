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

begin_c

static void button_draw(ui_t* u) {
    btn_t* b = &((button_t*)u)->btn;
    theme_t* theme = &u->a->theme;
    const colorf_t* color = b->bitset & BUTTON_STATE_PRESSED ? theme->color_background_pressed : theme->color_background;
    pointf_t pt = u->screen_xy(u);
    dc.fill(&dc, color, pt.x, pt.y, u->w, u->h);
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
    pt.y += (int)(baseline + (u->h - fh) / 2);
    pt.x += em;
    assertion(b->flip == null, "use checkbox_t instead of button_t flip buttons");
    dc.text(&dc, theme->color_text, f, pt.x, pt.y, b->label, (int)strlen(b->label));
    if (m >= 0) { // draw highlighted mnemonic
        copy[m] = 0;
        float mx = pt.x + font_text_width(f, copy, m);
        dc.text(&dc, theme->color_mnemonic, f, mx, pt.y, mn, (int)strlen(mn));
    }
}

void button_init(button_t* b, ui_t* parent, void* that, int key_flags, int key,
                 const char* mnemonic, const char* label,
                 float x, float y, float w, float h) {
    btn_init(&b->btn, parent, that, key_flags, key, mnemonic, label, x, y, w, h);
    b->btn.u.draw = button_draw;
}

void button_done(button_t* b) {
    btn_done(&b->btn);
}

end_c
