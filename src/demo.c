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
#include "dc.h"
#include "button.h"
#include "slider.h"
#include "toast.h"
#include "screen_writer.h"
#include "shaders.h"

BEGIN_C

static const char* SLIDER1_LABEL = "Slider1: %-3d";
static const char* SLIDER2_LABEL = "Slider2: %-3d";

enum {
    FONT_HEIGHT_PT      =  10, // pt = point = 1/72 inch
    MIN_BUTTON_WIDTH_PT =  90,
    VERTICAL_GAP_PT     =   8,
    HORIZONTAL_GAP_PT   =   8
};

typedef struct application_s {
    app_t* a;
    int font_height_px;
    font_t font;    // default UI font
    ui_theme_t theme;
//  int program_main;
    toast_t*  toast;
    bitmap_t  bitmaps[3];
    button_t* quit;
    button_t* exit;
    button_t* glyphs;
    button_t* test;
    bool testing; // testing draw commands primitives
    slider_t* slider1;
    slider_t* slider2;
    ui_t* view_content;
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
} application_t;

static inline_c float pt2px(app_t* a, float pt) { return pt * a->xdpi / 72.0f; }

static button_t* create_button(application_t* app, float x, float y, int key_flags, int key,
                               const char* mnemonic, const char* label, void (*click)(button_t* self)) {
    app_t* a = app->a;
    const float lw = font_text_width(&app->font, label, -1) + app->font.em;
    const float bw = max(lw, pt2px(a, MIN_BUTTON_WIDTH_PT));
    const float bh = app->font.height * app->theme.ui_height;
    button_t* b = button_create(app->view_content, app, &app->theme, key_flags, key, mnemonic, label, x, y, bw, bh);
    b->ui.that = app;
    b->click = click;
    return b;
}

static void slider_notify(slider_t* s) {
    application_t* app = (application_t*)s->ui.a->that;
    if (s == app->slider1) {
        snprintf0(app->slider1_label, SLIDER1_LABEL, app->slider1_current);
    } else if (s == app->slider2) {
        snprintf0(app->slider2_label, SLIDER2_LABEL, app->slider2_current);
    }
}

static slider_t* create_slider(application_t* app, float x, float y, const char* label, int* minimum, int* maximum, int* current) {
    float fh = app->font.height;
    float w = font_text_width(&app->font, label, -1) + app->font.em * 2;
    slider_t* s = slider_create(app->view_content, app, &app->theme, colors.orange, label, x, y, w, fh, minimum, maximum, current);
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
    dc.poly(&dc, colors.red,   vertices0, countof(vertices0));
    dc.poly(&dc, colors.green, vertices1, countof(vertices1));
    dc.fill(&dc, colors.black, 100, 100, app->bitmaps[2].w, app->bitmaps[2].h);
    dc.bblt(&dc, &app->bitmaps[2], 100, 100);
    colorf_t green = *colors.green;
    green.a = 0.75; // translucent
    dc.luma(&dc, &green, &app->font.atlas, 100, 100);
    dc.rect(&dc, colors.white, 100, 100, w - 200, h - 200, 4);
    float x = w / 2;
    float y = h / 2;
    float r = 100;
    dc.fill(&dc, colors.black, x - r, y - r, r * 2, r * 2);
    dc.ring(&dc, colors.white, x, y, r, r / 2);
    colorf_t c = *colors.green; c.a = 0.50;
    dc.fill(&dc, &c, x - r / 2, y - r / 2, r, r);
    dc.text(&dc, colors.white, &app->font, 500, 500, "Hello World", strlen("Hello World"));
    dc.text(&dc, colors.white, &app->font, 560, 560, "ABC", strlen("ABC"));
    dc.line(&dc, colors.red, w / 2 - 100, h / 2, w / 2 + 100, h / 2 + 100, 10);
    c = *colors_nc.dirty_gold;  c.a = 0.75;
    dc.line(&dc, &c, 0, 0, w, h, 4);
    x = 900;
    y = 700;
    r = 100;
    dc.fill(&dc, colors.black, x - r, y - r, r * 2, r * 2);
    dc.quadrant(&dc, colors.red,           x, y, r, 0);
    dc.quadrant(&dc, colors.green,         x, y, r, 1);
    dc.quadrant(&dc, colors.blue,          x, y, r, 2);
    dc.quadrant(&dc, colors_nc.dirty_gold, x, y, r, 3);
    x =  900;
    y = 1000;
    r =   50;
    dc.stadium(&dc, colors_nc.dirty_gold, x, y, 400, 200, r);
}

