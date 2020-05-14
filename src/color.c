#include "color.h"
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
#include "app.h"

begin_c

colorf_t colorf_from_rgb(uint32_t argb) {
    colorf_t f = { ((argb >> 16) & 0xFF) / 255.0f, ((argb >>  8) & 0xFF) / 255.0f,
                   ((argb >>  0) & 0xFF) / 255.0f, ((argb >> 24) & 0xFF) / 255.0f };
    return f;
}

static colorf_t transparent               = { 0.0000000, 0.0000000, 0.0000000, 0.0000000 };
static colorf_t black                     = { 0.0000000, 0.0000000, 0.0000000, 1.0000000 };
static colorf_t white                     = { 1.0000000, 1.0000000, 1.0000000, 1.0000000 };
static colorf_t red                       = { 1.0000000, 0.0000000, 0.0000000, 1.0000000 };
static colorf_t lime                      = { 0.0000000, 1.0000000, 0.0000000, 1.0000000 };
static colorf_t blue                      = { 0.0000000, 0.0000000, 1.0000000, 1.0000000 };
static colorf_t yellow                    = { 1.0000000, 1.0000000, 0.0000000, 1.0000000 };
static colorf_t cyan                      = { 0.0000000, 1.0000000, 1.0000000, 1.0000000 };
static colorf_t aqua                      = { 0.0000000, 1.0000000, 1.0000000, 1.0000000 };
static colorf_t magenta                   = { 1.0000000, 0.0000000, 1.0000000, 1.0000000 };
static colorf_t fuchsia                   = { 1.0000000, 0.0000000, 1.0000000, 1.0000000 };
static colorf_t silver                    = { 0.7529412, 0.7529412, 0.7529412, 1.0000000 };
static colorf_t gray                      = { 0.5019608, 0.5019608, 0.5019608, 1.0000000 };
static colorf_t maroon                    = { 0.5019608, 0.0000000, 0.0000000, 1.0000000 };
static colorf_t olive                     = { 0.5019608, 0.5019608, 0.0000000, 1.0000000 };
static colorf_t green                     = { 0.0000000, 0.5019608, 0.0000000, 1.0000000 };
static colorf_t purple                    = { 0.5019608, 0.0000000, 0.5019608, 1.0000000 };
static colorf_t teal                      = { 0.0000000, 0.5019608, 0.5019608, 1.0000000 };
static colorf_t navy                      = { 0.0000000, 0.0000000, 0.5019608, 1.0000000 };
static colorf_t dark_red                  = { 0.5450981, 0.0000000, 0.0000000, 1.0000000 };
static colorf_t brown                     = { 0.6470588, 0.1647059, 0.1647059, 1.0000000 };
static colorf_t firebrick                 = { 0.6980392, 0.1333333, 0.1333333, 1.0000000 };
static colorf_t crimson                   = { 0.8627451, 0.0784314, 0.2352941, 1.0000000 };
static colorf_t tomato                    = { 1.0000000, 0.3882353, 0.2784314, 1.0000000 };
static colorf_t coral                     = { 1.0000000, 0.4980392, 0.3137255, 1.0000000 };
static colorf_t indian_red                = { 0.8039216, 0.3607843, 0.3607843, 1.0000000 };
static colorf_t light_coral               = { 0.9411765, 0.5019608, 0.5019608, 1.0000000 };
static colorf_t dark_salmon               = { 0.9137255, 0.5882353, 0.4784314, 1.0000000 };
static colorf_t salmon                    = { 0.9803922, 0.5019608, 0.4470588, 1.0000000 };
static colorf_t light_salmon              = { 1.0000000, 0.6274510, 0.4784314, 1.0000000 };
static colorf_t orange_red                = { 1.0000000, 0.2705882, 0.0000000, 1.0000000 };
static colorf_t dark_orange               = { 1.0000000, 0.5490196, 0.0000000, 1.0000000 };
static colorf_t orange                    = { 1.0000000, 0.6470588, 0.0000000, 1.0000000 };
static colorf_t gold                      = { 1.0000000, 0.8431373, 0.0000000, 1.0000000 };
static colorf_t dark_golden_rod           = { 0.7215686, 0.5254902, 0.0431373, 1.0000000 };
static colorf_t golden_rod                = { 0.8549020, 0.6470588, 0.1254902, 1.0000000 };
static colorf_t pale_golden_rod           = { 0.9333333, 0.9098039, 0.6666667, 1.0000000 };
static colorf_t dark_khaki                = { 0.7411765, 0.7176471, 0.4196078, 1.0000000 };
static colorf_t khaki                     = { 0.9411765, 0.9019608, 0.5490196, 1.0000000 };
static colorf_t yellow_green              = { 0.6039216, 0.8039216, 0.1960784, 1.0000000 };
static colorf_t dark_olive_green          = { 0.3333333, 0.4196078, 0.1843137, 1.0000000 };
static colorf_t olive_drab                = { 0.4196078, 0.5568628, 0.1372549, 1.0000000 };
static colorf_t lawn_green                = { 0.4862745, 0.9882353, 0.0000000, 1.0000000 };
static colorf_t chart_reuse               = { 0.4980392, 1.0000000, 0.0000000, 1.0000000 };
static colorf_t green_yellow              = { 0.6784314, 1.0000000, 0.1843137, 1.0000000 };
static colorf_t dark_green                = { 0.0000000, 0.3921569, 0.0000000, 1.0000000 };
static colorf_t forest_green              = { 0.1333333, 0.5450981, 0.1333333, 1.0000000 };
static colorf_t lime_green                = { 0.1960784, 0.8039216, 0.1960784, 1.0000000 };
static colorf_t light_green               = { 0.5647059, 0.9333333, 0.5647059, 1.0000000 };
static colorf_t pale_green                = { 0.5960785, 0.9843137, 0.5960785, 1.0000000 };
static colorf_t dark_sea_green            = { 0.5607843, 0.7372549, 0.5607843, 1.0000000 };
static colorf_t medium_spring_green       = { 0.0000000, 0.9803922, 0.6039216, 1.0000000 };
static colorf_t spring_green              = { 0.0000000, 1.0000000, 0.4980392, 1.0000000 };
static colorf_t sea_green                 = { 0.1803922, 0.5450981, 0.3411765, 1.0000000 };
static colorf_t medium_aqua_marine        = { 0.4000000, 0.8039216, 0.6666667, 1.0000000 };
static colorf_t medium_sea_green          = { 0.2352941, 0.7019608, 0.4431373, 1.0000000 };
static colorf_t light_sea_green           = { 0.1254902, 0.6980392, 0.6666667, 1.0000000 };
static colorf_t dark_slate_gray           = { 0.1843137, 0.3098039, 0.3098039, 1.0000000 };
static colorf_t dark_cyan                 = { 0.0000000, 0.5450981, 0.5450981, 1.0000000 };
static colorf_t light_cyan                = { 0.8784314, 1.0000000, 1.0000000, 1.0000000 };
static colorf_t dark_turquoise            = { 0.0000000, 0.8078431, 0.8196079, 1.0000000 };
static colorf_t turquoise                 = { 0.2509804, 0.8784314, 0.8156863, 1.0000000 };
static colorf_t medium_turquoise          = { 0.2823530, 0.8196079, 0.8000000, 1.0000000 };
static colorf_t pale_turquoise            = { 0.6862745, 0.9333333, 0.9333333, 1.0000000 };
static colorf_t aqua_marine               = { 0.4980392, 1.0000000, 0.8313726, 1.0000000 };
static colorf_t powder_blue               = { 0.6901961, 0.8784314, 0.9019608, 1.0000000 };
static colorf_t cadet_blue                = { 0.3725490, 0.6196079, 0.6274510, 1.0000000 };
static colorf_t steel_blue                = { 0.2745098, 0.5098040, 0.7058824, 1.0000000 };
static colorf_t corn_flower_blue          = { 0.3921569, 0.5843138, 0.9294118, 1.0000000 };
static colorf_t deep_sky_blue             = { 0.0000000, 0.7490196, 1.0000000, 1.0000000 };
static colorf_t dodger_blue               = { 0.1176471, 0.5647059, 1.0000000, 1.0000000 };
static colorf_t light_blue                = { 0.6784314, 0.8470588, 0.9019608, 1.0000000 };
static colorf_t sky_blue                  = { 0.5294118, 0.8078431, 0.9215686, 1.0000000 };
static colorf_t light_sky_blue            = { 0.5294118, 0.8078431, 0.9803922, 1.0000000 };
static colorf_t midnight_blue             = { 0.0980392, 0.0980392, 0.4392157, 1.0000000 };
static colorf_t dark_blue                 = { 0.0000000, 0.0000000, 0.5450981, 1.0000000 };
static colorf_t medium_blue               = { 0.0000000, 0.0000000, 0.8039216, 1.0000000 };
static colorf_t royal_blue                = { 0.2549020, 0.4117647, 0.8823529, 1.0000000 };
static colorf_t blue_violet               = { 0.5411765, 0.1686275, 0.8862745, 1.0000000 };
static colorf_t indigo                    = { 0.2941177, 0.0000000, 0.5098040, 1.0000000 };
static colorf_t dark_slate_blue           = { 0.2823530, 0.2392157, 0.5450981, 1.0000000 };
static colorf_t slate_blue                = { 0.4156863, 0.3529412, 0.8039216, 1.0000000 };
static colorf_t medium_slate_blue         = { 0.4823529, 0.4078431, 0.9333333, 1.0000000 };
static colorf_t medium_purple             = { 0.5764706, 0.4392157, 0.8588235, 1.0000000 };
static colorf_t dark_magenta              = { 0.5450981, 0.0000000, 0.5450981, 1.0000000 };
static colorf_t dark_violet               = { 0.5803922, 0.0000000, 0.8274510, 1.0000000 };
static colorf_t dark_orchid               = { 0.6000000, 0.1960784, 0.8000000, 1.0000000 };
static colorf_t medium_orchid             = { 0.7294118, 0.3333333, 0.8274510, 1.0000000 };
static colorf_t thistle                   = { 0.8470588, 0.7490196, 0.8470588, 1.0000000 };
static colorf_t plum                      = { 0.8666667, 0.6274510, 0.8666667, 1.0000000 };
static colorf_t violet                    = { 0.9333333, 0.5098040, 0.9333333, 1.0000000 };
static colorf_t orchid                    = { 0.8549020, 0.4392157, 0.8392157, 1.0000000 };
static colorf_t medium_violet_red         = { 0.7803922, 0.0823529, 0.5215687, 1.0000000 };
static colorf_t pale_violet_red           = { 0.8588235, 0.4392157, 0.5764706, 1.0000000 };
static colorf_t deep_pink                 = { 1.0000000, 0.0784314, 0.5764706, 1.0000000 };
static colorf_t hot_pink                  = { 1.0000000, 0.4117647, 0.7058824, 1.0000000 };
static colorf_t light_pink                = { 1.0000000, 0.7137255, 0.7568628, 1.0000000 };
static colorf_t pink                      = { 1.0000000, 0.7529412, 0.7960784, 1.0000000 };
static colorf_t antique_white             = { 0.9803922, 0.9215686, 0.8431373, 1.0000000 };
static colorf_t beige                     = { 0.9607843, 0.9607843, 0.8627451, 1.0000000 };
static colorf_t bisque                    = { 1.0000000, 0.8941177, 0.7686275, 1.0000000 };
static colorf_t blanched_almond           = { 1.0000000, 0.9215686, 0.8039216, 1.0000000 };
static colorf_t wheat                     = { 0.9607843, 0.8705882, 0.7019608, 1.0000000 };
static colorf_t corn_silk                 = { 1.0000000, 0.9725490, 0.8627451, 1.0000000 };
static colorf_t lemon_chiffon             = { 1.0000000, 0.9803922, 0.8039216, 1.0000000 };
static colorf_t light_golden_rod_yellow   = { 0.9803922, 0.9803922, 0.8235294, 1.0000000 };
static colorf_t light_yellow              = { 1.0000000, 1.0000000, 0.8784314, 1.0000000 };
static colorf_t saddle_brown              = { 0.5450981, 0.2705882, 0.0745098, 1.0000000 };
static colorf_t sienna                    = { 0.6274510, 0.3215686, 0.1764706, 1.0000000 };
static colorf_t chocolate                 = { 0.8235294, 0.4117647, 0.1176471, 1.0000000 };
static colorf_t peru                      = { 0.8039216, 0.5215687, 0.2470588, 1.0000000 };
static colorf_t sandy_brown               = { 0.9568627, 0.6431373, 0.3764706, 1.0000000 };
static colorf_t burly_wood                = { 0.8705882, 0.7215686, 0.5294118, 1.0000000 };
static colorf_t tannum                    = { 0.8235294, 0.7058824, 0.5490196, 1.0000000 };
static colorf_t rosy_brown                = { 0.7372549, 0.5607843, 0.5607843, 1.0000000 };
static colorf_t moccasin                  = { 1.0000000, 0.8941177, 0.7098039, 1.0000000 };
static colorf_t navajo_white              = { 1.0000000, 0.8705882, 0.6784314, 1.0000000 };
static colorf_t peach_puff                = { 1.0000000, 0.8549020, 0.7254902, 1.0000000 };
static colorf_t misty_rose                = { 1.0000000, 0.8941177, 0.8823529, 1.0000000 };
static colorf_t lavender_blush            = { 1.0000000, 0.9411765, 0.9607843, 1.0000000 };
static colorf_t linen                     = { 0.9803922, 0.9411765, 0.9019608, 1.0000000 };
static colorf_t old_lace                  = { 0.9921569, 0.9607843, 0.9019608, 1.0000000 };
static colorf_t papaya_whip               = { 1.0000000, 0.9372549, 0.8352941, 1.0000000 };
static colorf_t sea_shell                 = { 1.0000000, 0.9607843, 0.9333333, 1.0000000 };
static colorf_t mint_cream                = { 0.9607843, 1.0000000, 0.9803922, 1.0000000 };
static colorf_t slate_gray                = { 0.4392157, 0.5019608, 0.5647059, 1.0000000 };
static colorf_t light_slate_gray          = { 0.4666667, 0.5333334, 0.6000000, 1.0000000 };
static colorf_t light_steel_blue          = { 0.6901961, 0.7686275, 0.8705882, 1.0000000 };
static colorf_t lavender                  = { 0.9019608, 0.9019608, 0.9803922, 1.0000000 };
static colorf_t floral_white              = { 1.0000000, 0.9803922, 0.9411765, 1.0000000 };
static colorf_t alice_blue                = { 0.9411765, 0.9725490, 1.0000000, 1.0000000 };
static colorf_t ghost_white               = { 0.9725490, 0.9725490, 1.0000000, 1.0000000 };
static colorf_t honeydew                  = { 0.9411765, 1.0000000, 0.9411765, 1.0000000 };
static colorf_t ivory                     = { 1.0000000, 1.0000000, 0.9411765, 1.0000000 };
static colorf_t azure                     = { 0.9411765, 1.0000000, 1.0000000, 1.0000000 };
static colorf_t snow                      = { 1.0000000, 0.9803922, 0.9803922, 1.0000000 };
static colorf_t dim_gray                  = { 0.4117647, 0.4117647, 0.4117647, 1.0000000 };
static colorf_t dim_grey                  = { 0.4117647, 0.4117647, 0.4117647, 1.0000000 };
static colorf_t grey                      = { 0.5019608, 0.5019608, 0.5019608, 1.0000000 };
static colorf_t dark_gray                 = { 0.6627451, 0.6627451, 0.6627451, 1.0000000 };
static colorf_t dark_grey                 = { 0.6627451, 0.6627451, 0.6627451, 1.0000000 };
static colorf_t light_gray                = { 0.8274510, 0.8274510, 0.8274510, 1.0000000 };
static colorf_t light_grey                = { 0.8274510, 0.8274510, 0.8274510, 1.0000000 };
static colorf_t gainsboro                 = { 0.8627451, 0.8627451, 0.8627451, 1.0000000 };
static colorf_t white_smoke               = { 0.9607843, 0.9607843, 0.9607843, 1.0000000 };

