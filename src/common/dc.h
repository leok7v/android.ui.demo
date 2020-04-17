#pragma once
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
    void (*init)(dc_t* dc, mat4x4 mvp, ...);
    void (*dispose)(dc_t* dc);
    void (*clear)(dc_t* dc, const colorf_t* color);
    void (*fill)(dc_t* dc, const colorf_t* color, float x, float y, float w, float h);
    void (*rect)(dc_t* dc, const colorf_t* color, float x, float y, float w, float h, float thickness);
    void (*ring)(dc_t* dc, const colorf_t* color, float x, float y, float radius, float inner);
    void (*bblt)(dc_t* dc, const bitmap_t* bitmap, float x, float y);
    void (*luma)(dc_t* dc, const colorf_t* color, bitmap_t* bitmap, float x, float y);
    void (*quad)(dc_t* dc, const colorf_t* color, bitmap_t* bitmap, quadf_t* quads, int count);
    void (*poly)(dc_t* dc, const colorf_t* color, pointf_t* vertices, int count); // filled with TRIANGLE_FAN
    int  (*text)(dc_t* dc, const colorf_t* color, font_t* font, float x, float y, const char* text, int count);
    mat4x4 mvp;
} dc_t;

dc_t dc;

END_C
