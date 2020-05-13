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
#include "ui.h"

begin_c

typedef struct edit_chunk_s edit_chunk_t;

typedef struct {
    int l; // line
    int c; // column
} edit_position_t;

typedef struct edit_s edit_t;

typedef struct edit_s {
    ui_t u;
    void* that;
    char* text;  // this can be readonly mmaped memory o null
    int   bytes; // number of bytes in possibly utf8 text
    void (*notify)(edit_t* e); // called every time text changed
    // implementation:
    int screen; // byte position in chunks of the top left of the screen
    edit_position_t top; // top left of the screen in (l,c) coordinates
    edit_position_t cursor;
    edit_position_t from; // selection
    edit_position_t to;
    edit_chunk_t* chunks; // heap allocated
    edit_chunk_t* last_chunk; // last chunk with valid .pos
} edit_t;

void edit_init(edit_t* e, ui_t* parent, void* that, float x, float y, float w, float h);
int  edit_bytes(edit_t* e); // size in bytes of edited text
void edit_read(edit_t* e, char* text, int position, int bytes); // read edited text
void edit_done(edit_t* e);

end_c