const colors_t colors = {
    &transparent,
    &black,
    &white,
    &red,
    &lime,
    &blue,
    &yellow,
    &cyan,
    &aqua,
    &magenta,
    &fuchsia,
    &silver,
    &gray,
    &maroon,
    &olive,
    &green,
    &purple,
    &teal,
    &navy,
    &dark_red,
    &brown,
    &firebrick,
    &crimson,
    &tomato,
    &coral,
    &indian_red,
    &light_coral,
    &dark_salmon,
    &salmon,
    &light_salmon,
    &orange_red,
    &dark_orange,
    &orange,
    &gold,
    &dark_golden_rod,
    &golden_rod,
    &pale_golden_rod,
    &dark_khaki,
    &khaki,
    &yellow_green,
    &dark_olive_green,
    &olive_drab,
    &lawn_green,
    &chart_reuse,
    &green_yellow,
    &dark_green,
    &forest_green,
    &lime_green,
    &light_green,
    &pale_green,
    &dark_sea_green,
    &medium_spring_green,
    &spring_green,
    &sea_green,
    &medium_aqua_marine,
    &medium_sea_green,
    &light_sea_green,
    &dark_slate_gray,
    &dark_cyan,
    &light_cyan,
    &dark_turquoise,
    &turquoise,
    &medium_turquoise,
    &pale_turquoise,
    &aqua_marine,
    &powder_blue,
    &cadet_blue,
    &steel_blue,
    &corn_flower_blue,
    &deep_sky_blue,
    &dodger_blue,
    &light_blue,
    &sky_blue,
    &light_sky_blue,
    &midnight_blue,
    &dark_blue,
    &medium_blue,
    &royal_blue,
    &blue_violet,
    &indigo,
    &dark_slate_blue,
    &slate_blue,
    &medium_slate_blue,
    &medium_purple,
    &dark_magenta,
    &dark_violet,
    &dark_orchid,
    &medium_orchid,
    &thistle,
    &plum,
    &violet,
    &orchid,
    &medium_violet_red,
    &pale_violet_red,
    &deep_pink,
    &hot_pink,
    &light_pink,
    &pink,
    &antique_white,
    &beige,
    &bisque,
    &blanched_almond,
    &wheat,
    &corn_silk,
    &lemon_chiffon,
    &light_golden_rod_yellow,
    &light_yellow,
    &saddle_brown,
    &sienna,
    &chocolate,
    &peru,
    &sandy_brown,
    &burly_wood,
    &tannum,
    &rosy_brown,
    &moccasin,
    &navajo_white,
    &peach_puff,
    &misty_rose,
    &lavender_blush,
    &linen,
    &old_lace,
    &papaya_whip,
    &sea_shell,
    &mint_cream,
    &slate_gray,
    &light_slate_gray,
    &light_steel_blue,
    &lavender,
    &floral_white,
    &alice_blue,
    &ghost_white,
    &honeydew,
    &ivory,
    &azure,
    &snow,
    &dim_gray,
    &dim_grey,
    &grey,
    &dark_gray,
    &dark_grey,
    &light_gray,
    &light_grey,
    &gainsboro,
    &white_smoke
};

