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
#include "checkbox.h"
#include "slider.h"
#include "toast.h"
#include "screen_writer.h"
#include "shaders.h"

begin_c

static const char* SLIDER1_LABEL = "Slider1: %-3d";
static const char* SLIDER2_LABEL = "Slider2: %-3d";

enum {
    FONT_HEIGHT_PT      =  10, // pt = point = 1/72 inch
    MIN_BUTTON_WIDTH_PT =  60,
    VERTICAL_GAP_PT     =   8,
    HORIZONTAL_GAP_PT   =   8
};

typedef struct {
    app_t a;
    int font_height_px;
    font_t font;    // default UI font
//  int program_main;
    texture_t bitmaps[3];
    button_t quit;
    button_t exit;
    checkbox_t glyphs;
    checkbox_t test;
    bool testing; // testing draw commands primitives
    slider_t slider1;
    slider_t slider2;
    ui_t ui_content;
    ui_t ui_textures;
    ui_t ui_glyphs;
    ui_t ui_ascii;
    int  slider1_minimum;
    int  slider1_maximum;
    int  slider1_current;
    char slider1_label[64];
    int  slider2_minimum;
    int  slider2_maximum;
    int  slider2_current;
    char slider2_label[64];
} demo_t;

static inline_c float pt2px(app_t* a, float pt) { return pt * a->xdpi / 72.0f; }

static void init_button(demo_t* d, button_t* b, float x, float y, int key_flags, int key,
                               const char* mnemonic, const char* label, void (*click)(ui_t*)) {
    app_t* a = &d->a;
    const float lw = font_text_width(&d->font, label, -1) + d->font.em;
    const float bw = max(lw, pt2px(a, MIN_BUTTON_WIDTH_PT));
    const float bh = d->font.height * d->a.theme.ui_height;
    button_init(b, &d->ui_content, d, key_flags, key, mnemonic, label, x, y, bw, bh);
    b->btn.ui.that = d;
    b->btn.click = click;
}

static void init_checkbox(demo_t* d, checkbox_t* cbx, float x, float y, int key_flags, int key,
                               const char* mnemonic, const char* label, void (*click)(ui_t*)) {
    app_t* a = &d->a;
    const float lw = font_text_width(&d->font, label, -1) + d->font.em;
    const float bw = max(lw, pt2px(a, MIN_BUTTON_WIDTH_PT));
    const float bh = d->font.height * d->a.theme.ui_height;
    checkbox_init(cbx, &d->ui_content, d, key_flags, key, mnemonic, label, x, y, bw, bh);
    cbx->btn.ui.that = d;
    cbx->btn.click = click;
}

static void slider_notify(slider_t* s) {
    demo_t* d = (demo_t*)s->ui.a->that;
    if (s == &d->slider1) {
        snprintf0(d->slider1_label, SLIDER1_LABEL, d->slider1_current);
    } else if (s == &d->slider2) {
        snprintf0(d->slider2_label, SLIDER2_LABEL, d->slider2_current);
    }
}

static void init_slider(demo_t* d, slider_t* s, float x, float y, const char* label,
                        int* minimum, int* maximum, int* current) {
    float fh = d->font.height;
    float w = font_text_width(&d->font, label, -1) + d->font.em * 2;
    slider_init(s, &d->ui_content, d, label, x, y, w, fh, minimum, maximum, current);
    s->ui.that = d;
    s->notify = slider_notify;
}

static void textures_mouse(ui_t* ui, int flags, float x, float y) {
    if (flags & MOUSE_LBUTTON_UP) { traceln("click at %.1f %.1f", x, y); }
}

static void test(ui_t* ui) {
    demo_t* d = (demo_t*)ui->a->that;
    const float w = ui->w;
    const float h = ui->h;
    pointf_t vertices0[] = { {1, 1}, {1 + 99, 1},  {1, 1 + 99} };
    pointf_t vertices1[] = { {w - 2, h - 2}, {w - 2, h - 2 - 99},  {w - 2 - 99, h - 2} };
    dc.poly(&dc, colors.red,   vertices0, countof(vertices0));
    dc.poly(&dc, colors.green, vertices1, countof(vertices1));
    dc.fill(&dc, colors.black, 100, 100, d->bitmaps[2].w, d->bitmaps[2].h);
    dc.bblt(&dc, &d->bitmaps[2], 100, 100);
    colorf_t green = *colors.green;
    green.a = 0.75; // translucent
    dc.luma(&dc, &green, &d->font.atlas, 100, 100);
    dc.rect(&dc, colors.white, 100, 100, w - 200, h - 200, 4);
    float x = w / 2;
    float y = h / 2;
    float r = 100;
    dc.fill(&dc, colors.black, x - r, y - r, r * 2, r * 2);
    dc.ring(&dc, colors.white, x, y, r, r / 2);
    colorf_t c = *colors.green; c.a = 0.50;
    dc.fill(&dc, &c, x - r / 2, y - r / 2, r, r);
    dc.text(&dc, colors.white, &d->font, 500, 500, "Hello World", strlen("Hello World"));
    dc.text(&dc, colors.white, &d->font, 560, 560, "ABC", strlen("ABC"));
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
    demo_t* d = (demo_t*)ui->a->that;
    if ((mouse_action & MOUSE_LBUTTON_UP) && d->testing) {
        d->testing = false;
        ui->a->invalidate(ui->a);
    }
}

