#include "app.h"
#include "dc.h"
#include "button.h"
#include "slider.h"
#include "screen_writer.h"
#include "stb_image.h"
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "shaders.h"

BEGIN_C

static const char* SLIDER1_LABEL = "Slider1: %-3d";
static const char* SLIDER2_LABEL = "Slider2: %-3d";

enum {
    FONT_HEIGHT_PT      =  12, // pt = point = 1/72 inch
    MIN_BUTTON_WIDTH_PT =  90,
    VERTICAL_GAP_PT     =   8,
    HORIZONTAL_GAP_PT   =   8
};

typedef struct application_s {
    app_t* a;
    int font_height_px;
    font_t font;    // default UI font
    ui_expo_t theme;
//  int program_main;
    char toast[256];
    int64_t toast_start_time;
    timer_callback_t toast_timer_callback;
    bitmap_t  bitmaps[3];
    button_t* quit;
    button_t* exit;
    button_t* checkbox;
    bool flip;
    slider_t* slider1;
    slider_t* slider2;
    ui_t* view_textures;
    ui_t* view_glyphs;
    ui_t* view_ascii;
    int  slider1_minimum;
    int  slider1_maximum;
    int  slider1_current;
    char slider1_label[64];
    int  slider2_minimum;
    int  slider2_maximum;
    int  slider2_current;
    char slider2_label[64];
    mat4x4 mvp; // model * view * projection
} application_t;

static inline_c float pt2px(app_t* a, float pt) { return pt * a->xdpi / 72.0f; }

static const uint64_t TOAST_TIME_IN_MS = 2750; // 2.75s

static void toast_timer_callback(timer_callback_t* timer_callback) {
    application_t* app = (application_t*)timer_callback->that;
    app->a->invalidate(app->a);
}

static void toast_add(application_t* app) {
    assert(app->toast_timer_callback.id == 0);
    app->toast_start_time = app->a->time_in_nanoseconds;
    app->toast_timer_callback.id = 0;
    app->toast_timer_callback.that = app;
    app->toast_timer_callback.ns = TOAST_TIME_IN_MS * NS_IN_MS / 10;
    app->toast_timer_callback.callback = toast_timer_callback;
    app->toast_timer_callback.last_fired = 0;
    app->a->timer_add(app->a, &app->toast_timer_callback);
}

static void toast_remove(application_t* app) {
    app->a->timer_remove(app->a, &app->toast_timer_callback);
    memset(&app->toast_timer_callback, 0, sizeof(app->toast_timer_callback));
    app->toast[0] = 0; // toast OFF
    app->toast_start_time = 0;
}

static void toast_draw(application_t* app) {
    app_t* a = app->a;
    if (app->toast[0] != 0) {
        if (app->toast_start_time == 0) {
            toast_add(app);
        }
        int64_t time_on_screen = a->time_in_nanoseconds - app->toast_start_time;
        if (time_on_screen / 1000 > TOAST_TIME_IN_MS * 1000) { // nanoseconds
            toast_remove(app);
        } else {
            float fh = app->font.height;
            float w = (int)font_text_width(&app->font, app->toast);
            int x = (a->root->w - w) / 2;
            int y = (int)((a->root->h - fh) / 2);
            dc.text(&dc, &colors.red, &app->font, x, y, app->toast, (int)strlen(app->toast));
        }
    }
}

static button_t* create_button(application_t* app, float x, float y, int key_flags, int key,
                               const char* mnemonic, const char* label, void (*click)(button_t* self)) {
    app_t* a = app->a;
    const float lw = font_text_width(&app->font, label) + app->font.em;
    const float bw = max(lw, pt2px(a, MIN_BUTTON_WIDTH_PT));
    const float bh = app->font.height * app->theme.ui_height;
    button_t* b = button_create(a->root, app, &app->theme, key_flags, key, mnemonic, label, x, y, bw, bh);
    b->ui.that = app;
    b->click = click;
    return b;
}

static void slider_notify(slider_t* s) {
    application_t* app = (application_t*)s->ui.a->that;
    if (s == app->slider1) {
        snprintf(app->slider1_label, countof(app->slider1_label), SLIDER1_LABEL, app->slider1_current);
    } else if (s == app->slider2) {
        snprintf(app->slider2_label, countof(app->slider2_label), SLIDER2_LABEL, app->slider2_current);
    }
}

