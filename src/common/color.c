#include "color.h"
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
#include "app.h"

BEGIN_C

colorf_t colorf_from_rgb(uint32_t argb) {
    colorf_t f = { ((argb >> 16) & 0xFF) / 255.0f, ((argb >>  8) & 0xFF) / 255.0f,
                   ((argb >>  0) & 0xFF) / 255.0f, ((argb >> 24) & 0xFF) / 255.0f };
    return f;
}

colors_t colors;

static_init(color) {
//  colors.color_transparent = {0.0f, 0.0f, 0.0f, 0.0f}; // already initialized by zeros
    colors.black           = colorf_from_rgb(0xff000000);
    colors.white           = colorf_from_rgb(0xffFFFFFF);
    colors.red             = colorf_from_rgb(0xffFF0000);
    colors.green           = colorf_from_rgb(0xff00FF00);
    colors.blue            = colorf_from_rgb(0xff0000FF);
    colors.orange          = colorf_from_rgb(0xffEE7600); // alt. 0xCC5500
    // "Norton Commander" palette see: https://photos.app->goo.gl/zbjPiiZkcr628BcI2
    colors.nc_dark_blue    = colorf_from_rgb(0xff0002A8); // background color
    colors.nc_light_blue   = colorf_from_rgb(0xff67F7FD); // light blue main text color
    colors.nc_dirty_gold   = colorf_from_rgb(0xffF0FFAF); // selection and labels color
    colors.nc_light_gray   = colorf_from_rgb(0xffAAABA9); // keyboard menu color (on black)
    colors.nc_teal         = colorf_from_rgb(0xff00A6A2); // keyboard menu labels background (on almost black)
    colors.nc_darker_blue  = colorf_from_rgb(0xff002080); // darker blue
    colors.nc_almost_black = colorf_from_rgb(0xff05101d); // keyboard menu text (on teal)
    // "Google Android Dark Theme"
    colors.dk_light_blue   = colorf_from_rgb(0xff87B0F3);
    colors.dk_dark_blue    = colorf_from_rgb(0xff2A364B);
    colors.dk_light_gray   = colorf_from_rgb(0xffb9b9b9);
    colors.dk_dark_gray    = colorf_from_rgb(0xff4d4d4d);
    colors.dk_gray_text    = colorf_from_rgb(0xffb3b3b3);
}

END_C
