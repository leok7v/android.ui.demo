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

begin_c

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

static void ui_init(ui_t* u, ui_t* parent, void* that, float x, float y, float w, float h) {
    *u = *parent;
    u->that = that;
    u->parent = null;
    u->children = null;
    u->next = null;
    u->focusable = false;
    u->hidden = false;
    u->decor = false;
    ui.add(parent, u, x, y, w, h);
    u->kind = UI_KIND_CONTAINER;
}

static void ui_done(ui_t* u) {
    ui.remove(u->parent, u);
    memset(u, 0, sizeof(*u));
}

static void ui_draw_childs(ui_t* u, bool decor) {
    ui_t* c = u->children;
    while (c != null) {
        if (!c->hidden && !c->decor == !decor) { c->draw(c); }
        c = c->next;
    }
}

static void ui_draw_children(ui_t* u) {
    ui_draw_childs(u, false);
    ui_draw_childs(u, true);
}

static void ui_draw(ui_t* u) {
    ui_draw_children(u);
}

static bool ui_touch(ui_t* u, int touch_action, float x, float y) { return false; }

static void ui_screen_touch(ui_t* u, int touch_action, float x, float y) { }

static bool ui_dispatch_touch(ui_t* u, int touch_action, float x, float y) {
    ui_t* c = u->children;
    bool consumed = false;
    while (c != null && !consumed) {
        if (!c->hidden && c->x <= x && x < c->x + c->w && c->y <= y && y < c->y + c->h) {
            if (c->touch != null) { c->touch(c, touch_action, x - c->x, y - c->y); }
            consumed = ui_dispatch_touch(c, touch_action, x - c->x, y - c->y);
        }
        c = c->next;
    }
    return consumed;
}

static void ui_dispatch_screen_touch(ui_t* u, int touch_action, float x, float y) {
    ui_t* c = u->children;
    while (c != null) { // do it even for hidden widgets
        if (c->screen_touch != null) { c->screen_touch(c, touch_action, x, y); }
        ui_dispatch_screen_touch(c, touch_action, x, y);
        c = c->next;
    }
}

static bool ui_keyboard(ui_t* u, int flags, int ch) { return false; }

static void ui_focus(ui_t* u, bool gain) { }

static pointf_t ui_screen_xy(ui_t* u) {
    pointf_t pt = {u->x, u->y};
    ui_t* p = u->parent;
    while (p != null) { pt.x += p->x; pt.y += p->y; p = p->parent; }
    return pt;
}

static bool ui_set_focus(ui_t* u, int x, int y) {
    assert(u != null);
    ui_t* child = u->children;
    bool focus_was_set = false;
    while (child != null && !focus_was_set) {
        assert(child != u);
        focus_was_set = ui_set_focus(child, x, y);
        child = child->next;
    }
    if (!focus_was_set && u->focusable) {
        const pointf_t pt = ui.screen_xy(u);
        focus_was_set = pt.x <= x && x < pt.x + u->w && pt.y <= y && y < pt.y + u->h;
        if (focus_was_set) { u->a->focus(u->a, u); }
    }
    return focus_was_set;
}

const ui_interface_t ui = {
    ui_init,
    ui_done,
    ui_add,
    ui_remove,
    ui_screen_xy,
    ui_set_focus,
    ui_dispatch_touch,
    ui_dispatch_screen_touch
};

const ui_t ui_proto = {
    ui_draw,
    ui_draw_children,
    ui_touch,
    ui_screen_touch,
    ui_keyboard,
    ui_focus
};

end_c
