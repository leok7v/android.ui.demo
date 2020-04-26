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

typedef struct color_info_s { // treat all fields as a const
    const char* name;
    const colorf_t* color;
    uint32_t rgba;
} color_info_t;

colorf_t colorf_from_rgb(uint32_t argb);

typedef struct colors_s { // treat all fields as a const
    const colorf_t
    *transparent,
    *black,
    *white,
    *red,
    *lime,
    *blue,
    *yellow,
    *cyan,
    *aqua,
    *magenta,
    *fuchsia,
    *silver,
    *gray,
    *maroon,
    *olive,
    *green,
    *purple,
    *teal,
    *navy,
    *dark_red,
    *brown,
    *firebrick,
    *crimson,
    *tomato,
    *coral,
    *indian_red,
    *light_coral,
    *dark_salmon,
    *salmon,
    *light_salmon,
    *orange_red,
    *dark_orange,
    *orange,
    *gold,
    *dark_golden_rod,
    *golden_rod,
    *pale_golden_rod,
    *dark_khaki,
    *khaki,
    *yellow_green,
    *dark_olive_green,
    *olive_drab,
    *lawn_green,
    *chart_reuse,
    *green_yellow,
    *dark_green,
    *forest_green,
    *lime_green,
    *light_green,
    *pale_green,
    *dark_sea_green,
    *medium_spring_green,
    *spring_green,
    *sea_green,
    *medium_aqua_marine,
    *medium_sea_green,
    *light_sea_green,
    *dark_slate_gray,
    *dark_cyan,
    *light_cyan,
    *dark_turquoise,
    *turquoise,
    *medium_turquoise,
    *pale_turquoise,
    *aqua_marine,
    *powder_blue,
    *cadet_blue,
    *steel_blue,
    *corn_flower_blue,
    *deep_sky_blue,
    *dodger_blue,
    *light_blue,
    *sky_blue,
    *light_sky_blue,
    *midnight_blue,
    *dark_blue,
    *medium_blue,
    *royal_blue,
    *blue_violet,
    *indigo,
    *dark_slate_blue,
    *slate_blue,
    *medium_slate_blue,
    *medium_purple,
    *dark_magenta,
    *dark_violet,
    *dark_orchid,
    *medium_orchid,
    *thistle,
    *plum,
    *violet,
    *orchid,
    *medium_violet_red,
    *pale_violet_red,
    *deep_pink,
    *hot_pink,
    *light_pink,
    *pink,
    *antique_white,
    *beige,
    *bisque,
    *blanched_almond,
    *wheat,
    *corn_silk,
    *lemon_chiffon,
    *light_golden_rod_yellow,
    *light_yellow,
    *saddle_brown,
    *sienna,
    *chocolate,
    *peru,
    *sandy_brown,
    *burly_wood,
    *tannum,
    *rosy_brown,
    *moccasin,
    *navajo_white,
    *peach_puff,
    *misty_rose,
    *lavender_blush,
    *linen,
    *old_lace,
    *papaya_whip,
    *sea_shell,
    *mint_cream,
    *slate_gray,
    *light_slate_gray,
    *light_steel_blue,
    *lavender,
    *floral_white,
    *alice_blue,
    *ghost_white,
    *honeydew,
    *ivory,
    *azure,
    *snow,
    *dim_gray,
    *dim_grey,
    *grey,
    *dark_gray,
    *dark_grey,
    *light_gray,
    *light_grey,
    *gainsboro,
    *white_smoke;
} colors_t;

extern const colors_t colors;
extern const color_info_t* color_names;
extern const int color_names_count;

typedef struct colors_dk_s { // treat all fields as a const
    const colorf_t
    * light_blue,
    * dark_blue,
    * light_gray,
    * dark_gray,
    * gray_text;
} colors_dk_t;

typedef struct colors_nc_s { // treat all fields as a const
    // "Norton Commander" palette see:
    // https://en.wikipedia.org/wiki/Norton_Commander#/media/File:Norton_Commander_5.51.png
    const colorf_t
    * dark_blue,    // background color
    * light_blue,   // light bluemain text color
    * dirty_gold,   // selection and labels color
    * light_gray,   // keyboard menu color (on black)
    * teal,         // keyboard menu labels background (on almost black)
    * darker_blue,  // darker blue
    * almost_black; // keyboard menu text (on teal)
} colors_nc_t;

extern const colors_dk_t colors_dk;
extern const color_info_t* color_dk_names;
extern const int color_dk_names_count;

extern const colors_nc_t colors_nc;
extern const color_info_t* color_nc_names;
extern const int color_nc_names_count;

END_C
