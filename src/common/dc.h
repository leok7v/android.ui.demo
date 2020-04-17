#pragma once
#include "color.h"
#include "bitmap.h"
#include "linmath.h"

BEGIN_C

typedef struct pointf_s { float x; float y; } pointf_t;
typedef struct rectf_s  { float x; float y; float w; float h; } rectf_t;
typedef struct quadf_s  { float x; float y; float s; float t; } quadf_t;

typedef struct dc_s dc_t;

typedef struct dc_s { // draw commands/context
    void (*init)(dc_t* dc, mat4x4 mvp, ...);
    void (*dispose)(dc_t* dc);
    void (*clear)(dc_t* dc, colorf_t* color);
    void (*fill)(dc_t* dc, colorf_t* color, float x, float y, float w, float h);
    void (*rect)(dc_t* dc, colorf_t* color, float x, float y, float w, float h, float thickness);
    void (*ring)(dc_t* dc, colorf_t* color, float x, float y, float radius, float inner);
    void (*bblt)(dc_t* dc, bitmap_t* bitmap, float x, float y);
    void (*luma)(dc_t* dc, colorf_t* color, bitmap_t* bitmap, float x, float y, float w, float h);
    void (*quad)(dc_t* dc, colorf_t* color, bitmap_t* bitmap, quadf_t* quads, int count);
    void (*poly)(dc_t* dc, colorf_t* color, pointf_t* vertices, int count); // filled TRIANGLE_FAN
    mat4x4 mvp;
} dc_t;

dc_t dc;

END_C
