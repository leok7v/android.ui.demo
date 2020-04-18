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
#include "rt.h"

BEGIN_C

typedef struct font_s font_t;
typedef struct colorf_s colorf_t;

typedef struct screen_writer_s screen_writer_t; // wraps text lines on '\n' character

typedef struct screen_writer_s {
    font_t* font;
    const colorf_t* color;
    float x;
    float y;
    void (*print)(screen_writer_t* tw, const char* format, ...);   // draws on the same line advances ".x"
    void (*println)(screen_writer_t* tw, const char* format, ...); // printf with new line advances ".y"
} screen_writer_t;

screen_writer_t screen_writer(float x, float y, font_t* f, const colorf_t* c);

END_C
