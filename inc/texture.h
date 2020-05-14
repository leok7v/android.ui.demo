#pragma once
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
#include "c.h"

begin_c

/* texture_t is super simple holder for GL texture2D with a data buffer in main CPU RAM */

typedef struct texture_s texture_t;

typedef struct texture_s {
    int w;
    int h;
    int comp;  // components per pixel 1 (grey), 2(grey+alpha), 3 (rgb), 4(rgba)
    int ti;    // texture index in OpenGL 0 if not texture attached
    void* data;
} texture_t;

typedef struct app_s app_t;

int texture_allocate(texture_t* b);

int texture_deallocate(texture_t* b);

int texture_update(texture_t* b);

int texture_allocate_and_update(texture_t* b);

int texture_load_asset(texture_t* b, app_t* a, const char* name);

void texture_dispose(texture_t* b);

end_c