static const color_info_t color_names_table[] = {
    { "transparent",               &transparent,               0xFF000000 },
    { "black",                     &black,                     0xFF000000 },
    { "white",                     &white,                     0xFFFFFFFF },
    { "red",                       &red,                       0xFFFF0000 },
    { "lime",                      &lime,                      0xFF00FF00 },
    { "blue",                      &blue,                      0xFF0000FF },
    { "yellow",                    &yellow,                    0xFFFFFF00 },
    { "cyan",                      &cyan,                      0xFF00FFFF },
    { "aqua",                      &aqua,                      0xFF00FFFF },
    { "magenta",                   &magenta,                   0xFFFF00FF },
    { "fuchsia",                   &fuchsia,                   0xFFFF00FF },
    { "silver",                    &silver,                    0xFFC0C0C0 },
    { "gray",                      &gray,                      0xFF808080 },
    { "maroon",                    &maroon,                    0xFF800000 },
    { "olive",                     &olive,                     0xFF808000 },
    { "green",                     &green,                     0xFF008000 },
    { "purple",                    &purple,                    0xFF800080 },
    { "teal",                      &teal,                      0xFF008080 },
    { "navy",                      &navy,                      0xFF000080 },
    { "dark red",                  &dark_red,                  0xFF8B0000 },
    { "brown",                     &brown,                     0xFFA52A2A },
    { "firebrick",                 &firebrick,                 0xFFB22222 },
    { "crimson",                   &crimson,                   0xFFDC143C },
    { "tomato",                    &tomato,                    0xFFFF6347 },
    { "coral",                     &coral,                     0xFFFF7F50 },
    { "indian red",                &indian_red,                0xFFCD5C5C },
    { "light coral",               &light_coral,               0xFFF08080 },
    { "dark salmon",               &dark_salmon,               0xFFE9967A },
    { "salmon",                    &salmon,                    0xFFFA8072 },
    { "light salmon",              &light_salmon,              0xFFFFA07A },
    { "orange red",                &orange_red,                0xFFFF4500 },
    { "dark orange",               &dark_orange,               0xFFFF8C00 },
    { "orange",                    &orange,                    0xFFFFA500 },
    { "gold",                      &gold,                      0xFFFFD700 },
    { "dark golden rod",           &dark_golden_rod,           0xFFB8860B },
    { "golden rod",                &golden_rod,                0xFFDAA520 },
    { "pale golden rod",           &pale_golden_rod,           0xFFEEE8AA },
    { "dark khaki",                &dark_khaki,                0xFFBDB76B },
    { "khaki",                     &khaki,                     0xFFF0E68C },
    { "yellow green",              &yellow_green,              0xFF9ACD32 },
    { "dark olive green",          &dark_olive_green,          0xFF556B2F },
    { "olive drab",                &olive_drab,                0xFF6B8E23 },
    { "lawn green",                &lawn_green,                0xFF7CFC00 },
    { "chart reuse",               &chart_reuse,               0xFF7FFF00 },
    { "green yellow",              &green_yellow,              0xFFADFF2F },
    { "dark green",                &dark_green,                0xFF006400 },
    { "forest green",              &forest_green,              0xFF228B22 },
    { "lime green",                &lime_green,                0xFF32CD32 },
    { "light green",               &light_green,               0xFF90EE90 },
    { "pale green",                &pale_green,                0xFF98FB98 },
    { "dark sea green",            &dark_sea_green,            0xFF8FBC8F },
    { "medium spring green",       &medium_spring_green,       0xFF00FA9A },
    { "spring green",              &spring_green,              0xFF00FF7F },
    { "sea green",                 &sea_green,                 0xFF2E8B57 },
    { "medium aqua marine",        &medium_aqua_marine,        0xFF66CDAA },
    { "medium sea green",          &medium_sea_green,          0xFF3CB371 },
    { "light sea green",           &light_sea_green,           0xFF20B2AA },
    { "dark slate gray",           &dark_slate_gray,           0xFF2F4F4F },
    { "dark cyan",                 &dark_cyan,                 0xFF008B8B },
    { "light cyan",                &light_cyan,                0xFFE0FFFF },
    { "dark turquoise",            &dark_turquoise,            0xFF00CED1 },
    { "turquoise",                 &turquoise,                 0xFF40E0D0 },
    { "medium turquoise",          &medium_turquoise,          0xFF48D1CC },
    { "pale turquoise",            &pale_turquoise,            0xFFAFEEEE },
    { "aqua marine",               &aqua_marine,               0xFF7FFFD4 },
    { "powder blue",               &powder_blue,               0xFFB0E0E6 },
    { "cadet blue",                &cadet_blue,                0xFF5F9EA0 },
    { "steel blue",                &steel_blue,                0xFF4682B4 },
    { "corn flower blue",          &corn_flower_blue,          0xFF6495ED },
    { "deep sky blue",             &deep_sky_blue,             0xFF00BFFF },
    { "dodger blue",               &dodger_blue,               0xFF1E90FF },
    { "light blue",                &light_blue,                0xFFADD8E6 },
    { "sky blue",                  &sky_blue,                  0xFF87CEEB },
    { "light sky blue",            &light_sky_blue,            0xFF87CEFA },
    { "midnight blue",             &midnight_blue,             0xFF191970 },
    { "dark blue",                 &dark_blue,                 0xFF00008B },
    { "medium blue",               &medium_blue,               0xFF0000CD },
    { "royal blue",                &royal_blue,                0xFF4169E1 },
    { "blue violet",               &blue_violet,               0xFF8A2BE2 },
    { "indigo",                    &indigo,                    0xFF4B0082 },
    { "dark slate blue",           &dark_slate_blue,           0xFF483D8B },
    { "slate blue",                &slate_blue,                0xFF6A5ACD },
    { "medium slate blue",         &medium_slate_blue,         0xFF7B68EE },
    { "medium purple",             &medium_purple,             0xFF9370DB },
    { "dark magenta",              &dark_magenta,              0xFF8B008B },
    { "dark violet",               &dark_violet,               0xFF9400D3 },
    { "dark orchid",               &dark_orchid,               0xFF9932CC },
    { "medium orchid",             &medium_orchid,             0xFFBA55D3 },
    { "thistle",                   &thistle,                   0xFFD8BFD8 },
    { "plum",                      &plum,                      0xFFDDA0DD },
    { "violet",                    &violet,                    0xFFEE82EE },
    { "orchid",                    &orchid,                    0xFFDA70D6 },
    { "medium violet red",         &medium_violet_red,         0xFFC71585 },
    { "pale violet red",           &pale_violet_red,           0xFFDB7093 },
    { "deep pink",                 &deep_pink,                 0xFFFF1493 },
    { "hot pink",                  &hot_pink,                  0xFFFF69B4 },
    { "light pink",                &light_pink,                0xFFFFB6C1 },
    { "pink",                      &pink,                      0xFFFFC0CB },
    { "antique white",             &antique_white,             0xFFFAEBD7 },
    { "beige",                     &beige,                     0xFFF5F5DC },
    { "bisque",                    &bisque,                    0xFFFFE4C4 },
    { "blanched almond",           &blanched_almond,           0xFFFFEBCD },
    { "wheat",                     &wheat,                     0xFFF5DEB3 },
    { "corn silk",                 &corn_silk,                 0xFFFFF8DC },
    { "lemon chiffon",             &lemon_chiffon,             0xFFFFFACD },
    { "light golden rod yellow",   &light_golden_rod_yellow,   0xFFFAFAD2 },
    { "light yellow",              &light_yellow,              0xFFFFFFE0 },
    { "saddle brown",              &saddle_brown,              0xFF8B4513 },
    { "sienna",                    &sienna,                    0xFFA0522D },
    { "chocolate",                 &chocolate,                 0xFFD2691E },
    { "peru",                      &peru,                      0xFFCD853F },
    { "sandy brown",               &sandy_brown,               0xFFF4A460 },
    { "burly wood",                &burly_wood,                0xFFDEB887 },
    { "tan",                       &tannum,                    0xFFD2B48C },
    { "rosy brown",                &rosy_brown,                0xFFBC8F8F },
    { "moccasin",                  &moccasin,                  0xFFFFE4B5 },
    { "navajo white",              &navajo_white,              0xFFFFDEAD },
    { "peach puff",                &peach_puff,                0xFFFFDAB9 },
    { "misty rose",                &misty_rose,                0xFFFFE4E1 },
    { "lavender blush",            &lavender_blush,            0xFFFFF0F5 },
    { "linen",                     &linen,                     0xFFFAF0E6 },
    { "old lace",                  &old_lace,                  0xFFFDF5E6 },
    { "papaya whip",               &papaya_whip,               0xFFFFEFD5 },
    { "sea shell",                 &sea_shell,                 0xFFFFF5EE },
    { "mint cream",                &mint_cream,                0xFFF5FFFA },
    { "slate gray",                &slate_gray,                0xFF708090 },
    { "light slate gray",          &light_slate_gray,          0xFF778899 },
    { "light steel blue",          &light_steel_blue,          0xFFB0C4DE },
    { "lavender",                  &lavender,                  0xFFE6E6FA },
    { "floral white",              &floral_white,              0xFFFFFAF0 },
    { "alice blue",                &alice_blue,                0xFFF0F8FF },
    { "ghost white",               &ghost_white,               0xFFF8F8FF },
    { "honeydew",                  &honeydew,                  0xFFF0FFF0 },
    { "ivory",                     &ivory,                     0xFFFFFFF0 },
    { "azure",                     &azure,                     0xFFF0FFFF },
    { "snow",                      &snow,                      0xFFFFFAFA },
    { "dim gray",                  &dim_gray,                  0xFF696969 },
    { "dim grey",                  &dim_grey,                  0xFF696969 },
    { "grey",                      &grey,                      0xFF808080 },
    { "dark gray",                 &dark_gray,                 0xFFA9A9A9 },
    { "dark grey",                 &dark_grey,                 0xFFA9A9A9 },
    { "light gray",                &light_gray,                0xFFD3D3D3 },
    { "light grey",                &light_grey,                0xFFD3D3D3 },
    { "gainsboro",                 &gainsboro,                 0xFFDCDCDC },
    { "white smoke",               &white_smoke,               0xFFF5F5F5 }
};

