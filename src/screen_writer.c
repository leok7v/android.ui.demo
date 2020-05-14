/* Copyright 2020 "Leo" Dmitry Kuznetsov https://leok7v.github.io/
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */
#include "screen_writer.h"
#include "color.h"
#include "app.h"
#include "font.h"
#include "ui.h"

begin_c

static void screen_writer_draw_text(screen_writer_t* sw, const char* format, va_list vl, bool line_feed) {
    assert(sw != null && format != null);
    char text[1024];
    vsnprintf0(text, format, vl);
    float x = dc.text(&dc, sw->color, sw->font, sw->x, sw->y, text, (int)strlen(text));
    if (line_feed) {
        sw->y += sw->font->height;
    } else {
        sw->x = x;
    }
}

static void draw_text_newline(screen_writer_t* sw, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    screen_writer_draw_text(sw, format, vl, true);
    va_end(vl);
}

static void draw_text_inline(screen_writer_t* sw, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    screen_writer_draw_text(sw, format, vl, false);
    va_end(vl);
}

screen_writer_t screen_writer(float x, float y, font_t* f, const colorf_t* c) {
    screen_writer_t tp;
    tp.font = f;
    tp.x = x;
    tp.y = y;
    tp.print = draw_text_inline;
    tp.println = draw_text_newline;
    tp.color = c;
    return tp;
}

end_c