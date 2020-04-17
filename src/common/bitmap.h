#pragma once
#include "rt.h"

BEGIN_C

typedef struct bitmap_s bitmap_t;

typedef struct bitmap_s {
    int w;
    int h;
    int comp;  // components per pixel 1 (grey), 2(grey+alpha), 3 (rgb), 4(rgba)
    int ti;    // texture index in OpenGL 0 if not texture attached
    byte* data;
} bitmap_t;

typedef struct app_s app_t;

int bitmap_allocate_texture(bitmap_t* b);

int bitmap_deallocate_texture(bitmap_t* b);

int bitmap_update_texture(bitmap_t* b);

int bitmap_allocate_and_update_texture(bitmap_t* b);

int bitmap_load_asset(bitmap_t* b, app_t* a, const char* name);

void bitmap_dispose(bitmap_t* b);

END_C
