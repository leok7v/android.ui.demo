#include "ui.h"
#include "button.h"
#include "app.h"

BEGIN_C

static void button_draw(ui_t* ui) {
    button_t* b = (button_t*)ui;
    colorf_t* color = b->bitset & BUTTON_STATE_PRESSED ? b->expo->color_background_pressed : b->expo->color_background;
    pointf_t pt = ui->screen_xy(ui);
    gl_draw_rect(color, pt.x, pt.y, ui->w, ui->h);
    int k = (int)strlen(b->label) + 1;
    const char* mn = b->mnemonic;
    char letter[2] = {};
    char copy[k];
    strncpy(copy, b->label, k);
    int m = -1;
    if (b->mnemonic != null) {
        const char* p = strstr(copy, b->mnemonic);
        if (p != null) { m = (int)(uintptr_t)(p - copy); }
    } else if (m < 0 && isalpha(b->key)) {
        int lck = tolower(b->key);
        for (int i = 0; i < k - 1 && m < 0; i++) {
            if (lck == tolower(copy[i])) { letter[0] = copy[i]; mn = letter; m = i; }
        }
    }
    font_t* f = b->expo->font;
    float fh = f->height;
    float em = f->em;
    float baseline = f->baseline;
    pt.y += (int)(baseline + (ui->h - fh) / 2);
    pt.x += em;
    gl_set_color(b->expo->color_text);
    if (b->flip != null) {
        const char* check = *b->flip ? "[X] " : "[.] ";
        pt.x = font_draw_text(f, pt.x, pt.y, check);
    }
    font_draw_text(f, pt.x, pt.y, b->label);
    if (m >= 0) { // draw highlighted mnemonic
        copy[m] = 0;
        float mx = pt.x + font_text_width(f, copy);
        gl_set_color(b->expo->color_mnemonic);
        font_draw_text(f, mx, pt.y, mn);
    }
}

static void button_mouse(ui_t* self, int mouse_action, float x, float y) {
    button_t* b = (button_t*)self;
    if (mouse_action & MOUSE_LBUTTON_DOWN) {
        b->bitset |= BUTTON_STATE_PRESSED;
        self->a->invalidate(self->a);
    }
    if (mouse_action & MOUSE_LBUTTON_UP) {
        // TODO: (Leo) if we need 3 (or more) state flip this is the place to do it. b->flip = (b->flip + 1) % b->checkbox_wrap_around;
        if (b->flip != null) { *b->flip = !*b->flip; }
        if (b->click != null) { b->click(b); }
        b->ui.a->vibrate(b->ui.a, EFFECT_CLICK);
        b->bitset &= ~BUTTON_STATE_PRESSED;
        self->a->invalidate(self->a);
    }
}

static void button_screen_mouse(ui_t* self, int mouse_action, float x, float y) {
    button_t* b = (button_t*)self;
    pointf_t pt = self->screen_xy(self);
    bool inside = pt.x <= x && x < pt.x + self->w && pt.y <= y && y <pt.y + self->h;
    if (!inside && (b->bitset & (BUTTON_STATE_PRESSED|BUTTON_STATE_ARMED) != 0)) {
        b->bitset &= ~(BUTTON_STATE_PRESSED|BUTTON_STATE_ARMED); // disarm button
        self->a->invalidate(self->a);
    }
}

button_t* button_create(ui_t* parent, void* that, ui_expo_t* expo,
                        int key_flags, int key, const char* mnemonic,
                        const char* label,
                        float x, float y, float w, float h) {
    button_t* b = (button_t*)allocate(sizeof(button_t));
    if (b != null) {
        b->ui = *parent;
        b->ui.that = that;
        b->ui.parent = null;
        b->ui.children = null;
        b->ui.next = null;
        b->ui.focusable = true; // buttons are focusable by default
        b->ui.hidden = false;
        b->click = null;
        b->key = key;
        b->key_flags = key_flags;
        b->mnemonic = mnemonic;
        b->label = label;
        b->bitset = 0;
        b->expo = expo;
        assert((void*)b == (void*)&b->ui);
        parent->add(parent, &b->ui, x, y, w, h);
        assert(b->ui.parent == parent);
        b->ui.kind = UI_KIND_BUTTON;
        b->ui.draw = button_draw;
        b->ui.mouse = button_mouse;
        b->ui.screen_mouse = button_screen_mouse;
    }
    return b;
}

void button_dispose(button_t* b) {
    if (b != null) {
        b->ui.parent->remove(b->ui.parent, &b->ui);
        deallocate(b);
    }
}

END_C
