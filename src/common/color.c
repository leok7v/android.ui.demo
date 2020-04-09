#include "color.h"
#include "app.h"

BEGIN_C

colors_t colors;

static_init(color) {
//  colors.color_transparent = {0.0f, 0.0f, 0.0f, 0.0f}; // already initialized by zeros
    colors.black           = gl_rgb2colorf(0x000000);
    colors.white           = gl_rgb2colorf(0xFFFFFF);
    colors.red             = gl_rgb2colorf(0xFF0000);
    colors.green           = gl_rgb2colorf(0x00FF00);
    colors.blue            = gl_rgb2colorf(0x0000FF);
    colors.orange          = gl_rgb2colorf(0xEE7600); // alt. 0xCC5500
    // "Norton Commander" palette see: https://photos.app->goo.gl/zbjPiiZkcr628BcI2
    colors.nc_dark_blue    = gl_rgb2colorf(0x0002A8); // background color
    colors.nc_light_blue   = gl_rgb2colorf(0x67F7FD); // light blue main text color
    colors.nc_dirty_gold   = gl_rgb2colorf(0xF0FFAF); // selection and labels color
    colors.nc_light_grey   = gl_rgb2colorf(0xAAABA9); // keyboard menu color (on black)
    colors.nc_teal         = gl_rgb2colorf(0x00A6A2); // keyboard menu labels background (on almost black)
    colors.nc_darker_blue  = gl_rgb2colorf(0x002080); // darker blue
    colors.nc_almost_black = gl_rgb2colorf(0x05101d); // keyboard menu text (on teal)
}

END_C
