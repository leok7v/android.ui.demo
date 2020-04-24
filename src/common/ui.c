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
#include "ui.h"
#include "app.h"

BEGIN_C

static void ui_add(ui_t* container, ui_t* child, float x, float y, float w, float h) {
    assert(child != null);
    assertion(child->parent == null && child->next == null, "Reparenting of a ui is not supported. Can be implemented if needed");
    if (child->next != null) { return; }
    ui_t* c = container->children;
    while (c != null) {
        assertion(c != child, "Attempt to add child that has already been added.");
        if (c == child) { return; }
        c = c->next;
    }
    child->next = container->children;
    container->children = child;
    child->x = x;
    child->y = y;
    child->w = w;
    child->h = h;
    child->parent = container;
}

static void ui_remove(ui_t* container, ui_t* child) {
    ui_t** prev = &container->children;
    ui_t* c = container->children;
    while (c != null) {
        if (c == child) {
            *prev = child->next;
            child->next = null;
            child->parent = null;
            return;
        }
        prev = &c->next;
        c = c->next;
    }
    assertion(false, "control not found");
}

static void ui_init(ui_t* ui, ui_t* parent, void* that, float x, float y, float w, float h) {
    *ui = *parent;
    ui->that = that;
    ui->parent = null;
    ui->children = null;
    ui->next = null;
    ui->focusable = false;
    ui->hidden = false;
    ui->decor = false;
    parent->add(parent, ui, x, y, w, h);
    ui->kind = UI_KIND_CONTAINER;
}

static void ui_done(ui_t* ui) {
    if (ui != null) {
        ui->parent->remove(ui->parent, ui);
    }
}

static void ui_draw_childs(ui_t* ui, bool decor) {
    ui_t* c = ui->children;
    while (c != null) {
        if (!c->hidden && !c->decor == !decor) { c->draw(c); }
        c = c->next;
    }
}

static void ui_draw_children(ui_t* ui) {
    ui_draw_childs(ui, false);
    ui_draw_childs(ui, true);
}

static void ui_draw(ui_t* ui) {
    ui_draw_children(ui);
}

static void ui_mouse(ui_t* ui, int mouse_action, float x, float y) {
}

static void ui_screen_mouse(ui_t* ui, int mouse_action, float x, float y) {
}

void ui_dispatch_mouse(ui_t* ui, int mouse_action, float x, float y) {
    ui_t* c = ui->children;
    while (c != null) {
        if (!c->hidden && c->x <= x && x < c->x + c->w && c->y <= y && y < c->y + c->h) {
            if (c->mouse != null) { c->mouse(c, mouse_action, x - c->x, y - c->y); }
            ui_dispatch_mouse(c, mouse_action, x - c->x, y - c->y);
        }
        c = c->next;
    }
}

void ui_dispatch_screen_mouse(ui_t* ui, int mouse_action, float x, float y) {
    ui_t* c = ui->children;
    while (c != null) { // do it even for hidden widgets
        if (c->screen_mouse != null) { c->screen_mouse(c, mouse_action, x, y); }
        ui_dispatch_screen_mouse(c, mouse_action, x, y);
        c = c->next;
    }
}

static void ui_keyboard(ui_t* ui, int flags, int ch) {
    if (ui->a->trace_flags & APP_TRACE_KEYBOARD) { app_trace_key(flags, ch); }
}

static void ui_focus(ui_t* ui, bool gain) {
//  traceln("%p gain=%d", ui, gain);
}

static pointf_t ui_screen_xy(ui_t* ui) {
    pointf_t pt = {ui->x, ui->y};
    ui_t* p = ui->parent;
    while (p != null) { pt.x += p->x; pt.y += p->y; p = p->parent; }
    return pt;
}

bool ui_set_focus(ui_t* ui, int x, int y) {
    assert(ui != null);
    ui_t* child = ui->children;
    bool focus_was_set = false;
    while (child != null && !focus_was_set) {
        assert(child != ui);
        focus_was_set = ui_set_focus(child, x, y);
        child = child->next;
    }
//  traceln("focus_was_set=%d on children of %p", focus_was_set, ui);
    if (!focus_was_set && ui->focusable) {
        const pointf_t pt = ui->screen_xy(ui);
//      traceln("%p %.1f,%.1f %.1fx%.1f focusable=%d mouse %d,%d  pt %.1f,%.1f", ui, ui->x, ui->y, ui->w, ui->h, ui->focusable, x, y, pt.x, pt.y);
        focus_was_set = pt.x <= x && x < pt.x + ui->w && pt.y <= y && y < pt.y + ui->h;
        if (focus_was_set) { ui->a->focus(ui->a, ui); }
    }
//  traceln("focus_was_set=%d on %p", focus_was_set, ui);
    return focus_was_set;
}

static const ui_t ui_interface = {
    ui_init,
    ui_done,
    ui_add,
    ui_remove,
    ui_draw,
    ui_draw_children,
    ui_screen_xy,
    ui_mouse,
    ui_screen_mouse,
    ui_keyboard,
    ui_focus,
    /* kind: */ UI_KIND_CONTAINER,
};

const ui_t* ui_if = &ui_interface;

END_C