static void content_mouse(ui_t* ui, int mouse_action, float x, float y) {
    application_t* app = (application_t*)ui->a->that;
    if ((mouse_action & MOUSE_LBUTTON_UP) && app->testing) {
        app->testing = false;
        ui->a->invalidate(ui->a);
    }
}

static void content_draw(ui_t* view) {
    application_t* app = (application_t*)view->a->that;
    dc.clear(&dc, colors_nc.dark_blue);
    if (app->testing) {
        test(view);
    } else {
        view->draw_children(view);
    }
}

static void glyphs_draw(ui_t* view) {
    application_t* app = (application_t*)view->a->that;
    font_t* f = &app->font;
    float x = view->x + 0.5;
    float y = view->y + 0.5;
    dc.luma(&dc, colors.white, &f->atlas, x, y);
    view->draw_children(view);
}

static void ascii_draw(ui_t* view) {
    application_t* app = (application_t*)view->a->that;
    float x = view->x + 0.5;
    float y = view->y + 0.5;
    char text[97] = {};
    for (int i = 0; i < 96; i++) { text[i] = 32 + i; }
    screen_writer_t sw = screen_writer(x, y, app->theme.font, colors.green);
    for (int i = 0; i < countof(text); i += 24) {
        sw.println(&sw, "%-24.24s", &text[i]);
    }
    view->draw_children(view);
}

