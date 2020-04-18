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
#include "bitmap.h"
#include "app.h"
#include "dc.h"
#include "stb_inc.h"
#include "stb_image.h"
#include <GLES/gl.h>

BEGIN_C

static int load_asset(bitmap_t* b, app_t* a, const char* name) {
    int r = 0;
    const void* data = null;
    int bytes = 0;
    int w = 0;
    int h = 0;
    int bytes_per_pixel = 0;
    void* asset = a->asset_map(a, name, &data, &bytes);
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
        a->asset_unmap(a, asset, &data, bytes);
    }
    return r;
}

int bitmap_load_asset(bitmap_t* b, app_t* a, const char* name) {
    assertion(b->data == null && b->ti == 0,
              "bitmap already has data=%p or texture=0x%08X",
              b->data, b->ti);
    int r = 0;
    if (b->data != null && b->ti != 0) {
        r = EINVAL;
    } else {
        memset(b, 0, sizeof(*b));
        r = load_asset(b, a, name);
        if (r != 0) { bitmap_dispose(b); }
    }
    return r;
}

int bitmap_allocate_texture(bitmap_t* b) {
    int r = 0;
    assertion(b->ti == 0, "texture already attached to bitmap ti=%d", b->ti);
    if (b->ti != 0) {
        r = EINVAL;
    } else {
        GLuint ti = 0;
        glGenTextures(1, &ti);
        // cannot call glGetError() here because it can be a) not in valid gl context, b) has accumulated previous errors
        if (ti == 0) {
            r = ENOMEM;
        } else {
            b->ti = ti;
            assertion(b->ti > 0, "need to change bitmap.ti type to uint32_t ti=0x%08X", ti);
            gl_init_texture(ti);
        }
    }
    return r;
}

int bitmap_deallocate_texture(bitmap_t* b) {
    int r = 0;
    assertion(b->ti != 0, "texture was not attached to bitmap");
    GLuint ti = b->ti;
    if (b->ti != 0) { glDeleteTextures(1, &ti); b->ti = 0; } else { r = EINVAL; }
    return r;
}

int bitmap_update_texture(bitmap_t* b) {
    int r = 0;
    int c = b->comp - 1;
    static const int formats[] = { GL_ALPHA, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
    assertion(0 <= c && c < countof(formats), "invalid number of components: %d", b->comp);
    if (0 <= c && c < countof(formats)) {
        int format = formats[c];
        gl_if_no_error(r, glBindTexture(GL_TEXTURE_2D, b->ti));
        gl_if_no_error(r, glTexImage2D(GL_TEXTURE_2D, 0, format, b->w, b->h, 0, format, GL_UNSIGNED_BYTE, b->data));
        gl_if_no_error(r, glBindTexture(GL_TEXTURE_2D, 0));
    } else {
        r = EINVAL;
    }
    return r;
}

int bitmap_allocate_and_update_texture(bitmap_t* b) {
    assert(b->data != null && b->w > 0 && b->h > 0 && b->comp > 0);
    int r = bitmap_allocate_texture(b);
    return r != 0 ? r : bitmap_update_texture(b);
}

void bitmap_dispose(bitmap_t* b) {
    assertion(b->ti == 0, "bitmap_deallocate_texture() must be called on hidden() before bitmap_dispose()");
    if (b->ti != 0) { bitmap_deallocate_texture(b); }
    if (b->data != null) { deallocate(b->data); }
    memset(b, 0, sizeof(*b));
}

END_C
