/* Copyright 2020 "Leo" Dmitry Kuznetsov https://leok7v.github.io/
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */
#include "edit.h"
#include "app.h"

// inspired by: https://github.com/landley/toybox/blob/cae14933a6b32bc7260964439f9def316c7520ba/toys/pending/vi.c

begin_c

typedef struct edit_chunk_s {
    edit_chunk_t* next;
    edit_chunk_t* prev;
    int   count; // number of bytes in a chunk
    byte* text;
    int   bytes; // number of allocated bytes in a chunk >= count
    int   kind;  // 0 - readonly, 1 - heap, 2 - stack/static
    int   pos;   // only valid after locate() for located chunk and all chunks before it
} edit_chunk_t;

/*  utf-8    // https://en.wikipedia.org/wiki/UTF-8
    Code    Continuation Minimum Maximum
    0xxxxxxx           0       0     127
    10xxxxxx       error
    110xxxxx           1     128    2047
    1110xxxx           2    2048   65535 excluding 55296 - 57343 (0xD800 – 0xDFFF)
    11110xxx           3   65536 1114111 excluding 65534 and 65535 (0xFFFE – 0xFFFF)
    11111xxx       error
*/

#define is_surrogate(c) (0xD800 <= (c) && (c) <= 0xDFFF) // utf-16 surrogates
#define is_bom(c) (0xFFFE <= (c) && (c) <= 0xFFFF) // BOM byte order mark 0xFEFF or 0xFFFE

static int utf8_decode(byte** s, int count) { // -1 error or not enought data
    if (count <= 0) { return -1; }
    byte c0 = (*s)[0];
    if ((c0 & ~0x7F) == 0) { s += 1; return c0; }
    if (count <= 1) { return -1; }
    byte c1 = (*s)[1];
    if ((c1 & 0xC0) != 0x80) { return -1; }
    int c = ((c0 & 0x1F) << 5)  | (c1 & 0x3F);
    if ((c0 & 0xE0) == 0xC0) { s += 2; return c; }
    if (count <= 2) { return -1; }
    byte c2 = (*s)[2];
    if ((c2 & 0xC0) != 0x80) { return -1; }
    c = (c << 6) | (c2 & 0x3F);
    if ((c0 & 0xF0) == 0xE0) { s += 3; return c < 2048 || is_surrogate(c) || is_bom(c) ? -1: c; }
    if (count <= 3) { return -1; }
    byte c3 = (*s)[3];
    if ((c3 & 0xC0) != 0x80) { return -1; }
    if ((c0 & 0xF8) == 0xF0) { s += 4; return c < 65536 ? -1 : c; }
    return -1;
}

static void read_text(edit_t* e, edit_chunk_t* c, char* text, int position, int bytes) {
    while (c != null && c->pos > position) { c = c->prev; } // roll backward
    if (c == null) { c = e->chunks; assert(e->chunks->pos == 0); } // start from the very beginning
    int p = c->pos;
    while (c != null && p < position + bytes) {
        c->pos = p;
        e->last_chunk = c; // last chunk with valid .pos
        if (p <= position && position + bytes < p + c->count) {
            // text is completely inside single chunk
            int from = position - p;
            memcpy(text, c->text + from, bytes);
            break;
        } else if (position <= p && p + c->count <= position + bytes) {
            // chunk is completely inside text
            memcpy(text + (p - position), c->text, c->count);
        } else if (p <= position && p + c->count <= position + bytes ) {
            // start of text is inside chunk
            int offset = position - p;
            int n = c->count - offset;
            memcpy(text, c->text + offset, n);
        } else if (p <= position + bytes && position + bytes <= p + c->count) {
            // end of text is inside chunk
            int n = position + bytes - p;
            int offset = position - bytes - n;
            memcpy(text + offset, c->text, n);
        } else {
            assertion(false, "should not ever be here");
        }
        p += c->count;
        c = c->next;
    }
}

static edit_chunk_t* locate(edit_t* e, int pos) {
    edit_chunk_t* c = e->last_chunk;
    while (c != null && c->pos > pos) { c = c->prev; }
    if (c == null) { c = e->chunks; assert(e->chunks->pos == 0); }
    int p = c->pos;
    while (c != null) {
        c->pos = p;
        e->last_chunk = c; // last chunk with valid .pos
        if (p + c->count > pos) { break; }
        p += c->count;
        c = c->next;
    }
    assert(c != null);
    return c;
}

