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
#include "glh.h"
#include "stb_inc.h"
#include "stb_image.h"

begin_c

static int load_asset(texture_t* b, app_t* a, const char* name) {
    int r = 0;
    const void* data = null;
    int bytes = 0;
    int w = 0;
    int h = 0;
    int bytes_per_pixel = 0;
    void* asset = sys.asset_map(a, name, &data, &bytes);
    if (asset == null) {
        r = errno;
    } else {
        byte* p = null;
        if (stbi_info_from_memory((const byte*)data, bytes, &w, &h, &bytes_per_pixel)) {
            p = stbi_load_from_memory((const byte*)data, bytes, &w, &h, &bytes_per_pixel, bytes_per_pixel);
        }
        if (p == null) {
            r = ENODATA;
        } else {
            b->w = w;
            b->h = h;
            b->comp = bytes_per_pixel;
            b->ti = 0;
            b->data = p;
        }
        sys.asset_unmap(a, asset, &data, bytes);
    }
    return r;
}

int texture_load_asset(texture_t* b, app_t* a, const char* name) {
    assertion(b->data == null && b->ti == 0,
              "bitmap already has data=%p or texture=0x%08X",
              b->data, b->ti);
    int r = 0;
    if (b->data != null && b->ti != 0) {
        r = EINVAL;
    } else {
        memset(b, 0, sizeof(*b));
        r = load_asset(b, a, name);
        if (r != 0) { texture_dispose(b); }
    }
    return r;
}

int texture_allocate(texture_t* b) {
    assertion(b->ti == 0, "texture already attached to bitmap ti=%d", b->ti);
    return b->ti != 0 ? EINVAL : gl_allocate(&b->ti);
}

int texture_deallocate(texture_t* b) {
    assertion(b->ti != 0, "texture was not attached to bitmap");
    int r = b->ti != 0 ? gl_delete_texture(b->ti) : EINVAL;
    b->ti = 0;
    return r;
}

int texture_update(texture_t* b) {
    return gl_update(b->ti, b->w, b->h, b->comp, b->data);
}

int texture_allocate_and_update(texture_t* b) {
    assert(b->data != null && b->w > 0 && b->h > 0 && b->comp > 0);
    int r = texture_allocate(b);
    return r != 0 ? r : texture_update(b);
}

void texture_dispose(texture_t* b) {
    assertion(b->ti == 0, "texture_deallocate() must be called on hidden() before texture_dispose()");
    if (b->ti != 0) { texture_deallocate(b); }
    if (b->data != null) { deallocate(b->data); }
    memset(b, 0, sizeof(*b));
}

end_c
