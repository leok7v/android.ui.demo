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
#include "slider.h"
#include "app.h"

begin_c

static const char* SLIDER_DEC_LABEL = "[-]";
static const char* SLIDER_INC_LABEL = "[+]";

static void slider_notify(slider_t* s) {
    s->notify(s);
    s->u.a->invalidate(s->u.a); // after notify because notify may do something to layout etc...
}

static int slider_scale(slider_t* s) {
    enum { CTRL_SHIFT = KEYBOARD_CTRL|KEYBOARD_SHIFT };
    return (s->u.a->keyboard_flags & CTRL_SHIFT) == CTRL_SHIFT     ? 1000 :
           (s->u.a->keyboard_flags & CTRL_SHIFT) == KEYBOARD_SHIFT ?  100 :
           (s->u.a->keyboard_flags & CTRL_SHIFT) == KEYBOARD_CTRL  ?   10 : 1;
}

static int slider_dec_inc(slider_t* s, float x, float y) {
    theme_t* theme = &s->u.a->theme;
    const float em4 = theme->font->em / 4;
    const float dec_width = font_text_width(theme->font, SLIDER_DEC_LABEL, -1) + em4;
    const float inc_width = font_text_width(theme->font, SLIDER_INC_LABEL, -1) + em4;
    if (0 <= x && x <= dec_width) {
        if (s->minimum != null && s->maximum != null && s->current != null && *s->minimum < *s->current) {
            return -1;
        }
    } else if (s->u.w - inc_width <= x && x <= s->u.w) {
        if (s->minimum != null && s->maximum != null && s->current != null && *s->current < *s->maximum) {
            return +1;
        }
    }
    return 0;
}

static bool slider_click_inc_dec(slider_t* s, int direction, int d) {
    bool changed = false;
    if (direction < 0) {
        int v = *s->current - d;
        changed = *s->current != v;
        *s->current = v < *s->minimum ? *s->minimum : v;
    } else if (direction > 0) {
        int v = *s->current + d;
        changed = *s->current != v;
        *s->current = *s->maximum < v ? *s->maximum : v;
    }
    return changed;
}

static void slider_autorepeat(timer_callback_t* tcb) {
    slider_t* s = (slider_t*)tcb->that;
    if ((s->u.a->touch_flags & MOUSE_LBUTTON_FLAG) == 0 && tcb->id > 0) {
        s->u.a->timer_remove(s->u.a, tcb);
    } else {
        const int x = s->u.a->last_touch_x - s->u.x;
        const int y = s->u.a->last_touch_y - s->u.y;
        if (slider_click_inc_dec(s, slider_dec_inc(s, x, y), slider_scale(s))) {
            slider_notify(s);
        }
    }
}

static void slider_start_autorepeat(timer_callback_t* tcb) {
    slider_t* s = (slider_t*)tcb->that;
    s->u.a->timer_remove(s->u.a, tcb);
    if ((s->u.a->touch_flags & MOUSE_LBUTTON_FLAG) != 0) {
        s->timer_callback.callback = slider_autorepeat;
        s->timer_callback.ns = (1000ULL * NS_IN_MS) / 30; // 30 times per second
        s->u.a->timer_add(s->u.a, tcb);
    }
}

static bool slider_touch(ui_t* u, int touch_action, float x, float y) {
    slider_t* s = (slider_t*)u;
    app_t* a = u->a;
    theme_t* theme = &u->a->theme;
    bool consumed = false;
    if (s->u.focusable && (touch_action & TOUCH_DOWN) != 0) {
        bool changed = slider_click_inc_dec(s, slider_dec_inc(s, x, y), slider_scale(s));
        if (changed) {
            if (s->timer_callback.id <= 0) {
                s->timer_callback.that = s;
                s->timer_callback.callback = slider_start_autorepeat;
                s->timer_callback.ns = 350ULL * NS_IN_MS; // first timer fires in 350 milliseconds
                a->timer_add(a, &s->timer_callback);
            }
            slider_notify(s);
            consumed = true;
        }
    } else if (s->u.focusable && (a->touch_flags & MOUSE_LBUTTON_FLAG)) { // drag with mouse button down
        const float em4 = theme->font->em / 4;
        const float dec_width = font_text_width(theme->font, SLIDER_DEC_LABEL, -1) + em4;
        const float inc_width = font_text_width(theme->font, SLIDER_INC_LABEL, -1) + em4;
        if (dec_width <= x && x <= u->w - inc_width) {
            if (s->minimum != null && s->maximum != null && s->current != null) {
                int range = *s->maximum - *s->minimum + 1;
                assertion(range > 0, "empty or inverted range [%d..%d]", *s->minimum, *s->maximum);
                double ratio = (x - dec_width) / (u->w - dec_width - inc_width);
                assertion(0 <= ratio && ratio <= 1.0, "ratio=%.3f", ratio);
                if (range > 0) {
                    int v = (int)(*s->minimum + ratio * range);
                    assertion(*s->minimum <= v && v <= *s->maximum, "[%d..%d] range=%d ratio=%.3f v=%d", *s->minimum, *s->maximum, range, ratio, v);
                    if (*s->minimum <= v && v <= *s->maximum && v != *s->current) {
                        *s->current = v;
                        slider_notify(s);
                        consumed = true;
                    }
                }
            }
        }
    }
    return consumed;
}

static void slider_screen_touch(ui_t* u, int touch_action, float x, float y) {
    slider_t* s = (slider_t*)u;
    app_t* a = u->a;
    if (((a->touch_flags & MOUSE_LBUTTON_FLAG) == 0 || (touch_action & TOUCH_UP) != 0) && s->timer_callback.id != 0) {
        a->timer_remove(u->a, &s->timer_callback);
    }
}

