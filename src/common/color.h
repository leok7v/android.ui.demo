#pragma once
#include "rt.h"

BEGIN_C

typedef struct colorf_s { float r, g, b, a; } packed colorf_t;

colorf_t colorf_from_rgb(uint32_t argb);

typedef struct colors_s { // treat all fields as a const
    colorf_t transparent;
    colorf_t black;
    colorf_t white;
    colorf_t red;
    colorf_t green;
    colorf_t blue;
    colorf_t orange;
    // "Norton Commander" palette see:
    // https://en.wikipedia.org/wiki/Norton_Commander#/media/File:Norton_Commander_5.51.png
    colorf_t nc_dark_blue;    // background color
    colorf_t nc_light_blue;   // light bluemain text color
    colorf_t nc_dirty_gold;   // selection and labels color
    colorf_t nc_light_grey;   // keyboard menu color (on black)
    colorf_t nc_teal;         // keyboard menu labels background (on almost black)
    colorf_t nc_darker_blue;  // darker blue
    colorf_t nc_almost_black; // keyboard menu text (on teal)
} colors_t;

extern colors_t colors; // treat as const

END_C
