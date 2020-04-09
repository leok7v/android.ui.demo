#pragma once
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
