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
#include "toast.h"
#include "app.h"
#include "screen_writer.h"

BEGIN_C

static const uint64_t TOAST_TIME_IN_NS = 2750LL * NS_IN_MS; // 2.75s

static void toast_timer_callback(timer_callback_t* timer_callback) {
    app_t* a = (app_t*)timer_callback->that;
    a->invalidate(a);
}

static void toast_add(toast_t* t) {
    app_t* a = t->ui.a;
    assert(t->toast_timer_callback.id == 0);
    t->toast_start_time = a->time_in_nanoseconds;
    t->toast_timer_callback.id = 0;
    t->toast_timer_callback.that = a;
    t->toast_timer_callback.ns = t->nanoseconds / 2;
    t->toast_timer_callback.callback = toast_timer_callback;
    t->toast_timer_callback.last_fired = 0;
    a->timer_add(a, &t->toast_timer_callback);
    t->ui.hidden = false;
}

static void toast_cancel(toast_t* t) {
    if (t->toast_timer_callback.id != 0) { // it is not an error to cancel cancelled toast
        app_t* a = t->ui.a;
        a->timer_remove(a, &t->toast_timer_callback);
        memset(&t->toast_timer_callback, 0, sizeof(t->toast_timer_callback));
        t->text[0] = 0; // toast OFF
        t->toast_start_time = 0;
        t->ui.hidden = true;
    }
}

static int toast_measure(toast_t* t, float *w, float *h) {
    assert(t->theme != null && t->theme->font != null);
    font_t* f = t->theme->font;
    int n = 1;
    char* s = t->text;
    while (*s != 0) { n += *s == '\n'; s++; }
    s = t->text;
    for (int i = 0; i < n; i++) {
        int k = 0;
        char* line = s;
        while (*s != 0 && *s != '\n') { k++; s++; }
        if (*s == '\n') { s++; }
        float width = font_text_width(f, line, k);
        if (width > *w) { *w = width; }
    }
    *h = n * f->height;
    return n;
}

static void toast_render(toast_t* t) {
    app_t* a = t->ui.a;
    float w = 0;
    float h = 0;
    int n = toast_measure(t, &w, &h);
    font_t* f = t->theme->font;
    w += f->em * 2;
    h += f->em * 2;
    int x = (int)((a->root->w - w) / 2);
    int y = (int)((a->root->h - h) / 2);
    colorf_t c = *colors_dk.light_gray;
    c.a = 0.75;
    dc.fill(&dc, &c, x, y, w, h);
    char* s = t->text;
    screen_writer_t sw = screen_writer(0, y + f->em / 2 + f->height, f, colors.black);
    for (int i = 0; i < n; i++) {
        int k = 0;
        char* line = s;
        while (*s != 0 && *s != '\n') { k++; s++; }
        if (*s == '\n') { s++; }
        float width = font_text_width(f, line, k);
        sw.x = x + (w - width) / 2;
        sw.println(&sw, "%.*s", k, line);
    }
}

static void toast_draw(ui_t* self) {
    toast_t* t = self->that;
    app_t* a = self->a;
    assert(t->theme != null && t->theme->font != null);
    if (t->text[0] != 0) {
        if (t->toast_start_time == 0) {
            toast_add(t);
        } else {
            int64_t time_on_screen = a->time_in_nanoseconds - t->toast_start_time;
            if (time_on_screen > t->nanoseconds) { // nanoseconds
                toast_cancel(t);
            } else if (t->theme != null && t->theme->font != null) {
                toast_render(t);
            }
        }
    }
}

static void toast_print(toast_t* t, const char* format, ...) {
    assertion(t->ui.parent == ui_root, "toast() must be added to ui_root");
    if (t->toast_timer_callback.id != 0) { t->cancel(t); }
    assert(t->toast_start_time == 0); // timer will be added in toast_draw
    va_list vl;
    va_start(vl, format);
    vsnprintf0(t->text, format, vl);
    va_end(vl);
    t->ui.hidden = false;
    t->ui.a->invalidate(t->ui.a);
}

toast_t* toast(app_t* a) {
    static toast_t toast;
    if (toast.ui.a == null) {
        toast.cancel = toast_cancel;
        toast.print = toast_print;
        toast.ui.a = a;
        toast.ui = *ui_root;
        toast.nanoseconds = TOAST_TIME_IN_NS;
        toast.ui.children = null;
        toast.ui.parent = null;
        toast.ui.focus = false;
        toast.ui.hidden = true;
        toast.ui.decor = true;
        toast.ui.kind = UI_KIND_CONTAINER;
        toast.ui.that = &toast;
        toast.ui.draw = toast_draw;
    }
    return &toast;
}

END_C
