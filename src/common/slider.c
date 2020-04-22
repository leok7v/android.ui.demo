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
#include "slider.h"
#include "app.h"

BEGIN_C

static const char* SLIDER_DEC_LABEL = "[-]";
static const char* SLIDER_INC_LABEL = "[+]";

static void slider_notify(slider_t* s) {
    s->notify(s);
    s->ui.a->invalidate(s->ui.a); // after notify because notify may do something to layout etc...
}

static int slider_scale(slider_t* s) {
    enum { CTRL_SHIFT = KEYBOARD_CTRL|KEYBOARD_SHIFT };
//  if (app.trace_flags & APP_TRACE_KEYBOARD) { app_trace_key(s->ui.a->keyboard_flags, 0); }
    return (s->ui.a->keyboard_flags & CTRL_SHIFT) == CTRL_SHIFT     ? 1000 :
           (s->ui.a->keyboard_flags & CTRL_SHIFT) == KEYBOARD_SHIFT ?  100 :
           (s->ui.a->keyboard_flags & CTRL_SHIFT) == KEYBOARD_CTRL  ?   10 : 1;
}

static int slider_dec_inc(slider_t* s, float x, float y) {
    const float em4 = s->theme->font->em / 4;
    const float dec_width = font_text_width(s->theme->font, SLIDER_DEC_LABEL, -1) + em4;
    const float inc_width = font_text_width(s->theme->font, SLIDER_INC_LABEL, -1) + em4;
    if (0 <= x && x <= dec_width) {
        if (s->minimum != null && s->maximum != null && s->current != null && *s->minimum < *s->current) {
            return -1;
        }
    } else if (s->ui.w - inc_width <= x && x <= s->ui.w) {
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
    if ((s->ui.a->mouse_flags & MOUSE_LBUTTON_FLAG) == 0 && tcb->id > 0) {
        s->ui.a->timer_remove(s->ui.a, tcb);
    } else {
        const int x = s->ui.a->last_mouse_x - s->ui.x;
        const int y = s->ui.a->last_mouse_y - s->ui.y;
        if (slider_click_inc_dec(s, slider_dec_inc(s, x, y), slider_scale(s))) {
            slider_notify(s);
        }
    }
}

static void slider_start_autorepeat(timer_callback_t* tcb) {
    slider_t* s = (slider_t*)tcb->that;
    s->ui.a->timer_remove(s->ui.a, tcb);
    if ((s->ui.a->mouse_flags & MOUSE_LBUTTON_FLAG) != 0) {
        s->timer_callback.callback = slider_autorepeat;
        s->timer_callback.ns = (1000ULL * NS_IN_MS) / 30; // 30 times per second
        s->ui.a->timer_add(s->ui.a, tcb);
    }
}

static void slider_mouse(ui_t* self, int mouse_action, float x, float y) {
    slider_t* s = (slider_t*)self;
    app_t* a = self->a;
    if (s->ui.focusable && (mouse_action & MOUSE_LBUTTON_DOWN) != 0) {
        bool changed = slider_click_inc_dec(s, slider_dec_inc(s, x, y), slider_scale(s));
        if (changed) {
            if (s->timer_callback.id <= 0) {
                s->timer_callback.that = s;
                s->timer_callback.callback = slider_start_autorepeat;
                s->timer_callback.ns = 350ULL * NS_IN_MS; // first timer fires in 350 milliseconds
                a->timer_add(self->a, &s->timer_callback);
            }
            slider_notify(s);
        }
    } else if (s->ui.focusable && (a->mouse_flags & MOUSE_LBUTTON_FLAG)) { // drag with mouse button down
        const float em4 = s->theme->font->em / 4;
        const float dec_width = font_text_width(s->theme->font, SLIDER_DEC_LABEL, -1) + em4;
        const float inc_width = font_text_width(s->theme->font, SLIDER_INC_LABEL, -1) + em4;
        if (dec_width <= x && x <= self->w - inc_width) {
            if (s->minimum != null && s->maximum != null && s->current != null) {
                int range = *s->maximum - *s->minimum + 1;
                assertion(range > 0, "empty or inverted range [%d..%d]", *s->minimum, *s->maximum);
                double ratio = (x - dec_width) / (self->w - dec_width - inc_width);
                assertion(0 <= ratio && ratio <= 1.0, "ratio=%.3f", ratio);
                if (range > 0) {
                    int v = (int)(*s->minimum + ratio * range);
//                  traceln("range=%d ratio=%.3f v=%d", range, ratio, v);
                    assertion(*s->minimum <= v && v <= *s->maximum, "[%d..%d] range=%d ratio=%.3f v=%d", *s->minimum, *s->maximum, range, ratio, v);
                    if (*s->minimum <= v && v <= *s->maximum && v != *s->current) { *s->current = v; slider_notify(s); }
                }
            }
        }
    }
}

static void slider_screen_mouse(ui_t* self, int mouse_action, float x, float y) {
    slider_t* s = (slider_t*)self;
    app_t* a = self->a;
    if (((a->mouse_flags & MOUSE_LBUTTON_FLAG) == 0 || (mouse_action & MOUSE_LBUTTON_UP) != 0) && s->timer_callback.id != 0) {
        a->timer_remove(self->a, &s->timer_callback);
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

static void slider_keyboard(ui_t* self, int flags, int ch) {
    slider_t* s = (slider_t*)self;
//  if (app.trace_flags & APP_TRACE_KEYBOARD) { app_trace_key(flags, ch); }
    int dx = slider_dec_inc_key(s, flags, ch);
    if (dx != 0) {
        int v = *s->current + dx;
        if (v < *s->minimum) { v = *s->minimum; }
        if (*s->maximum < v) { v = *s->maximum; }
        if (v != *s->current) { *s->current = v; slider_notify(s); }
    }
}

static void slider_draw(ui_t* self) {
    slider_t* s = (slider_t*)self;
    app_t* a = self->a;
    assertion(*s->maximum - *s->minimum > 0, "range must be positive [%d..%d]", *s->minimum, *s->maximum);
    const pointf_t pt = self->screen_xy(self);
    font_t* f = s->theme->font;
    const float fh = f->height;
    const float em4 = f->em / 4;
    const float baseline = f->baseline;
    float x = pt.x;
    float y = pt.y + (int)(baseline + (self->h - fh) / 2);
    const float dec_width = s->ui.focusable ? font_text_width(f, SLIDER_DEC_LABEL, -1) + em4 : 0;
    const float inc_width = s->ui.focusable ? font_text_width(f, SLIDER_INC_LABEL, -1) + em4 : 0;
    const float indicator_width = self->w - dec_width - inc_width;
    if (s->label != null) {
        dc.text(&dc, s->theme->color_text, f, x + dec_width, y, s->label, (int)strlen(s->label));
    }
    if (s->ui.focusable) {
        dc.text(&dc, s->theme->color_background, f, x, y, SLIDER_DEC_LABEL, (int)strlen(SLIDER_DEC_LABEL));
        dc.text(&dc, s->theme->color_background, f, x + self->w - inc_width, y, SLIDER_INC_LABEL, (int)strlen(SLIDER_INC_LABEL));
    }
    if (indicator_width <= 0) {
        traceln("WARNING: indicator_width=%.1f < 0", indicator_width);
    } else {
        assertion(*s->minimum <= *s->current && *s->current <= *s->maximum, "s->current=%d out of range: [%d..%d]", *s->current, *s->minimum, *s->maximum);
        const double r = (*s->current - *s->minimum) / (double)(*s->maximum - *s->minimum);
        x = pt.x + dec_width;
        y = self->y + baseline + 1.5;
        const float w = (float)(indicator_width * r);
        const float h = self->h - baseline - 3;
        dc.fill(&dc, s->color_slider, x, y, w, h);
        dc.fill(&dc, s->theme->color_background, x + w, y, indicator_width - w, h);
    }
    if (a->focused == self) { // for focused slider highlight +/= with mnemonics color:
        x = pt.x;
        y = pt.y + (int)(baseline + (self->h - fh) / 2);
        dc.text(&dc, s->theme->color_mnemonic, f, x + f->em, y, SLIDER_DEC_LABEL + 1, 1);
        dc.text(&dc, s->theme->color_mnemonic, f, x + f->em + self->w - inc_width, y, SLIDER_INC_LABEL + 1, 1);
    }
}

slider_t* slider_create(ui_t* parent, void* that, ui_theme_t* theme,
                        colorf_t* color_slider,
                        const char* label, float x, float y, float w, float h,
                        int* minimum, int* maximum, int* current) {
    slider_t* s = (slider_t*)allocate(sizeof(slider_t));
    if (s != null) {
        const float em4 = theme->font->em / 4;
        const float dec_width = font_text_width(theme->font, SLIDER_DEC_LABEL, -1) + em4;
        const float inc_width = font_text_width(theme->font, SLIDER_INC_LABEL, -1) + em4;
        s->ui = *parent;
        s->ui.that = that;
        s->ui.parent = null;
        s->ui.children = null;
        s->ui.next = null;
        s->ui.focusable = true; // default: with buttons
        s->ui.hidden = false;
        s->notify = null;
        s->label = label;
        s->minimum = minimum;
        s->maximum = maximum;
        s->current = current;
        s->theme = theme;
        s->color_slider = color_slider;
        assert((void*)s == (void*)&s->ui);
        parent->add(parent, &s->ui, x, y, dec_width + w + inc_width, h);
        assert(s->ui.parent == parent);
//      traceln("ui.(x,y)[w x h] = (%.1f,%.1f)[%.1fx%.1f]", s->ui.x, s->ui.y, s->ui.w, s->ui.h);
        s->ui.kind = UI_KIND_SLIDER;
        s->ui.draw = slider_draw;
        s->ui.mouse = slider_mouse;
        s->ui.keyboard = slider_keyboard;
        s->ui.screen_mouse = slider_screen_mouse;
    }
//  traceln("slider=%p", s);
    return s;
}

void slider_dispose(slider_t* s) {
    if (s != null) {
        s->ui.parent->remove(s->ui.parent, &s->ui);
        deallocate(s);
    }
}

END_C