static slider_t* create_slider(application_t* app, float x, float y, const char* label, int* minimum, int* maximum, int* current) {
    float fh = app->font.height;
    float w = font_text_width(&app->font, label) + app->font.em * 2;
    slider_t* s = slider_create(app->a->root, app, &app->theme, &colors.orange, label, x, y, w, fh, minimum, maximum, current);
    s->ui.that = app;
    s->notify = slider_notify;
    s->ui.focusable = true; // has buttons
    return s;
}

static void textures_mouse(ui_t* self, int flags, float x, float y) {
//  traceln("flags=0x%08X", flags);
    if (flags & MOUSE_LBUTTON_UP) { traceln("click at %.1f %.1f", x, y); }
}

static void test(ui_t* view) {
    application_t* app = (application_t*)view->a->that;
    const float w = view->w;
    const float h = view->h;
    pointf_t vertices0[] = { {1, 1}, {1 + 99, 1},  {1, 1 + 99} };
    pointf_t vertices1[] = { {w - 2, h - 2}, {w - 2, h - 2 - 99},  {w - 2 - 99, h - 2} };
    dc.poly(&dc, &colors.red,   vertices0, countof(vertices0));
    dc.poly(&dc, &colors.green, vertices1, countof(vertices1));
    dc.fill(&dc, &colors.black, 100, 100, app->bitmaps[2].w, app->bitmaps[2].h);
    dc.bblt(&dc, &app->bitmaps[2], 100, 100);
    colorf_t green = colors.green;
    green.a = 0.75; // translucent
    dc.luma(&dc, &green, &app->font.atlas, 100, 100);
    dc.rect(&dc, &colors.white, 100, 100, w - 200, h - 200, 4);
    dc.ring(&dc, &colors.white, w / 2, h / 2, 100, 50);
    dc.text(&dc, &colors.white, &app->font, 500, 500, "Hello World", strlen("Hello World"));
    dc.text(&dc, &colors.white, &app->font, 560, 560, "ABC", strlen("ABC"));
    dc.line(&dc, &colors.red, w / 2 - 100, h / 2, w / 2 + 100, h / 2 + 100, 10);
}

static void root_draw(ui_t* view) {
    application_t* app = (application_t*)view->a->that;
    dc.clear(&dc, &colors.nc_dark_blue);
    const bool simple_test = false;
    if (simple_test) {
        test(view);
    } else {
        view->draw_children(view);
        toast_draw(app);
    }
}

static void glyphs_draw(ui_t* view) {
    application_t* app = (application_t*)view->a->that;
    font_t* f = &app->font;
    float x = view->x + 0.5;
    float y = view->y + 0.5;
    dc.luma(&dc, &colors.white, &f->atlas, x, y);
    view->draw_children(view);
}

static void ascii_draw(ui_t* view) {
    application_t* app = (application_t*)view->a->that;
    float x = view->x + 0.5;
    float y = view->y + 0.5;
    char text[97] = {};
    for (int i = 0; i < 96; i++) { text[i] = 32 + i; }
    screen_writer_t sw = screen_writer(x, y, app->theme.font, &colors.green);
    for (int i = 0; i < countof(text); i += 24) {
        sw.println(&sw, "%-24.24s", &text[i]);
    }
    view->draw_children(view);
}

static void textures_draw(ui_t* view) {
    application_t* app = (application_t*)view->a->that;
    dc.line(&dc, &colors.white, 0.5, 0.5, 0.5, 240 + 2.5, 1);
    for (int i = 0; i < countof(app->bitmaps); i++) {
        bitmap_t* b = &app->bitmaps[i];
        float x = i * (b->w + 1.5);
        dc.bblt(&dc, b, x + 1.5, 1.5);
        dc.line(&dc, &colors.white, x + b->w + 1.5, 1.5, x + b->w + 1.5, b->h + 1.5, 1.5);
    }
    dc.line(&dc, &colors.white, 0.5, 1.5, view->w, 1.5, 1);
    dc.line(&dc, &colors.white, 0.5, 240 + 2.5, view->w, 240 + 2.5, 1);
    view->draw_children(view);
}

static void on_quit(button_t* b) {
    application_t* app = (application_t*)b->ui.a->that;
    app->a->quit(app->a);
}

