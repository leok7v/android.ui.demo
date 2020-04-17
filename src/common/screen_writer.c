#include "screen_writer.h"
#include "color.h"
#include "app.h"
#include "font.h"
#include "glh.h"
#include "ui.h"

BEGIN_C

static void screen_writer_draw_text(screen_writer_t* sw, const char* format, va_list vl, bool line_feed) {
    assert(sw != null && format != null);
    char text[1024];
    int r = vsnprintf(text, countof(text), format, vl);
    (void)r; // unused - can be used for stack_alloc() resize of text
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

END_C