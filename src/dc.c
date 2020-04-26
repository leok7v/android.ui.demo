#include "dc.h"
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
#include "glh.h"
#include "shaders.h"
#include "font.h"
#include <GLES/gl.h>
#include <GLES3/gl3.h>

BEGIN_C

static void init(dc_t* dc);
static void viewport(dc_t* dc, float x, float y, float w, float h);
static void dispose(dc_t* dc);
static void clear(dc_t* dc, const colorf_t* color);
static void fill(dc_t* dc, const colorf_t* color, float x, float y, float w, float h);
static void rect(dc_t* dc, const colorf_t* color, float x, float y, float w, float h, float width);
static void ring(dc_t* dc, const colorf_t* color, float x, float y, float radius, float inner);
static void bblt(dc_t* dc, const bitmap_t* bitmap, float x, float y);
static void luma(dc_t* dc, const colorf_t* color, bitmap_t* bitmap, float x, float y);
static void tex4(dc_t* dc, const colorf_t* color, bitmap_t* bitmap, quadf_t* quads, int count);
static void poly(dc_t* dc, const colorf_t* color, const pointf_t* vertices, int count);
static void line(dc_t* dc, const colorf_t* c, float x0, float y0, float x1, float y1, float thickness);
static float text(dc_t* dc, const colorf_t* color, font_t* font, float x, float y, const char* text, int count);
static void quadrant(dc_t* dc, const colorf_t* color, float x, float y, float r, int q);
static void stadium(dc_t* dc, const colorf_t* color, float x, float y, float w, float h, float r);

static void orthographic_projection_2d(mat4x4 m, float x, float y, float w, float h);

dc_t dc = {
    init,
    viewport,
    dispose,
    clear,
    fill,
    rect,
    ring,
    bblt,
    luma,
    tex4,
    poly,
    line,
    text,
    quadrant,
    stadium,
};

static void init(dc_t* dc) {
    const char* version = (const char*)glGetString(GL_VERSION); (void)version;
    int major = 0;
    int minor = 0;
#if defined(GL_MAJOR_VERSION) && defined(GL_MINOR_VERSION) // GLES3
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    bool got_version = glGetError() == 0;
    while (glGetError() != 0) { }
#else
    bool got_version = false;
#endif
    // calls do fail on GL ES 2.0:
    if (!got_version) {
        const char* p = version;
        while (*p != 0 && !isdigit(*p)) { p++; }
        if (p != 0) { sscanf(p, "%d.%d", &major, &minor); }
    }
//  traceln("GL_VERSION=%d.%d %s", major, minor, version);
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    gl_check(glEnable(GL_BLEND));
    gl_check(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    gl_check(glDisable(GL_DEPTH_TEST));
    gl_check(glDisable(GL_CULL_FACE));
    gl_check(glEnableVertexAttribArray(0));
}

static void viewport(dc_t* dc, float x, float y, float w, float h) {
    orthographic_projection_2d(dc->mvp, x, y, w, h);
    gl_check(glViewport(x, y, w, h));
}

static void dispose(dc_t* dc) {
}

static void clear(dc_t* dc, const colorf_t* color) {
    if (color->a != 0) {
        gl_check(glClearColor(color->r, color->g, color->b, color->a));
        gl_check(glClear(GL_COLOR_BUFFER_BIT));
    }
}

static void use_program(int program) {
#ifdef DEBUG
    gl_check(glValidateProgram(program));
    GLint status = 0;
    gl_check(glGetProgramiv(program, GL_VALIDATE_STATUS, &status));
    if (!status) {
        GLsizei count = 0;
        char message[1024] = {};
        glGetProgramInfoLog(program, countof(message) - 1, &count, message);
        traceln("%s", message);
        exit(1);
    }
#endif
    gl_check(glUseProgram(program));
}

static void fill(dc_t* dc, const colorf_t* color, float x, float y, float w, float h) {
    const float vertices[] = { x, y, x + w, y,  x + w, y + h, x, y + h };
    use_program(shaders.fill);
    gl_check(glUniformMatrix4fv(shaders.fill_mvp, 1, false, (GLfloat*)dc->mvp));
    gl_check(glUniform4fv(shaders.fill_rgba, 1, (GLfloat*)color));
    gl_check(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices));
    gl_check(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
}

static void rect(dc_t* dc, const colorf_t* color, float x, float y, float w, float h, float thickness) {
    assert(0 < thickness && thickness <= min(w, h)); // use fill() for thickness out of this range
    fill(dc, color, x, y, w, thickness);
    fill(dc, color, x, y + h - thickness, w, thickness);
    fill(dc, color, x, y, thickness, h);
    fill(dc, color, x + w - thickness, y, thickness, h);
}

static void ring(dc_t* dc, const colorf_t* color, float x, float y, float radius, float inner) {
    assert(inner < radius);
    const float x0 = x - radius;
    const float y0 = y - radius;
    const float x1 = x + radius;
    const float y1 = y + radius;
    const float ri = inner / radius;
    const GLfloat vertices[] = {
        x0, y0,  -1, -1,
        x1, y0,   1, -1,
        x1, y1,   1,  1,
        x0, y1,  -1,  1 };
    use_program(shaders.ring);
    gl_check(glUniformMatrix4fv(shaders.ring_mvp, 1, false, (GLfloat*)dc->mvp));
    gl_check(glUniform4fv(shaders.ring_rgba, 1, (GLfloat*)color));
    // outter and inner radius (inclusive) squared:
    gl_check(glUniform1f(shaders.ring_ro2, 1.0)); // outer radius is 1.0 ^ 2 = 1.0
    gl_check(glUniform1f(shaders.ring_ri2, ri * ri));
    gl_check(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertices));
    gl_check(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
}

static void quadrant(dc_t* dc, const colorf_t* color, float x, float y, float r, int q) {
    float r2 = r * 2 / sqrt(2);
    int sx[4] = { +r2, +r2, -r2, -r2 };
    int sy[4] = { -r2, +r2, +r2, -r2 };
    float dx = sx[q & 0x3];
    float dy = sy[q & 0x3];
    const GLfloat vertices[] = {
        x     , y     ,  0, 0,
        x     , y + dy,  0, 1,
        x + dx, y     ,  1, 0 };
    use_program(shaders.ring);
    gl_check(glUniformMatrix4fv(shaders.ring_mvp, 1, false, (GLfloat*)dc->mvp));
    gl_check(glUniform4fv(shaders.ring_rgba, 1, (GLfloat*)color));
    // outter and inner radius (inclusive) squared:
    gl_check(glUniform1f(shaders.ring_ro2, 0.5)); // outer radius^2
    gl_check(glUniform1f(shaders.ring_ri2, 0));
    gl_check(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertices));
    gl_check(glDrawArrays(GL_TRIANGLES, 0, 3));
}