const color_info_t* color_names = color_names_table;
const int color_names_count = countof(color_names_table);

static const colorf_t dk_light_blue = { 0.5294118, 0.6901961, 0.9529412, 1.0000000 };
static const colorf_t dk_dark_blue  = { 0.1647059, 0.2117647, 0.2941177, 1.0000000 };
static const colorf_t dk_light_gray = { 0.7254902, 0.7254902, 0.7254902, 1.0000000 };
static const colorf_t dk_dark_gray  = { 0.3019608, 0.3019608, 0.3019608, 1.0000000 };
static const colorf_t dk_gray_text  = { 0.7019608, 0.7019608, 0.7019608, 1.0000000 };

const colors_dk_t colors_dk = {
    &dk_light_blue,
    &dk_dark_blue,
    &dk_light_gray,
    &dk_dark_gray,
    &dk_gray_text
};

static const color_info_t color_dk_names_table[] = {
    { "light_blue", &dk_light_blue, 0xff87B0F3 },
    { "dark_blue",  &dk_dark_blue,  0xff2A364B },
    { "light_gray", &dk_light_gray, 0xffb9b9b9 },
    { "dark_gray ", &dk_dark_gray,  0xff4d4d4d },
    { "gray_text",  &dk_gray_text,  0xffb3b3b3 },
};