static void content_draw(ui_t* ui) {
    demo_t* d = (demo_t*)ui->a->that;
    dc.clear(&dc, colors_nc.dark_blue);
    if (d->testing) {
        test(ui);
    } else {
        ui->draw_children(ui);
    }
}

static void glyphs_draw(ui_t* ui) {
    demo_t* d = (demo_t*)ui->a->that;
    font_t* f = &d->font;
    float x = ui->x + 0.5;
    float y = ui->y + 0.5;
    dc.luma(&dc, colors.white, &f->atlas, x, y);
    ui->draw_children(ui);
}

static void ascii_draw(ui_t* ui) {
    demo_t* d = (demo_t*)ui->a->that;
    float x = ui->x + 0.5;
    float y = ui->y + 0.5;
    char text[97] = {};
    for (int i = 0; i < 96; i++) { text[i] = 32 + i; }
    screen_writer_t sw = screen_writer(x, y, d->a.theme.font, colors.green);
    for (int i = 0; i < countof(text); i += 24) {
        sw.println(&sw, "%-24.24s", &text[i]);
    }
    ui->draw_children(ui);
}

static void textures_draw(ui_t* ui) {
    demo_t* d = (demo_t*)ui->a->that;
    dc.line(&dc, colors.white, 0.5, 0.5, 0.5, 240 + 2.5, 1);
    for (int i = 0; i < countof(d->bitmaps); i++) {
        texture_t* b = &d->bitmaps[i];
        float x = i * (b->w + 1.5);
        dc.bblt(&dc, b, x + 1.5, 1.5);
        dc.line(&dc, colors.white, x + b->w + 1.5, 1.5, x + b->w + 1.5, b->h + 1.5, 1.5);
    }
    dc.line(&dc, colors.white, 0.5, 1.5, ui->w, 1.5, 1);
    dc.line(&dc, colors.white, 0.5, 240 + 2.5, ui->w, 240 + 2.5, 1);
    ui->draw_children(ui);
}

static void on_quit(ui_t* ui) {
    app_t* a = ui->a;
    a->quit(a);
}

static void on_exit(ui_t* ui) {
    app_t* a = ui->a;
    a->exit(a, 153); // gdb shows octal 0o231 exit status for some reason...
}

static void on_glyphs(ui_t* ui) {
    app_t* a = ui->a;
    demo_t* d = (demo_t*)a->that;
    a->show_keyboard(a, !d->ui_glyphs.hidden);
}

static void on_test(ui_t* b) {
//  app_t* a = ui->a;
//  b->btn.ui.a->invalidate(b->btn.ui.a); // no need to call invalidate here
}