static void textures_draw(ui_t* view) {
    application_t* app = (application_t*)view->a->that;
    dc.line(&dc, colors.white, 0.5, 0.5, 0.5, 240 + 2.5, 1);
    for (int i = 0; i < countof(app->bitmaps); i++) {
        bitmap_t* b = &app->bitmaps[i];
        float x = i * (b->w + 1.5);
        dc.bblt(&dc, b, x + 1.5, 1.5);
        dc.line(&dc, colors.white, x + b->w + 1.5, 1.5, x + b->w + 1.5, b->h + 1.5, 1.5);
    }
    dc.line(&dc, colors.white, 0.5, 1.5, view->w, 1.5, 1);
    dc.line(&dc, colors.white, 0.5, 240 + 2.5, view->w, 240 + 2.5, 1);
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

static void on_glyphs(button_t* b) {
    application_t* app = (application_t*)b->ui.a->that;
    b->ui.a->show_keyboard(b->ui.a, !app->view_glyphs->hidden);
}

static void on_test(button_t* b) {
    b->ui.a->invalidate(b->ui.a);
}

static void init_ui(application_t* app) {
    ui_t* content = app->a->root->create(app->a->root, app, 0, 0, app->a->root->w, app->a->root->h);
    app->view_content = content;
    float vgap = pt2px(app->a, VERTICAL_GAP_PT);
    float hgap = pt2px(app->a, HORIZONTAL_GAP_PT);
    float bh = app->font.height * 3 / 2; // button height
    float y = 240 + vgap;
    app->quit   = create_button(app, 10, y, 0, 'q', "Q", "Quit", on_quit);      y += bh + vgap;
    app->exit   = create_button(app, 10, y, 0, 'e', "E", "Exit(153)", on_exit); y += bh + vgap;
    app->test   = create_button(app, 10, y, 0, 'x', "X", "Test", on_test);      y += bh + vgap;
    app->glyphs = create_button(app, 10, y, 0, 'x', "X", "Glyphs", on_glyphs);  y += bh + vgap;
    int x = app->glyphs->ui.w + hgap * 4;
    y = 240 + vgap;
    app->slider1_minimum = 0;
    app->slider1_maximum = 255;
    app->slider1_current = 240;
    snprintf0(app->slider1_label, SLIDER1_LABEL, app->slider1_current);
    app->slider1 = create_slider(app, x, y, app->slider1_label, &app->slider1_minimum, &app->slider1_maximum, &app->slider1_current);
    y += bh + vgap;
    app->slider2_minimum = 0;
    app->slider2_maximum = 1023;
    app->slider2_current = 512;
    snprintf0(app->slider2_label, SLIDER2_LABEL, app->slider2_current);
    app->slider2 = create_slider(app, x, y, app->slider2_label, &app->slider2_minimum, &app->slider2_maximum, &app->slider2_current);
    y += bh + vgap;
    app->view_glyphs = content->create(content, app, x, y, app->font.atlas.w, app->font.atlas.h);
    app->view_glyphs->draw = glyphs_draw;
    app->view_glyphs->hidden = true;
    app->test->flip = &app->testing;
    app->glyphs->flip = &app->view_glyphs->hidden;
    app->glyphs->inverse = true; // because flip point to hidden not to `shown` in the absence of that bit
    y += app->font.atlas.h;
    app->view_ascii = content->create(content, app, 0, y, app->font.em * 26, app->font.em * 4);
    app->view_ascii->draw = ascii_draw;
    content->draw  = content_draw;
    content->mouse = content_mouse;
    app->view_textures = content->create(content, app, 0, 0, 320 * 3 + 4, 240 + 2);
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

static void init_theme(application_t* app) {
    ui_theme_t* th = &app->theme;
    th->font = &app->font;
    th->ui_height = 1.5; // 150% of font height in pixels for UI elements height
    th->color_text        = colors_nc.light_blue;
    th->color_background  = colors_nc.teal;
    th->color_mnemonic    = colors_nc.dirty_gold;
    th->color_focused     = th->color_text; // TODO: lighter
    th->color_background_focused = colors_nc.light_gray;
    th->color_armed = th->color_text; // TODO: different
    th->color_background_armed = colors.orange;
    th->color_pressed = th->color_text; // TODO: different
    th->color_background_pressed = colors.green;
}

static int create_gl_program(app_t* a, const char* name, int *program) {
    *program = 0;
    int r = 0;
    void* assets[2] = {};
    static const char* suffix[] = { "vertex", "fragment" };
    char names[2][128];
    for (int i = 0; i < countof(names); i++) {
        snprintf0(names[i], "%s_%s.glsl", name, suffix[i]);
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

static void resized(app_t* a) {
    // both model and view matricies are identity:
    gl_ortho_2d(dc.mvp, a->root->x, a->root->y, a->root->w, a->root->h);
    a->invalidate(a);
}

static void shown(app_t* a) {
    dc.init(&dc);
    resized(a);
    application_t* app = (application_t*)a->that;
    load_font(app);
    (void)create_gl_program;
//  int r = create_gl_program(a, "main", &app->program_main);
    int r = shaders_init();
    assert(r == 0);
    init_theme(app);
    for (int i = 0; i < countof(app->bitmaps); i++) {
        bitmap_allocate_and_update_texture(&app->bitmaps[i]);
    }
    init_ui(app);
    toast_t* t = toast(a);
    t->theme = &app->theme;
    t->print(t, "resolution\n%.0fx%.0fpx", a->root->w, a->root->h);
}

static void hidden(app_t* a) {
    application_t* app = (application_t*)a->that;
    // Application/activity is detached from the window.
    // Window surface may be different next time application is shown()
    // On Android application may continue running.
    toast(a)->cancel(toast(a));
    app->view_textures->dispose(app->view_textures); app->view_textures = null;
    app->view_glyphs->dispose(app->view_glyphs);     app->view_glyphs   = null;
    app->view_ascii->dispose(app->view_ascii);       app->view_ascii    = null;
    app->view_content->dispose(app->view_content);   app->view_content  = null;
    button_dispose(app->quit);    app->quit    = null;
    button_dispose(app->exit);    app->exit    = null;
    button_dispose(app->glyphs);  app->glyphs  = null;
    button_dispose(app->test);    app->test    = null;
    slider_dispose(app->slider1); app->slider1 = null;
    slider_dispose(app->slider2); app->slider2 = null;
    bitmap_deallocate_texture(&app->font.atlas);
    for (int i = 0; i < countof(app->bitmaps); i++) { bitmap_deallocate_texture(&app->bitmaps[i]); }
//  shader_program_dispose(app->program_main);   app->program_main = 0;
    shaders_dispose();
    dc.dispose(&dc);
}

static void stop(app_t* a) {
}

static void paused(app_t* a) {
}

static void resume(app_t* a) {
}

static void init(app_t* a) { // init application
    application_t* app = (application_t*)a->that;
    a->root->that = &app;
    if (app->bitmaps[0].data == null) {
        bitmap_load_asset(&app->bitmaps[0], a, "cube-320x240.png");
        bitmap_load_asset(&app->bitmaps[1], a, "geometry-320x240.png");
        bitmap_load_asset(&app->bitmaps[2], a, "machine-320x240.png");
    }
}

static void done(app_t* a) {
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
    app.a->resized = resized;
    app.a->hidden  = hidden;
    app.a->pause   = paused;
    app.a->stop    = stop;
    app.a->resume  = resume;
    app.a->done    = done;
}

END_C
