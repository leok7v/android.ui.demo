#pragma once
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
#include "c.h"
#include "color.h"
#include "stb_inc.h"
#include "stb_truetype.h" // for stbtt_fontinfo
#include "texture.h"

begin_c

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
    texture_t atlas;
} font_t;

typedef struct app_s app_t;

int font_find_glyph_index(font_t* f, int unicode_codepoint); // returns -1 if glyph not found

// all functions returns 0 on success posix error otherwise

int font_load_asset(font_t* f, app_t* a, const char* name, int height_in_pixels, int from, int count);

float font_text_width(font_t* f, const char* text, int count); // count == -1, use strlen(text)

void font_dispose(font_t* font);

end_c