static void init_ui(demo_t* d) {
    ui_t* content = &d->ui_content;
    d->a.root.init(content, &d->a.root, d, 0, 0, d->a.root.w, d->a.root.h);
    float vgap = pt2px(&d->a, VERTICAL_GAP_PT);
    float hgap = pt2px(&d->a, HORIZONTAL_GAP_PT);
    float bh = d->font.height * 3 / 2; // button height
    float y = 240 + vgap;
    init_button(d, &d->quit,     10, y, 0, 'q', "Q", "Quit",      on_quit);   y += bh + vgap;
    init_button(d, &d->exit,     10, y, 0, 'e', "E", "Exit(153)", on_exit);   y += bh + vgap;
    init_checkbox(d, &d->test,   10, y, 0, 'x', "X", "Test",      on_test);   y += bh + vgap;
    init_checkbox(d, &d->glyphs, 10, y, 0, 'x', "X", "Glyphs",    on_glyphs); y += bh + vgap;
    // ascii and ui_textures views
    content->init(&d->ui_ascii, content, d, 0, y, d->font.em * 26, d->font.em * 4);
    d->ui_ascii.draw = ascii_draw;
    content->draw  = content_draw;
    content->mouse = content_mouse;
    content->init(&d->ui_textures, content, d, 0, 0, 320 * 3 + 4, 240 + 2);
    d->ui_textures.mouse = textures_mouse;
    d->ui_textures.draw = textures_draw;
    // sliders
    const int x = d->glyphs.btn.ui.w + hgap;
    y = 240 + vgap;
    d->slider1_minimum = 0;
    d->slider1_maximum = 255;
    d->slider1_current = 240;
    snprintf0(d->slider1_label, SLIDER1_LABEL, d->slider1_current);
    init_slider(d, &d->slider1, x, y, d->slider1_label, &d->slider1_minimum, &d->slider1_maximum, &d->slider1_current);
    y += bh + vgap;
    d->slider2_minimum = 0;
    d->slider2_maximum = 1023;
    d->slider2_current = 512;
    snprintf0(d->slider2_label, SLIDER2_LABEL, d->slider2_current);
    init_slider(d, &d->slider2, x, y, d->slider2_label, &d->slider2_minimum, &d->slider2_maximum, &d->slider2_current);
    y += bh + vgap;
    content->init(&d->ui_glyphs, content, d, x, y, d->font.atlas.w, d->font.atlas.h);
    d->ui_glyphs.draw = glyphs_draw;
    d->ui_glyphs.hidden = true;
    d->test.btn.flip = &d->testing;
    d->glyphs.btn.flip = &d->ui_glyphs.hidden;
    d->glyphs.btn.inverse = true; // because flip point to hidden not to `shown` in the absence of that bit
    y += d->font.atlas.h;
}

static void load_font(demo_t* d) {
    int r = 0;
    int hpx = (int)(pt2px(&d->a, FONT_HEIGHT_PT) + 0.5); // font height in pixels
    if (hpx != d->font.height) {
        if (d->font.atlas.data != null) { font_dispose(&d->font); }
        // https://en.wikipedia.org/wiki/Liberation_fonts https://github.com/liberationfonts
        // https://github.com/liberationfonts/liberation-fonts/releases
        // Useful: https://www.glyphrstudio.com/online/ and https://convertio.co/otf-ttf/
        // https://github.com/googlefonts/noto-fonts/tree/master
        r = font_load_asset(&d->font, &d->a, "liberation-mono-bold-ascii.ttf", hpx, 32, 98);
        assert(r == 0); (void)r;
    }
    r = texture_allocate_and_update(&d->font.atlas);
    assert(r == 0);
    if (r != 0) { exit(r); } // fatal
}

static void init_theme(demo_t* d) {
    static colorf_t light_gray_alpha_0_60;
    light_gray_alpha_0_60 = *colors.light_gray; light_gray_alpha_0_60.a = 0.6;
    theme_t* th = &d->a.theme;
    th->font = &d->font;
    th->ui_height = 1.5; // 150% of font height in pixels for UI elements height
    th->color_text                = colors_nc.light_blue;
    th->color_background          = colors_nc.teal;
    th->color_mnemonic            = colors_nc.dirty_gold;
    th->color_focused             = th->color_text; // TODO: lighter
    th->color_background_focused  = colors_nc.light_gray;
    th->color_armed               = th->color_text; // TODO: different
    th->color_background_armed    = colors.orange;
    th->color_pressed             = th->color_text; // TODO: different
    th->color_background_pressed  = colors.green;
    th->color_slider              = colors.orange;
    th->color_toast_text          = colors.black;
    th->color_toast_background    = &light_gray_alpha_0_60;
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
    }
    r = shader_program_create_and_link(program, sources, countof(sources));
    assert(r == 0);
    for (int i = 0; i < countof(sources); i++) {
        a->asset_unmap(a, assets[i], sources[i].data, sources[i].bytes);
    }
    return r;
}

static void resized(app_t* a, int x, int y, int w, int h) {
    // both model and view matricies are identity:
    ui_t* root = &a->root;
    root->x = x;
    root->y = y;
    root->w = w;
    root->h = h;
    dc.viewport(&dc, root->x, root->y, root->w, root->h);
    // no need to call invalidate() caller will do it
}