static int slider_dec_inc_key(slider_t* s, int flags, int ch) {
    int scale = slider_scale(s);
    int range = -1;
    int page = 1;
    if (s->minimum != null && s->maximum != null && s->current != null) {
        range = *s->maximum - *s->minimum + 1;
        page = range / 16; // 16 pages in scroller range, takes 1/2 second to PageUp/PageDw thru at 33Hz
        if (page == 0) { page = 1; }
    }
    switch (ch) {
        case '+': case '=': case '>': case '.': return +1; // TODO: '<' is over ',' on US keyboard, can be different for other locales
        case '-': case '_': case '<': case ',': return -1; // TODO: '>' is over '.' on US keyboard, can be different for other locales
        case KEY_CODE_RIGHT: case KEY_CODE_UP:   return  scale;
        case KEY_CODE_LEFT:  case KEY_CODE_DOWN: return -scale;
        case KEY_CODE_PAGE_UP:   return  page;
        case KEY_CODE_PAGE_DOWN: return -page;
        default: return 0;
    }
}

static bool slider_keyboard(ui_t* u, int flags, int ch) {
    bool consumed = false;
    slider_t* s = (slider_t*)u;
    int dx = slider_dec_inc_key(s, flags, ch);
    if (dx != 0) {
        int v = *s->current + dx;
        if (v < *s->minimum) { v = *s->minimum; }
        if (*s->maximum < v) { v = *s->maximum; }
        if (v != *s->current) {
            *s->current = v;
            slider_notify(s);
            consumed = true;
        }
    }
    return consumed;
}

static void slider_draw(ui_t* u) {
    slider_t* s = (slider_t*)u;
    app_t* a = u->a;
    theme_t* theme = &a->theme;
    assertion(*s->maximum - *s->minimum > 0, "range must be positive [%d..%d]", *s->minimum, *s->maximum);
    const pointf_t pt = u->screen_xy(u);
    font_t* f = theme->font;
    const float fh = f->height;
    const float em4 = f->em / 4;
    const float baseline = f->baseline;
    float x = pt.x;
    float y = pt.y + (int)(baseline + (u->h - fh) / 2);
    const float dec_width = s->u.focusable ? font_text_width(f, SLIDER_DEC_LABEL, -1) + em4 : 0;
    const float inc_width = s->u.focusable ? font_text_width(f, SLIDER_INC_LABEL, -1) + em4 : 0;
    const float indicator_width = u->w - dec_width - inc_width;
    if (s->label != null) {
        dc.text(&dc, theme->color_text, f, x + dec_width, y, s->label, (int)strlen(s->label));
    }
    if (s->u.focusable) {
        dc.text(&dc, theme->color_background, f, x, y, SLIDER_DEC_LABEL, (int)strlen(SLIDER_DEC_LABEL));
        dc.text(&dc, theme->color_background, f, x + u->w - inc_width, y, SLIDER_INC_LABEL, (int)strlen(SLIDER_INC_LABEL));
    }
    if (indicator_width <= 0) {
        traceln("WARNING: indicator_width=%.1f < 0", indicator_width);
    } else {
        assertion(*s->minimum <= *s->current && *s->current <= *s->maximum, "s->current=%d out of range: [%d..%d]", *s->current, *s->minimum, *s->maximum);
        const double r = (*s->current - *s->minimum) / (double)(*s->maximum - *s->minimum);
        x = pt.x + dec_width;
        y = u->y + baseline + 1.5;
        const float w = (float)(indicator_width * r);
        const float h = u->h - baseline - 3;
        dc.fill(&dc, theme->color_slider, x, y, w, h);
        dc.fill(&dc, theme->color_background, x + w, y, indicator_width - w, h);
    }
    if (a->focused == u) { // for focused slider highlight +/= with mnemonics color:
        x = pt.x;
        y = pt.y + (int)(baseline + (u->h - fh) / 2);
        dc.text(&dc, theme->color_mnemonic, f, x + f->em, y, SLIDER_DEC_LABEL + 1, 1);
        dc.text(&dc, theme->color_mnemonic, f, x + f->em + u->w - inc_width, y, SLIDER_INC_LABEL + 1, 1);
    }
}

void slider_init(slider_t* s, ui_t* parent, void* that,
                 const char* label, float x, float y, float w, float h,
                 int* minimum, int* maximum, int* current) {
    theme_t* theme = &parent->a->theme;
    const float em4 = theme->font->em / 4;
    const float dec_width = font_text_width(theme->font, SLIDER_DEC_LABEL, -1) + em4;
    const float inc_width = font_text_width(theme->font, SLIDER_INC_LABEL, -1) + em4;
    s->u = *parent;
    s->u.that = that;
    s->u.parent = null;
    s->u.children = null;
    s->u.next = null;
    s->u.focusable = true; // default: with buttons
    s->u.hidden = false;
    s->notify = null;
    s->label = label;
    s->minimum = minimum;
    s->maximum = maximum;
    s->current = current;
    assert((void*)s == (void*)&s->u);
    parent->add(parent, &s->u, x, y, dec_width + w + inc_width, h);
    assert(s->u.parent == parent);
    s->u.kind = UI_KIND_SLIDER;
    s->u.draw = slider_draw;
    s->u.touch = slider_touch;
    s->u.keyboard = slider_keyboard;
    s->u.screen_touch = slider_screen_touch;
}

void slider_done(slider_t* s) {
    if (s != null) {
        s->u.parent->remove(s->u.parent, &s->u);
        memset(s, 0, sizeof(*s));
    }
}

end_c
