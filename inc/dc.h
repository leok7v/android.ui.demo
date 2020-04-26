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
#include "bitmap.h"
#include "color.h"
#include "font.h"
#include "linmath.h"

BEGIN_C

typedef struct pointf_s { float x; float y; } packed pointf_t;
typedef struct rectf_s  { float x; float y; float w; float h; } packed rectf_t;
typedef struct quadf_s  { float x; float y; float s; float t; } packed quadf_t;

typedef struct dc_s dc_t;

typedef struct dc_s { // draw commands/context
    void (*init)(dc_t* dc);
    void (*viewport)(dc_t* dc, float x, float y, float w, float h);
    void (*dispose)(dc_t* dc);
    void (*clear)(dc_t* dc, const colorf_t* color);
    void (*fill)(dc_t* dc, const colorf_t* color, float x, float y, float w, float h);
    void (*rect)(dc_t* dc, const colorf_t* color, float x, float y, float w, float h, float thickness);
    void (*ring)(dc_t* dc, const colorf_t* color, float x, float y, float radius, float inner);
    void (*bblt)(dc_t* dc, const bitmap_t* bitmap, float x, float y);
    void (*luma)(dc_t* dc, const colorf_t* color, bitmap_t* bitmap, float x, float y);
    void (*tex4)(dc_t* dc, const colorf_t* color, bitmap_t* bitmap, quadf_t* quads, int count);
    void (*poly)(dc_t* dc, const colorf_t* color, const pointf_t* vertices, int count); // filled with TRIANGLE_FAN
    void (*line)(dc_t* dc, const colorf_t* c, float x0, float y0, float x1, float y1, float thickness);
    float(*text)(dc_t* dc, const colorf_t* color, font_t* font, float x, float y, const char* text, int count);
    void (*quadrant)(dc_t* dc, const colorf_t* color, float x, float y, float r, int quadrant);
    void (*stadium)(dc_t* dc, const colorf_t* color, float x, float y, float w, float h, float r);
    mat4x4 mvp; // model * view * projection
} dc_t;

extern dc_t dc;

END_C