static void shown(app_t* a, int w, int h) {
    a->root.w = w;
    a->root.h = h;
    dc.init(&dc);
    resized(a, 0, 0, w, h);
    demo_t* d = (demo_t*)a->that;
    load_font(d);
    (void)create_gl_program;
//  int r = create_gl_program(a, "main", &d->program_main);
    int r = shaders_init();
    assert(r == 0);
    init_theme(d);
    for (int i = 0; i < countof(d->bitmaps); i++) {
        texture_allocate_and_update(&d->bitmaps[i]);
    }
    init_ui(d);
    toast_print(0, "resolution\n%.0fx%.0fpx", a->root.w, a->root.h);
}

static void draw(app_t* a) {
    assertion(!a->root.hidden, "there is no meaningful reason to hide root");
    a->root.draw(&a->root);
}

static void hidden(app_t* a) {
    demo_t* d = (demo_t*)a->that;
    // Application/activity is detached from the window.
    // Window surface may be different next time application is shown()
    // On Android application may continue running.
    toast_cancel();
    d->ui_textures.done(&d->ui_textures);
    d->ui_glyphs.done(&d->ui_glyphs);
    d->ui_ascii.done(&d->ui_ascii);
    d->ui_content.done(&d->ui_content);
    button_done(&d->quit);
    button_done(&d->exit);
    checkbox_done(&d->glyphs);
    checkbox_done(&d->test);
    slider_done(&d->slider1);
    slider_done(&d->slider2);
    texture_deallocate(&d->font.atlas);
    for (int i = 0; i < countof(d->bitmaps); i++) { texture_deallocate(&d->bitmaps[i]); }
//  shader_program_dispose(d->program_main);   d->program_main = 0;
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
    demo_t* d = (demo_t*)a->that;
    a->root.that = &d;
    app->root    = *ui_if;
    app->root.a  = a;
    if (d->bitmaps[0].data == null) {
        texture_load_asset(&d->bitmaps[0], a, "cube-320x240.png");
        texture_load_asset(&d->bitmaps[1], a, "geometry-320x240.png");
        texture_load_asset(&d->bitmaps[2], a, "machine-320x240.png");
    }
}

static void done(app_t* a) {
    demo_t* d = (demo_t*)a->that;
    for (int i = 0; i < countof(d->bitmaps); i++) {
        texture_dispose(&d->bitmaps[i]);
    }
    font_dispose(&d->font);
}

static bool dispatch_keyboard_shortcuts(ui_t* ui, int flags, int keycode) {
    if (ui->kind == UI_KIND_BUTTON && !ui->hidden) {
        btn_t* b = (btn_t*)ui;
        int kc = isalpha(keycode) ? tolower(keycode) : keycode;
        int k = isalpha(b->key) ? tolower(b->key) : b->key;
        if (!ui->hidden && b->key_flags == flags && k == kc) {
            // TODO: (Leo) if 3 (or more) states checkboxes are required this is the place to do it.
            //       b->flip = (b->flip + 1) % b->flip_wrap_around;
            if (b->flip != null) { *b->flip = !*b->flip; }
            ui->a->invalidate(ui->a);
            b->click(&b->ui);
            return true; // stop search
        }
    }
    ui_t* c = ui->children;
    while (c != null) {
        if (!c->hidden) {
            if (dispatch_keyboard_shortcuts(c, flags, keycode)) { return true; }
        }
        c = c->next;
    }
    return false;
}

static void key(app_t* a, int flags, int keycode) {
    if (keycode == KEY_CODE_BACK) {
        a->quit(a);
    } else {
        a->keyboard_flags = flags;
        if (a->focused != null && a->focused->keyboard != null) {
            a->focused->keyboard(a->focused, flags, keycode);
        }
        if (flags & KEYBOARD_KEY_PRESSED) {
            int f = flags & ~(KEYBOARD_KEY_PRESSED|KEYBOARD_SHIFT|KEYBOARD_NUMLOCK|KEYBOARD_CAPSLOCK);
            dispatch_keyboard_shortcuts(&a->root, f, keycode);
        }
    }
}

static void touch(app_t* a, int index, int action, int x, int y) {
    if (index == 0) {
        if (action & MOUSE_LBUTTON_DOWN) {
            if (!ui_set_focus(&a->root, x, y)) {
                a->focus(a, null); // kill focus if no focusable components were found
            }
        }
        ui_dispatch_mouse(&a->root, action, x, y);
        ui_dispatch_screen_mouse(&a->root, action, x, y);
    } else {
        // multi-touch gesture recognizer goes here
    }
}

static demo_t demo = {
    /* a = */ {
        init,
        shown,
        draw,
        resized,
        hidden,
        paused,
        stop,
        resume,
        done,
        key,
        touch
    }
};

static_init(demo) {
    app = &demo.a;
    app->that    = &demo;
}

end_c
