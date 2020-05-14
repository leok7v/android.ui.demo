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
#include "font.h"
#include "app.h"
#include "stb_rect_pack.h"

begin_c

static int32_t next_power_of_2(int32_t n) {
    n--; n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16;
    return n + 1;
}

static int pack_font_to_texture(font_t* f, const void* ttf, stbtt_packedchar* chars, int hpx, int from, int count) {
    assert(hpx >= 2);
    assert(f->atlas.data == null);
    int chars_bytes = sizeof(stbtt_packedchar) * count;
    int r = 0;
    const int np2 = next_power_of_2(hpx);
    // maximum number of bytes for hpx X hpx square cells
    const int bytes_needed = np2 * np2 * count;
    // below is heuristics that work well for LiberationMono-Bold.otf
    // for a single attempt on [32..127] range
    f->atlas.w = next_power_of_2((int)sqrt(bytes_needed / 4));
    f->atlas.h = f->atlas.w / 2;
    f->atlas.comp = 1;
    bool done = true;
    do {
        deallocate(f->atlas.data);
        f->atlas.data = allocate(f->atlas.w * f->atlas.h); // 1 byte per pixel
        if (f->atlas.data == null) {
            r = errno;
            done = true;
        } else {
            stbtt_pack_context pc = {};
            memset(chars, 0, chars_bytes);
            done = stbtt_PackBegin(&pc, f->atlas.data, f->atlas.w, f->atlas.h, 0, 1, null);
            if (done) {
                done = stbtt_PackFontRange(&pc, (void*)ttf, 0, hpx, from, count, chars);
                stbtt_PackEnd(&pc);
            }
        }
        if (!done) {
            if (f->atlas.w * f->atlas.h <= bytes_needed) {
                if (f->atlas.w <= f->atlas.h) { f->atlas.w *= 2; } else { f->atlas.h *= 2; }
            } else {
                r = ENOMEM;
                done = true;
            }
        }
    } while (!done);
    return r;
}

static int load_asset(font_t* f, app_t* a, const char* name, int hpx, int from, int count) {
    int r = 0;
    int bytes = 0;
    const void* data = null;
    void* asset = sys.asset_map(a, name, &data, &bytes);
    assertion(asset != null, "asset \"%s\"not found", name);
    if (asset == null) {
        r = errno;
    } else {
        f->height = hpx;
        stbtt_InitFont(&f->fi, data, 0);
        int ascent = 0;
        int descent = 0;
        int line_gap = 0;
        float scale = stbtt_ScaleForPixelHeight(&f->fi, hpx);
        stbtt_GetFontVMetrics(&f->fi, &ascent, &descent, &line_gap);
        if (count <= 0) { count = f->fi.numGlyphs; }
        f->ascent = (int)(ascent * scale);
        f->descent = (int)(descent * scale);
        f->baseline = (int)(ascent * scale);
        f->from = from;
        f->count = count;
        int chars_bytes = sizeof(stbtt_packedchar) * count;
        stbtt_packedchar* chars = (stbtt_packedchar*)allocate(sizeof(stbtt_packedchar) * chars_bytes);
        if (chars == null) {
            r = errno;
        } else {
            f->chars = chars;
            r = pack_font_to_texture(f, data, chars, hpx, from, count);
            if (r == 0) { f->em = font_text_width(f, "M", 1); }
        }
        sys.asset_unmap(a, asset, data, bytes);
    }
    if (r != 0) { font_dispose(f); }
    return r;
}

int font_load_asset(font_t* f, app_t* a, const char* name, int hpx, int from, int count) {
    assertion(f->atlas.data == null && f->chars == null && f->atlas.ti == 0,
             "bitmap already has data=%p chars=%p or texture=0x%08X or heigh in pixels too small %d",
              f->atlas.data, f->chars, f->atlas.ti, hpx);
    assertion(hpx >= 3 && 0 <= from && (count == -1 || count > 0),
             "invalid paramerers hpx=%d from=%d count=%d",
              hpx, from, count);
    int r = 0;
    if (f->atlas.data != null || f->chars != null || f->atlas.ti != 0 || hpx < 3 || from <= 0 || count == 0) {
        r = EINVAL;
    } else {
        memset(f, 0, sizeof(*f));
        r = load_asset(f, a, name, hpx, from, count);
        if (r != 0) { font_dispose(f); }
    }
    return r;
}

int font_find_glyph_index(font_t* f, int unicode_codepoint) {
    int ix = stbtt_FindGlyphIndex(&f->fi, unicode_codepoint);
    if (ix != 0) {
        ix = -1; errno = ENODATA;
    } else {
        assertion(ix > f->from, "ix=%d from=%d count=%d", ix, f->from, f->count);
        ix = ix - f->from;
    }
    return ix;
}

void font_dispose(font_t* f) {
    assertion(f->atlas.ti == 0, "font_deallocate() must be called on hidden() before font_dispose()");
    // Plan B: just in case font_dispose() called while window is still not hidden()
    if (f->atlas.ti != 0) { texture_deallocate(&f->atlas); }
    deallocate(f->atlas.data);
    deallocate(f->chars);
    memset(f, 0, sizeof(*f));
}

float font_text_width(font_t* f, const char* text, int count) {
    if (count < 0) { count = strlen(text); }
    float x = 0;
    float y = 0;
    const int w = f->atlas.w;
    const int h = f->atlas.h;
    stbtt_packedchar* chars = (stbtt_packedchar*)f->chars;
    for (int i = 0; i < count; i++) {
        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(chars, w, h, text[i] - f->from, &x, &y, &q, 0);
    }
    return x;
}

end_c
