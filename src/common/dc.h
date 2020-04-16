#pragma once
#include "color.h"
#include "bitmap.h"
#include "linmath.h"

BEGIN_C

typedef struct pointf_s { float x; float y; } pointf_t;
typedef struct rectf_s  { float x; float y; float w; float h; } rectf_t;

typedef struct dc_s dc_t;

typedef struct dc_s { // draw commands/context
    void (*init)(dc_t* dc, mat4x4 mvp, ...);
    void (*dispose)(dc_t* dc);
    void (*clear)(dc_t* dc, colorf_t* color);
    void (*fill)(dc_t* dc, colorf_t* color, float x, float y, float w, float h);
    void (*rect)(dc_t* dc, colorf_t* color, float x, float y, float w, float h);
    void (*line)(dc_t* dc, colorf_t* color, float x0, float y0, float x1, float y1, float width);
    void (*ring)(dc_t* dc, colorf_t* color, float x, float y, float radius, float inner);
    void (*bblt)(dc_t* dc, bitmap_t* bitmap, float x, float y);
    void (*luma)(dc_t* dc, bitmap_t* bitmap, colorf_t* color, float x, float y, float s, float t, float w, float h);
    void (*poly)(dc_t* dc, colorf_t* color, const pointf_t* vertecies, int count); // triangle fan
    mat4x4 mvp;
} dc_t;

dc_t dc;

END_C