static void on_exit(button_t* b) {
    application_t* app = (application_t*)b->ui.a->that;
    app->a->exit(app->a, 153); // gdb shows octal 0o231 exit status for some reason...
}

static void init_ui(application_t* app) {
    float vgap = pt2px(app->a, VERTICAL_GAP_PT);
    float hgap = pt2px(app->a, HORIZONTAL_GAP_PT);
    float bh = app->font.height * 3 / 2; // button height
    float y = 240 + vgap;
    app->quit     = create_button(app, 10, y, 0, 'q', "Q", "Quit", on_quit);        y += bh + vgap;
    app->exit     = create_button(app, 10, y, 0, 'e', "E", "Exit(153)", on_exit);   y += bh + vgap;
    app->checkbox = create_button(app, 10, y, 0, 'x', "X", "Checkbox", null);       y += bh + vgap;

    int x = app->checkbox->ui.w + hgap * 4;
    y = 240 + vgap;
    app->slider1_minimum = 0;
    app->slider1_maximum = 255;
    app->slider1_current = 240;
    snprintf(app->slider1_label, countof(app->slider1_label), SLIDER1_LABEL, app->slider1_current);
    app->slider1 = create_slider(app, x, y, app->slider1_label, &app->slider1_minimum, &app->slider1_maximum, &app->slider1_current);

    y += bh + vgap;
    app->slider2_minimum = 0;
    app->slider2_maximum = 1023;
    app->slider2_current = 512;
    snprintf(app->slider2_label, countof(app->slider2_label), SLIDER2_LABEL, app->slider2_current);
    app->slider2 = create_slider(app, x, y, app->slider2_label, &app->slider2_minimum, &app->slider2_maximum, &app->slider2_current);

    y += bh + vgap;
    app->view_glyphs = app->a->root->create(app->a->root, app, x, y, app->font.atlas.w, app->font.atlas.h);
    app->view_glyphs->draw = glyphs_draw;
    app->view_glyphs->hidden = true;
    app->checkbox->flip = &app->view_glyphs->hidden;

    y += app->font.atlas.h;
    app->view_ascii = app->a->root->create(app->a->root, app, 0, y, app->font.em * 26, app->font.em * 4);
    app->view_ascii->draw = ascii_draw;

    app->a->root->draw = root_draw;

    app->view_textures = app->a->root->create(app->a->root, app, 0, 0, 320 * 3 + 4, 240 + 2);
    app->view_textures->mouse = textures_mouse;
    app->view_textures->draw = textures_draw;
}

static void load_font(application_t* app) {
    int r = 0;
    int hpx = (int)(pt2px(app->a, FONT_HEIGHT_PT) + 0.5); // font height in pixels
    if (hpx != app->font.height) {
        if (app->font.atlas.data != null) { font_dispose(&app->font); }
        // https://en.wikipedia.org/wiki/Liberation_fonts https://github.com/liberationfonts
        // https://github.com/liberationfonts/liberation-fonts/releases
        // Useful: https://www.glyphrstudio.com/online/ and https://convertio.co/otf-ttf/
        // https://github.com/googlefonts/noto-fonts/tree/master
        r = font_load_asset(&app->font, app->a, "liberation-mono-bold-ascii.ttf", hpx, 32, 98);
        assert(r == 0); (void)r;
    }
    r = bitmap_allocate_and_update_texture(&app->font.atlas);
    assert(r == 0);
    if (r != 0) { exit(r); } // fatal
}

static void init_expo(application_t* app) {
    ui_expo_t* ex = &app->theme;
    ex->font = &app->font;
    ex->ui_height = 1.5; // 150% of font height in pixels for UI elements height
    ex->color_text = &colors.nc_light_blue;
    ex->color_background =&colors.nc_teal;
    ex->color_mnemonic = &colors.nc_dirty_gold;
    ex->color_focused = ex->color_text; // TODO: lighter
    ex->color_background_focused = &colors.nc_light_gray;
    ex->color_armed = ex->color_text; // TODO: different
    ex->color_background_armed = &colors.orange;
    ex->color_pressed = ex->color_text; // TODO: different
    ex->color_background_pressed = &colors.green;
}