// static inline_c float pow2(float v) { return v * v; }

static void stadium(dc_t* dc, const colorf_t* c, float x, float y, float w, float h, float r) {
    float x0 = x + r;
    float y0 = y + r;
    float x1 = x - r + w;
    float y1 = y - r + h;
    pointf_t qs[] = { { x1, y0 }, { x1, y1 }, { x0, y1 }, { x0, y0 } }; // quadrants
    for (int q = 0; q < 4; q++) {
        dc->quadrant(dc, c, qs[q].x, qs[q].y, r, q);
    }
    float r2 = r * 2;
    dc->fill(dc, c, x0, y, w - r2, r);
    dc->fill(dc, c, x0, y1, w - r2, r);
    dc->fill(dc, c, x, y0, w, h - r2);
}

static void bblt(dc_t* dc, const bitmap_t* bitmap, float x, float y) {
    const int w = bitmap->w;
    const int h = bitmap->h;
    const GLfloat vertices[] = {
        x,     y,      0, 0,
        x + w, y,      1, 0,
        x + w, y + h,  1, 1,
        x,     y + h,  0, 1
    };
    use_program(shaders.bblt);
    gl_check(glUniformMatrix4fv(shaders.bblt_mvp, 1, false, (GLfloat*)dc->mvp));
    gl_check(glUniform1i(shaders.bblt_tex, 1)); // index(!) of GL_TEXTURE1 below:
    gl_check(glActiveTexture(GL_TEXTURE1));
    gl_check(glBindTexture(GL_TEXTURE_2D, bitmap->ti));
    gl_check(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertices));
    gl_check(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
}

static void luma(dc_t* dc, const colorf_t* color, bitmap_t* bitmap, float x, float y) {
    const float w = bitmap->w;
    const float h = bitmap->h;
    GLfloat vertices[] = {
        x,     y,      0, 0,
        x + w, y,      1, 0,
        x + w, y + h,  1, 1,
        x,     y + h,  0, 1
    };
    use_program(shaders.luma);
    gl_check(glUniformMatrix4fv(shaders.luma_mvp, 1, false, (GLfloat*)dc->mvp));
    gl_check(glUniform4fv(shaders.luma_rgba, 1, (GLfloat*)color));
    gl_check(glUniform1i(shaders.luma_tex, 1)); // index(!) of GL_TEXTURE1 below
    gl_check(glActiveTexture(GL_TEXTURE1));
    gl_check(glBindTexture(GL_TEXTURE_2D, bitmap->ti));
    gl_check(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertices));
    gl_check(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
}

