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

begin_c

typedef struct bitmap_s bitmap_t;

typedef struct bitmap_s {
    int w;
    int h;
    int comp;  // components per pixel 1 (grey), 2(grey+alpha), 3 (rgb), 4(rgba)
    int ti;    // texture index in OpenGL 0 if not texture attached
    void* data;
} bitmap_t;

typedef struct app_s app_t;

int bitmap_allocate_texture(bitmap_t* b);

int bitmap_deallocate_texture(bitmap_t* b);

int bitmap_update_texture(bitmap_t* b);

int bitmap_allocate_and_update_texture(bitmap_t* b);

int bitmap_load_asset(bitmap_t* b, app_t* a, const char* name);

void bitmap_dispose(bitmap_t* b);

end_c