const color_info_t* color_dk_names = color_dk_names_table;
const int color_dk_names_count = countof(color_dk_names_table);

static const colorf_t nc_dark_blue    = { 0.0000000, 0.0078431, 0.6588235, 1.0000000 };
static const colorf_t nc_light_blue   = { 0.4039216, 0.9686275, 0.9921569, 1.0000000 };
static const colorf_t nc_dirty_gold   = { 0.9411765, 1.0000000, 0.6862745, 1.0000000 };
static const colorf_t nc_light_gray   = { 0.6666667, 0.6705883, 0.6627451, 1.0000000 };
static const colorf_t nc_teal         = { 0.0000000, 0.6509804, 0.6352941, 1.0000000 };
static const colorf_t nc_darker_blue  = { 0.0000000, 0.1254902, 0.5019608, 1.0000000 };
static const colorf_t nc_almost_black = { 0.0196078, 0.0627451, 0.1137255, 1.0000000 };

const colors_nc_t colors_nc = {
    &nc_dark_blue,
    &nc_light_blue,
    &nc_dirty_gold,
    &nc_light_gray,
    &nc_teal,
    &nc_darker_blue,
    &nc_almost_black
};

static const color_info_t color_nc_names_table[] = {
    { "dark_blue",                   &nc_dark_blue,                0xff0002A8 },
    { "light_blue",                  &nc_light_blue,               0xff67F7FD },
    { "dirty_gold",                  &nc_dirty_gold,               0xffF0FFAF },
    { "light_gray",                  &nc_light_gray,               0xffAAABA9 },
    { "teal",                        &nc_teal,                     0xff00A6A2 },
    { "darker_blue",                 &nc_darker_blue,              0xff002080 },
    { "almost_black",                &nc_almost_black,             0xff05101d },
};

const color_info_t* color_nc_names = color_nc_names_table;
const int color_nc_names_count = countof(color_names_table);

end_c
