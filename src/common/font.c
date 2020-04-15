#include "font.h"
#include "app.h"
#include "stb_rect_pack.h"
#include <GLES/gl.h>

BEGIN_C

static int32_t next_power_of_2(int32_t n) {
    n--; n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16;
    return n + 1;
}

static int pack_font_to_texture(font_t* f, const void* ttf, stbtt_packedchar* chars, int hpx, int from, int count) {
    assert(hpx >= 2);
    assert(f->data == null);
    int chars_bytes = sizeof(stbtt_packedchar) * count;
    int r = 0;
    const int np2 = next_power_of_2(hpx);
    // maximum number of bytes for hpx X hpx square cells
    const int bytes_needed = np2 * np2 * count;
    // below is heuristics that work well for LiberationMono-Bold.otf
    // for a single attempt on [32..127] range
    f->aw = next_power_of_2((int)sqrt(bytes_needed / 4));
    f->ah = f->aw / 2;
    bool done = true;
    do {
        deallocate(f->data);
        f->data = allocate(f->aw * f->ah); // 1 byte per pixel
//      traceln("trying atlas=%dx%d", f->aw, f->ah);
        if (f->data == null) {
            r = errno;
            done = true;
        } else {
            stbtt_pack_context pc = {};
            memset(chars, 0, chars_bytes);
            done = stbtt_PackBegin(&pc, f->data, f->aw, f->ah, 0, 1, null);
            if (done) {
//              stbtt_PackSetOversampling(&pc, 2, 2); // do NOT oversample font, default is 1,1
                done = stbtt_PackFontRange(&pc, (void*)ttf, 0, hpx, from, count, chars);
                stbtt_PackEnd(&pc);
            }
        }
        if (!done) {
            if (f->aw * f->ah <= bytes_needed) {
                if (f->aw <= f->ah) { f->aw *= 2; } else { f->ah *= 2; }
            } else {
                r = ENOMEM;
                done = true;
            }
        }
    } while (!done);
//  traceln("atlas=%dx%d r=%d", f->aw, f->ah, r);
    return r;
}

static int load_asset(font_t* f, app_t* a, const char* name, int hpx, int from, int count) {
    int r = 0;
    int bytes = 0;
    const void* data = null;
    void* asset = a->asset_map(a, name, &data, &bytes);
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
//      traceln("%s ascent=%.1f, descent=%.1f, line_gap=%d baseline=%.1f count=%d glyphs=%d",
//              name, f->ascent, f->descent, line_gap, f->baseline, count, f->fi.numGlyphs);
        int chars_bytes = sizeof(stbtt_packedchar) * count;
        stbtt_packedchar* chars = (stbtt_packedchar*)allocate(sizeof(stbtt_packedchar) * chars_bytes);
        if (chars == null) {
            r = errno;
        } else {
            f->chars = chars;
            r = pack_font_to_texture(f, data, chars, hpx, from, count);
            if (r == 0) { f->em = font_text_width(f, "M"); }
        }
        a->asset_unmap(a, asset, data, bytes);
    }
    if (r != 0) { font_dispose(f); }
    return r;
}

int font_load_asset(font_t* f, app_t* a, const char* name, int hpx, int from, int count) {
    assertion(f->data == null && f->chars == null && f->ti == 0,
             "bitmap already has data=%p chars=%p or texture=0x%08X or heigh in pixels too small %d",
              f->data, f->chars, f->ti, hpx);
    assertion(hpx >= 3 && 0 <= from && (count == -1 || count > 0),
             "invalid paramerers hpx=%d from=%d count=%d",
              hpx, from, count);
    int r = 0;
    if (f->data != null || f->chars != null || f->ti != 0 || hpx < 3 || from <= 0 || count == 0) {
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

int font_allocate_texture(font_t* f) {
    assertion(f->ti == 0, "texture already allocated ti=0x%08X", f->ti);
    int r = 0;
    if (f->ti != 0) {
        r = EINVAL;
    } else {
        GLuint ti = 0;
        glGenTextures(1, &ti);
        if (ti == 0) {
            r = ENOMEM;
        } else {
            f->ti = ti;
            assert(f->ti > 0);
            gl_init_texture(ti);
        }
    }
    return r;
}

int font_deallocate_texture(font_t* f) {
    if (f->ti != 0) { GLuint ti = f->ti; glDeleteTextures(1, &ti); f->ti = 0; }
    return 0;
}

int font_update_texture(font_t* f) {
    assert(f->data != null && f->chars != null && f->ti > 0);
    glBindTexture(GL_TEXTURE_2D, f->ti);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, f->aw, f->ah, 0, GL_ALPHA, GL_UNSIGNED_BYTE, f->data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return 0;
}

int font_allocate_and_update_texture(font_t* f) {
    int r = font_allocate_texture(f);
    return r != 0 ? r : font_update_texture(f);
}

void font_dispose(font_t* f) {
    assertion(f->ti == 0, "font_deallocate_texture() must be called on hidden() before font_dispose()");
    // Plan B: just in case font_dispose() called while window is still not hidden()
    if (f->ti != 0) { font_deallocate_texture(f); }
    deallocate(f->data);
    deallocate(f->chars);
    memset(f, 0, sizeof(*f));
}

float font_text_width(font_t* f, const char* text) {
    float x = 0;
    float y = 0;
    const int w = f->aw;
    const int h = f->ah;
    stbtt_packedchar* chars = (stbtt_packedchar*)f->chars;
    while (*text != 0) {
        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(chars, w, h, *text - f->from, &x, &y, &q, 0);
        text++;
    }
    return x;
}

float font_draw_text(font_t* f, float x, float y, const char* text) {
    int r = 0;
    int n = (int)strlen(text);
    GLfloat vertices[12 * n];
    GLfloat texture_coordinates[12 * n];
    GLfloat* pv = vertices;
    GLfloat* pt = texture_coordinates;
    gl_if_no_error(r, glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));
    gl_if_no_error(r, glBindTexture(GL_TEXTURE_2D, f->ti));
    const int w = f->aw;
    const int h = f->ah;
    stbtt_packedchar* chars = (stbtt_packedchar*)f->chars;
    while (*text != 0) {
        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(chars, w, h, *text - f->from, &x, &y, &q, 0);
        text++;
        const float x0 = q.x0, x1 = q.x1, y0 = q.y0, y1 = q.y1;
        const float s0 = q.s0, t0 = q.t0, s1 = q.s1, t1 = q.t1;
        *pv++ = x0; *pv++ = y0; *pv++ = x1; *pv++ = y0; *pv++ = x1; *pv++ = y1;
        *pv++ = x1; *pv++ = y1; *pv++ = x0; *pv++ = y1; *pv++ = x0; *pv++ = y0;
        *pt++ = s0; *pt++ = t0; *pt++ = s1; *pt++ = t0; *pt++ = s1; *pt++ = t1;
        *pt++ = s1; *pt++ = t1; *pt++ = s0; *pt++ = t1; *pt++ = s0; *pt++ = t0;
    }
    gl_if_no_error(r, gl_draw_texture_quads(vertices, texture_coordinates, n));
    gl_if_no_error(r, glBindTexture(GL_TEXTURE_2D, 0));
    assertion(r == 0, "failed %s", gl_strerror(r));
    return x;
}

END_C
