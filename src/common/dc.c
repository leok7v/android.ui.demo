#include "dc.h"
#include "glh.h"
#include "shaders.h"
#ifndef  MAKE_USE_OF_GLES3
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#else
#include <GLES/gl.h>
#include <GLES3/gl3.h>
#endif

BEGIN_C

static void init(dc_t* dc, mat4x4 mvp, ...);
static void dispose(dc_t* dc);
static void clear(dc_t* dc, colorf_t* color);
static void fill(dc_t* dc, colorf_t* color, float x, float y, float w, float h);
static void rect(dc_t* dc, colorf_t* color, float x, float y, float w, float h, float width);
static void ring(dc_t* dc, colorf_t* color, float x, float y, float radius, float inner);
static void bblt(dc_t* dc, bitmap_t* bitmap, float x, float y);
static void luma(dc_t* dc, colorf_t* color, bitmap_t* bitmap, float x, float y, float w, float h);
static void quad(dc_t* dc, colorf_t* color, bitmap_t* bitmap, quadf_t* quads, int count);
static void poly(dc_t* dc, colorf_t* color, pointf_t* vertices, int count);

dc_t dc = {
    init,
    dispose,
    clear,
    fill,
    rect,
    ring,
    bblt,
    luma,
    quad,
    poly
};

static void init(dc_t* dc, mat4x4 mvp, ...) {
    assert(sizeof(GLuint) == sizeof(uint32_t));
    assert(sizeof(GLint) == sizeof(int32_t));
    assert(sizeof(GLushort) == sizeof(uint16_t));
    assert(sizeof(GLshort) == sizeof(int16_t));
    assert(sizeof(GLfloat) == sizeof(float));
    assert(sizeof(GLchar) == sizeof(char) && sizeof(GLbyte) == sizeof(char) && sizeof(GLubyte) == sizeof(byte));
    assert(sizeof(GLsizei) == sizeof(int));
    assert(sizeof(GLintptr) == sizeof(uintptr_t));
    assert(sizeof(GLsizeiptr) == sizeof(GLsizeiptr));
    // in gles 3.2 we are better of with glGenSamplers(), glSamplerParameter and glBindSampler(texture unit)
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    gl_check(glEnableVertexAttribArray(0));
    assert(sizeof(dc->mvp) == 16 * sizeof(float));
    memcpy(dc->mvp, mvp, sizeof(dc->mvp));
}

static void dispose(dc_t* dc) {
}

static void clear(dc_t* dc, colorf_t* color) {
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

static void fill(dc_t* dc, colorf_t* color, float x, float y, float w, float h) {
    const float vertices[] = { x, y, x + w, y,  x + w, y + h, x, y + h };
    use_program(shaders.fill);
    gl_check(glUniformMatrix4fv(shaders.fill_mvp, 1, false, (GLfloat*)dc->mvp));
    gl_check(glUniform4fv(shaders.fill_rgba, 1, (GLfloat*)color));
    gl_check(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices));
    gl_check(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
}

static void rect(dc_t* dc, colorf_t* color, float x, float y, float w, float h, float thickness) {
    assert(0 < thickness && thickness <= min(w, h)); // use fill() for thickness out of this range
    fill(dc, color, x, y, w, thickness);
    fill(dc, color, x, y + h - thickness, w, thickness);
    fill(dc, color, x, y, thickness, h);
    fill(dc, color, x + w - thickness, y, thickness, h);
}

static void ring(dc_t* dc, colorf_t* color, float x, float y, float radius, float inner) {
    assert(inner < radius);
    float x0 = x - radius;
    float y0 = y - radius;
    float x1 = x + radius;
    float y1 = y + radius;
    float ri = inner / radius;
    GLfloat vertices[] = {
        x0, y0,  0, 0,
        x1, y0,  1, 0,
        x1, y1,  1, 1,
        x0, y1,  0, 1 };
    use_program(shaders.ring);
    gl_check(glUniformMatrix4fv(shaders.ring_mvp, 1, false, (GLfloat*)dc->mvp));
    gl_check(glUniform4fv(shaders.ring_rgba, 1, (GLfloat*)color));
    // outter and inner radius (inclusive) squared:
    gl_check(glUniform1f(shaders.ring_ro2, 1.0)); // outer radius is 1.0 ^ 2 = 1.0
    gl_check(glUniform1f(shaders.ring_ri2, ri * ri));
    gl_check(glEnableVertexAttribArray(0));
    gl_check(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertices));
    gl_check(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
}

static void bblt(dc_t* dc, bitmap_t* bitmap, float x, float y) {
    int ti = bitmap->ti;
    int tw = bitmap->w;
    int th = bitmap->h;
    GLfloat vertices[] = {
        x,      y,       0, 0,
        x + tw, y,       1, 0,
        x + tw, y + th,  1, 1,
        x,      y + th,  0, 1
    };
    use_program(shaders.bblt);
    gl_check(glUniformMatrix4fv(shaders.bblt_mvp, 1, false, (GLfloat*)dc->mvp));
    gl_check(glUniform1i(shaders.bblt_tex, 1)); // index(!) of GL_TEXTURE1 below:
    gl_check(glActiveTexture(GL_TEXTURE1));
    gl_check(glBindTexture(GL_TEXTURE_2D, ti));
    gl_check(glEnableVertexAttribArray(0));
    gl_check(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertices));
    gl_check(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
}

static void luma(dc_t* dc, colorf_t* color, bitmap_t* bitmap, float x, float y, float w, float h) {
    int tw = bitmap->w;
    int th = bitmap->h;
    GLfloat vertices[] = {
        x,      y,       0, 0,
        x + tw, y,       1, 0,
        x + tw, y + th,  1, 1,
        x,      y + th,  0, 1
    };
    use_program(shaders.luma);
    gl_check(glUniformMatrix4fv(shaders.luma_mvp, 1, false, (GLfloat*)dc->mvp));
    gl_check(glUniform4fv(shaders.luma_rgba, 1, (GLfloat*)color));
    gl_check(glUniform1i(shaders.luma_tex, 1)); // index(!) of GL_TEXTURE1 below
    gl_check(glActiveTexture(GL_TEXTURE1));
    gl_check(glBindTexture(GL_TEXTURE_2D, bitmap->ti));
    gl_check(glEnableVertexAttribArray(0));
    gl_check(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertices));
    gl_check(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
}

static void quad(dc_t* dc, colorf_t* color, bitmap_t* bitmap, quadf_t* quads, int count) {
    use_program(shaders.luma);
    gl_check(glUniformMatrix4fv(shaders.luma_mvp, 1, false, (GLfloat*)dc->mvp));
    gl_check(glUniform4fv(shaders.luma_rgba, 1, (GLfloat*)color));
    gl_check(glUniform1i(shaders.luma_tex, 1)); // index(!) of GL_TEXTURE1 below
    gl_check(glActiveTexture(GL_TEXTURE1));
    gl_check(glBindTexture(GL_TEXTURE_2D, bitmap->ti));
    gl_check(glEnableVertexAttribArray(0));
    gl_check(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (GLfloat*)quads));
    gl_check(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
}

static void poly(dc_t* dc, colorf_t* color, pointf_t* vertices, int count) {
    use_program(shaders.fill);
    gl_check(glUniformMatrix4fv(shaders.fill_mvp, 1, false, (GLfloat*)dc->mvp));
    gl_check(glUniform4fv(shaders.fill_rgba, 1, (GLfloat*)color));
    gl_check(glEnableVertexAttribArray(0));
    gl_check(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices));
    gl_check(glDrawArrays(GL_TRIANGLE_FAN, 0, count));
}

END_C
