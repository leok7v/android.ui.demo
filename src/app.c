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
#include "btn.h" // btn is abstract touchable 'click==action' area

begin_c

static bool dispatch_keyboard_shortcuts(ui_t* u, int flags, int keycode) {
    bool consumed = false;
    if (u->kind == UI_KIND_BUTTON && !u->hidden) {
        btn_t* b = (btn_t*)u;
        int kc = isalpha(keycode) ? tolower(keycode) : keycode;
        int k = isalpha(b->key) ? tolower(b->key) : b->key;
        if (!u->hidden && b->key_flags == flags && k == kc) {
            // TODO: (Leo) if 3 (or more) states checkboxes are required this is the place to do it.
            //       b->flip = (b->flip + 1) % b->flip_wrap_around;
            if (b->flip != null) { *b->flip = !*b->flip; }
            u->a->invalidate(u->a);
            b->click(&b->u);
            return true; // stop search
        }
    }
    ui_t* c = u->children;
    while (c != null && !consumed) {
        if (!c->hidden) {
            consumed = dispatch_keyboard_shortcuts(c, flags, keycode);
        }
        c = c->next;
    }
    return consumed;
}

static bool dispatch_key_to_children(ui_t* u, int flags, int keycode) {
    bool consumed = false;
    ui_t* c = u->children;
    while (!consumed && c != null) {
        consumed = c->keyboard(c, flags, keycode);
        c = c->next;
    }
    consumed = consumed || u->keyboard(u, flags, keycode);
    return consumed;
}

bool app_dispatch_key(app_t* a, int flags, int keycode) {
    bool consumed = false;
    a->keyboard_flags = flags;
    // focused ui element gets first vote on further dispatch including keyboar shortcuts
    if (a->focused != null && a->focused->keyboard != null) {
        consumed = a->focused->keyboard(a->focused, flags, keycode);
    }
    // if key was not consumed each element in the root votes next
    // because e.g. it may have it's own keyboard shortcuts for actions
    consumed = consumed || dispatch_key_to_children(&a->root, flags, keycode);
    // and if it is still unconsumed walk the tree down to roots looking for btn shortcuts:
    if (!consumed && (flags & KEYBOARD_KEY_PRESSED)) {
        int f = flags & ~(KEYBOARD_KEY_PRESSED|KEYBOARD_SHIFT|KEYBOARD_NUMLOCK|KEYBOARD_CAPSLOCK);
        consumed = dispatch_keyboard_shortcuts(&a->root, f, keycode);
    }
    return consumed;
}

bool app_dispatch_touch(app_t* a, int index, int action, int x, int y) {
    bool consumed = false;
    if (index == 0) {
        if (action & TOUCH_DOWN) {
            if (!ui.set_focus(&a->root, x, y)) {
                a->focus(a, null); // kill focus if no focusable components were found
            }
        }
        ui.dispatch_screen_touch(&a->root, action, x, y);
        consumed = ui.dispatch_touch(&a->root, action, x, y);
    } else {
        // multi-touch gesture recognizer goes here
    }
    return consumed;
}

end_c
