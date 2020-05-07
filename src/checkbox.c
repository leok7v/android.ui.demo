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
#include "checkbox.h"
#include "app.h"

begin_c

static float checkbox_draw(ui_t* u, pointf_t pt, bool on) {
    btn_t* b = &((checkbox_t*)ui)->btn;
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

static void button_draw(ui_t* u) {
    // modern touch UI checkbox usually does NOT have label or keyboard shortcut
    // but `old` style checkbox may still be drivven from keyboard and be a focus
    btn_t* b = &((checkbox_t*)ui)->btn;
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

void checkbox_init(checkbox_t* cbx, ui_t* parent, void* that,
                 int key_flags, int key, const char* mnemonic,
                 const char* label,
                 float x, float y, float w, float h) {
    btn_init(&cbx->btn, parent, that, key_flags, key, mnemonic, label, x, y, w, h);
    cbx->btn.ui.draw = button_draw;
}

void checkbox_done(checkbox_t* cbx) {
    btn_done(&cbx->btn);
}

end_c
