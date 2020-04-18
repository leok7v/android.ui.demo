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

typedef struct colorf_s { float r, g, b, a; } packed colorf_t;

colorf_t colorf_from_rgb(uint32_t argb);

typedef struct colors_s { // treat all fields as a const
    colorf_t transparent;
    // "primary" colors:
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
    colorf_t nc_light_gray;   // keyboard menu color (on black)
    colorf_t nc_teal;         // keyboard menu labels background (on almost black)
    colorf_t nc_darker_blue;  // darker blue
    colorf_t nc_almost_black; // keyboard menu text (on teal)
    // "Google Android Dark Theme"
    colorf_t dk_light_blue;
    colorf_t dk_dark_blue;
    colorf_t dk_light_gray;
    colorf_t dk_dark_gray;
    colorf_t dk_gray_text;
} colors_t;

extern colors_t colors; // treat as const

END_C
