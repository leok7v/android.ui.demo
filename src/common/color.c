#include "color.h"
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
    colors.black           = colorf_from_rgb(0xFF000000);
    colors.white           = colorf_from_rgb(0xFFFFFFFF);
    colors.red             = colorf_from_rgb(0xFFFF0000);
    colors.green           = colorf_from_rgb(0xFF00FF00);
    colors.blue            = colorf_from_rgb(0xFF0000FF);
    colors.orange          = colorf_from_rgb(0xFFEE7600); // alt. 0xCC5500
    // "Norton Commander" palette see: https://photos.app->goo.gl/zbjPiiZkcr628BcI2
    colors.nc_dark_blue    = colorf_from_rgb(0xFF0002A8); // background color
    colors.nc_light_blue   = colorf_from_rgb(0xFF67F7FD); // light blue main text color
    colors.nc_dirty_gold   = colorf_from_rgb(0xFFF0FFAF); // selection and labels color
    colors.nc_light_grey   = colorf_from_rgb(0xFFAAABA9); // keyboard menu color (on black)
    colors.nc_teal         = colorf_from_rgb(0xFF00A6A2); // keyboard menu labels background (on almost black)
    colors.nc_darker_blue  = colorf_from_rgb(0xFF002080); // darker blue
    colors.nc_almost_black = colorf_from_rgb(0xFF05101d); // keyboard menu text (on teal)
}

END_C