static edit_chunk_t* next_eol(edit_t* e, edit_chunk_t* c, int* position) {
    int pos = *position + 1;
    while (c != null) {
        int i = pos - c->pos;
        while (i < c->count && c->text[i] != '\n') { i++; }
        if (i < c->count) { assert(c->text[i] == '\n'); *position = i + 1; break; }
        edit_chunk_t* n = c->next;
        if (n != null) {
            n->pos = c->pos + c->count;
            e->last_chunk = n;
        }
        c = n;
    }
    return c; // may be null at end of text
}

static bool edit_touch(ui_t* u, int touch_action, float x, float y) {
    edit_t* e = (edit_t*)u;
    bool consumed = false;
    if (e->u.focusable && (touch_action & TOUCH_DOWN) != 0) {
        consumed = true;
    }
    return consumed;
}

static void edit_focus(ui_t* u, bool gain) {
//  edit_t* e = (edit_t*)u;
    traceln("focused=%d", gain);
}

static bool edit_keyboard(ui_t* u, int flags, int key) {
    bool consumed = false;
//  edit_t* e = (edit_t*)u;
    traceln("flags=0x%08X key=%d 0x%02X %c", flags, key, key, key);
    return consumed;
}

static void edit_draw(ui_t* u) {
    edit_t*  e = (edit_t*)u;
    app_t*   a = u->a;
    theme_t* t = &a->theme;
    font_t*  f = t->font;
    int y = u->y;
    int n = (u->w + f->em - 1) / f->em;
    char text[n + 1];
    int pos = e->screen;
    edit_chunk_t* c = locate(e, pos);
    while (c != null && y < u->y + u->h) {
        int eol = pos;
        edit_chunk_t* next = next_eol(e, c, &eol);
        int m = min(n, next == null ? c->pos + c->count - pos : eol - pos);
        edit_read(e, text, pos, m); // TODO: can be optimized to start from chunk "c"
        if (m > 0 && text[m] == '\n') { m--; } // do not draw '\n'
        if (m > 0 && text[m] == '\r') { m--; } // do not draw '\r'
        dc.text(&dc, t->color_text, f, u->x, y, text, m);
        y += f->height;
        pos = eol;
        c = next;
    }
}

int edit_bytes(edit_t* e) {
    int bytes = 0;
    edit_chunk_t* c = e->chunks;
    while (c != null) {
        byte* p = c->text; // sanity check - not necessary
        assertion(utf8_decode(&p, c->count) != -1, "invalid utf8");
        bytes += c->count;
        c = c->next;
    }
    return bytes;
}

void edit_read(edit_t* e, char* text, int position, int bytes) {
    read_text(e, e->last_chunk, text, position, bytes);
}

void edit_init(edit_t* e, ui_t* parent, void* that, float x, float y, float w, float h) {
    assert((void*)e == (void*)&e->u);
    e->u = *parent;
    e->u.that = that;
    e->u.parent = null;
    e->u.children = null;
    e->u.next = null;
    e->u.focusable = true;
    e->u.hidden = false;
    assert(e->last_chunk == null);
    app_t*   a = e->u.a;
    theme_t* t = &a->theme;
    font_t*  f = t->font;
    h = (h / f->height) * f->height;
    assertion(h > 0, "height must be at least f->height");
    parent->add(parent, &e->u, x, y, w, h);
    assert(e->u.parent == parent);
    e->u.kind = UI_KIND_EDIT;
    e->u.draw = edit_draw;
    e->u.touch = edit_touch;
    e->u.keyboard = edit_keyboard;
    e->u.focus = edit_focus;
    e->chunks = allocate(sizeof(edit_chunk_t));
    static const byte empty[1]; // simplifies null condition testing
    e->chunks->text = e->text == null ? (byte*)empty : (byte*)e->text;
    e->chunks->count = e->bytes;
    assert(e->chunks->bytes == 0); // was not heap allocated
    assert(e->chunks->kind == 0); // read only
    assert(e->chunks->prev == null && e->chunks->next == null); // allocate() zeros memory
}

void edit_done(edit_t* e) {
    edit_chunk_t* c = e->chunks;
    while (c != null) {
        edit_chunk_t* n = c->next;
        deallocate(c);
        c = n;
    }
    memset(e, 0, sizeof(*e));
}

end_c
