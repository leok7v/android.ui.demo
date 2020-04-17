#pragma once
#include "rt.h"
#include "bitmap.h"
#include "color.h"
#include "stb_inc.h"
#include "stb_truetype.h" // for stbtt_fontinfo

BEGIN_C

typedef struct font_s {
    int   from;   // first glyph index
    int   count;  // number of glyphs
    int   height; // in pixels
    float em;     // width of "M" in pixels
    float ascent;   // distance from baseline to the top pixel of the glyph
    float descent;  // negative max glyphs descent below baseline
    float baseline; // positive the vertical offset from the top of rendering box to the glyph baseline
    // internal implementation details:
    stbtt_fontinfo fi;
    void* chars;  // per character info
    bitmap_t atlas;
} font_t;

typedef struct app_s app_t;

int font_find_glyph_index(font_t* f, int unicode_codepoint); // returns -1 if glyph not found

// all functions returns 0 on success posix error otherwise

int font_load_asset(font_t* f, app_t* a, const char* name, int height_in_pixels, int from, int count);

float font_text_width(font_t* f, const char* text);

void font_dispose(font_t* font);

END_C
