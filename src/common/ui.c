#include "ui.h"
#include "app.h"

BEGIN_C

static void ui_add(ui_t* self, ui_t* child, float x, float y, float w, float h) {
    assert(child != null);
    assertion(child->parent == null && child->next == null, "Reparenting of a ui is not supported. Can be implemented if needed");
    if (child->next != null) { return; }
    ui_t* c = self->children;
    while (c != null) {
        assertion(c != child, "Attempt to add child that has already been added.");
        if (c == child) { return; }
        c = c->next;
    }
    child->next = self->children;
    self->children = child;
    child->x = x;
    child->y = y;
    child->w = w;
    child->h = h;
    child->parent = self;
}

static void ui_remove(ui_t* self, ui_t* child) {
    ui_t** prev = &self->children;
    ui_t* c = self->children;
    while (c != null) {
        if (c == child) {
            *prev = child->next;
            child->next = null;
            return;
        }
        prev = &c->next;
        c = c->next;
    }
    assertion(false, "control not found");
}

static ui_t* ui_create(ui_t* parent, void* that, float x, float y, float w, float h) {
    ui_t* ui = (ui_t*)allocate(sizeof(ui_t));
    if (ui != null) {
        *ui = *parent;
        ui->that = that;
        ui->parent = null;
        ui->children = null;
        ui->next = null;
        ui->focusable = false;
        ui->hidden = false;
        parent->add(parent, ui, x, y, w, h);
        ui->kind = UI_KIND_CONTAINER;
    }
    return ui;

}

static void ui_dispose(ui_t* ui) {
    if (ui != null) {
        ui->parent->remove(ui->parent, ui);
        deallocate(ui);
    }
}

static void ui_draw_children(ui_t* self) {
    ui_t* c = self->children;
    while (c != null) {
        if (!c->hidden) { c->draw(c); }
        c = c->next;
    }
}

static void ui_draw(ui_t* self) {
    ui_draw_children(self);
}

static void ui_mouse(ui_t* self, int mouse_action, float x, float y) {
    ui_t* c = self->children;
    while (c != null) {
        if (!c->hidden && c->x <= x && x < c->x + c->w && c->y <= y && y < c->y + c->h &&
            c->mouse != null) {
            c->mouse(c, mouse_action, x - c->x, y - c->y);
        }
        c = c->next;
    }
}

static void ui_screen_mouse(ui_t* self, int mouse_action, float x, float y) {
    ui_t* c = self->children;
    while (c != null) { // do it even for hidden widgets
        if (c->screen_mouse != null) { c->screen_mouse(c, mouse_action, x, y); }
        c = c->next;
    }
}

static void ui_keyboard(ui_t* self, int flags, int ch) {
    if (self->a->trace_flags & APP_TRACE_KEYBOARD) { app_trace_key(flags, ch); }
}

static void ui_focus(ui_t* self, bool gain) {
//  traceln("%p gain=%d", self, gain);
}

static pointf_t ui_screen_xy(ui_t* ui) {
    pointf_t pt = {ui->x, ui->y};
    ui_t* p = ui->parent;
    while (p != null) { pt.x += p->x; pt.y += p->y; p = p->parent; }
    return pt;
}

ui_t root = {
    /* that: */ null,
    /* kind: */ UI_KIND_CONTAINER,
    /* x, y, w, h */ 0, 0, 0, 0,
    /* hidden: */ false,
    /* focusable: */ false,
    /* parent: */ null,
    /* ui: */ null,
    /* next: */ null,
    /* children: */ null,
    ui_screen_xy,
    ui_add,
    ui_remove,
    ui_create,
    ui_dispose,
    ui_draw,
    ui_draw_children,
    ui_mouse,
    ui_screen_mouse,
    ui_keyboard,
    ui_focus
};

END_C