static int create_gl_program(app_t* a, const char* name, int *program) {
    *program = 0;
    int r = 0;
    void* assets[2] = {};
    static const char* suffix[] = { "vertex", "fragment" };
    char names[2][128];
    for (int i = 0; i < countof(names); i++) {
        snprintf(names[i], countof(names[i]), "%s_%s.glsl", name, suffix[i]);
    };
    gl_shader_source_t sources[] = {
        {GL_SHADER_VERTEX, names[0], null, 0},
        {GL_SHADER_FRAGMENT, names[1], null, 0}
    };
    for (int i = 0; i < countof(sources); i++) {
        assets[i] = a->asset_map(a, sources[i].name, &sources[i].data, &sources[i].bytes);
        assertion(assets[i] != null, "asset \"%s\"not found", sources[i].name);
//      traceln("%s=\n%.*s", sources[i].name, sources[i].bytes, sources[i].data);
    }
    r = shader_program_create_and_link(program, sources, countof(sources));
    assert(r == 0);
    for (int i = 0; i < countof(sources); i++) {
        a->asset_unmap(a, assets[i], sources[i].data, sources[i].bytes);
    }
    return r;
}

static void shown(app_t* a) {
    application_t* app = (application_t*)a->that;
    // both model and view matricies are identity
    gl_ortho2D(app->mvp, 0, a->root->w, a->root->h, 0);
    dc.init(&dc, app->mvp);
    load_font(app);
    (void)create_gl_program;
//  int r = create_gl_program(a, "main", &app->program_main);
    int r = shaders_init();
    assert(r == 0);
    init_expo(app);
    for (int i = 0; i < countof(app->bitmaps); i++) {
        bitmap_allocate_and_update_texture(&app->bitmaps[i]);
    }
    init_ui(app);
    snprintf(app->toast, countof(app->toast), "resolution %.0fx%.0fpx", a->root->w, a->root->h);
}

static void hidden(app_t* a) {
    // application/activity is detached from its window. On Android application will NOT exit
    application_t* app = (application_t*)a->that;
    app->view_textures->dispose(app->view_textures); app->view_textures = null;
    app->view_glyphs->dispose(app->view_glyphs);     app->view_glyphs = null;
    app->view_ascii->dispose(app->view_ascii);       app->view_ascii = null;
    button_dispose(app->quit);          app->quit = null;
    button_dispose(app->exit);          app->exit = null;
    button_dispose(app->checkbox);      app->checkbox = null;
    slider_dispose(app->slider1);       app->slider1 = null;
    slider_dispose(app->slider2);       app->slider2 = null;
    bitmap_deallocate_texture(&app->font.atlas);
    for (int i = 0; i < countof(app->bitmaps); i++) { bitmap_deallocate_texture(&app->bitmaps[i]); }
//  shader_program_dispose(app->program_main);   app->program_main = 0;
    shaders_dispose();
    dc.dispose(&dc);
}

static void idle(app_t* a) {
    application_t* app = (application_t*)a->that;
    snprintf(app->slider1_label, countof(app->slider1_label), SLIDER1_LABEL, app->slider1_current);
    snprintf(app->slider2_label, countof(app->slider2_label), SLIDER2_LABEL, app->slider2_current);
}

static void stop(app_t* a) {
//  application_t* app = (application_t*)a->that;
}

static void paused(app_t* a) { // pause() defined in unistd.h
//  application_t* app = (application_t*)a->that;
}

static void resume(app_t* a) {
//  application_t* app = (application_t*)a->that;
}

static void init(app_t* a) { // init application
    application_t* app = (application_t*)a->that;
    if (app->bitmaps[0].data == null) {
        bitmap_load_asset(&app->bitmaps[0], a, "cube-320x240.png");
        bitmap_load_asset(&app->bitmaps[1], a, "geometry-320x240.png");
        bitmap_load_asset(&app->bitmaps[2], a, "machine-320x240.png");
    }
}

static void destroy(app_t* a) {
    application_t* app = (application_t*)a->that;
    for (int i = 0; i < countof(app->bitmaps); i++) {
        bitmap_dispose(&app->bitmaps[i]);
    }
    font_dispose(&app->font);
}

static application_t app;

void app_create(app_t* a) {
    a->that = &app;
    app.a = a;
    app.a->init    = init;
    app.a->shown   = shown;
    app.a->idle    = idle;
    app.a->hidden  = hidden;
    app.a->pause   = paused;
    app.a->stop    = stop;
    app.a->resume  = resume;
    app.a->destroy = destroy;
    app.a->root->that = &app;
}

END_C