static void tex4(dc_t* dc, const colorf_t* color, bitmap_t* bitmap, quadf_t* quads, int count) {
    use_program(shaders.luma);
    gl_check(glUniformMatrix4fv(shaders.luma_mvp, 1, false, (GLfloat*)dc->mvp));
    gl_check(glUniform4fv(shaders.luma_rgba, 1, (GLfloat*)color));
    gl_check(glUniform1i(shaders.luma_tex, 1)); // index(!) of GL_TEXTURE1 below
    gl_check(glActiveTexture(GL_TEXTURE1));
    gl_check(glBindTexture(GL_TEXTURE_2D, bitmap->ti));
    for (int i = 0; i < count; i++) {
        gl_check(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (GLfloat*)&quads[i * 4]));
        gl_check(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
    }
}

static void poly(dc_t* dc, const colorf_t* color, const pointf_t* vertices, int count) {
    use_program(shaders.fill);
    gl_check(glUniformMatrix4fv(shaders.fill_mvp, 1, false, (GLfloat*)dc->mvp));
    gl_check(glUniform4fv(shaders.fill_rgba, 1, (GLfloat*)color));
    gl_check(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices));
    gl_check(glDrawArrays(GL_TRIANGLE_FAN, 0, count));
}

static void line(dc_t* dc, const colorf_t* c, float x0, float y0, float x1, float y1, float thickness) {
    if (x0 == x1 || y0 == y1) {
        const int x = min(x0, x1);
        const int y = min(y0, y1);
        const int w = max(x0, x1) - x;
        const int h = max(y0, y1) - y;
        if (h == 0) {
            dc->fill(dc, c, x, y, w, thickness);
        } else {
            assert(w == 0);
            dc->fill(dc, c, x, y, thickness, h);
        }
    } else {
        float dx = x1 - x0;
        float dy = y1 - y0;
        const float d = sqrt(dx * dx + dx * dy);
        dx *= thickness / d;
        dy *= thickness / d;
        const pointf_t vertices[] = {
            { x0 - dy, y0 + dx },
            { x1 - dy, y1 + dx },
            { x1 + dy, y1 - dx },
            { x0 + dy, y0 - dx }
        };
        dc->poly(dc, c, vertices, 4);
    }
}

static float text(dc_t* dc, const colorf_t* c, font_t* f, float x, float y, const char* text, int n) {
    quadf_t quads[n * 4];
    const int w = f->atlas.w;
    const int h = f->atlas.h;
    stbtt_packedchar* chars = (stbtt_packedchar*)f->chars;
    int k = 0;
    for (int i = 0; i < n; i++) {
        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(chars, w, h, text[i] - f->from, &x, &y, &q, 0);
        quadf_t q0 = {q.x0, q.y0, q.s0, q.t0}; quads[k++] = q0;
        quadf_t q1 = {q.x1, q.y0, q.s1, q.t0}; quads[k++] = q1;
        quadf_t q2 = {q.x1, q.y1, q.s1, q.t1}; quads[k++] = q2;
        quadf_t q3 = {q.x0, q.y1, q.s0, q.t1}; quads[k++] = q3;
    }
    dc->tex4(dc, c, &f->atlas, quads, n);
    return x;
}

static void orthographic_projection_2d(mat4x4 m, float x, float y, float w, float h) {
    const float znear = -1;
    const float zfar  =  1;
    const float inv_z =  1 / (zfar - znear);
    const float inv_x =  1 / w;
    const float inv_y = -1 / h;
    // first column:
    m[0][0] = 2 * inv_x;
    m[1][0] = 0;
    m[2][0] = 0;
    m[3][0] = 0;
    // second column:
    m[0][1] = 0;
    m[1][1] = inv_y * 2;
    m[2][1] = 0;
    m[3][1] = 0;
    // third column:
    m[0][2] = 0;
    m[1][2] = 0;
    m[2][2] = inv_z * -2;
    m[3][2] = 0;
    // forth column:
    m[0][3] = -(x + x + w) * inv_x;
    m[1][3] = -(y + y + h) * inv_y;
    m[2][3] = -(zfar + znear) * inv_z;
    m[3][3] = 1;
}

END_C
