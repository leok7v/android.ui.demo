#include "dc.h"
#include "glh.h"
#include "shaders.h"
#include <GLES/gl.h>
#include <GLES2/gl2.h>

BEGIN_C

static void init(dc_t* dc, mat4x4 mvp, ...);
static void dispose(dc_t* dc);
static void clear(dc_t* dc, colorf_t* color);
static void fill(dc_t* dc, colorf_t* color, float x, float y, float w, float h);
static void rect(dc_t* dc, colorf_t* color, float x, float y, float w, float h);
static void line(dc_t* dc, colorf_t* color, float x0, float y0, float x1, float y1, float width);
static void ring(dc_t* dc, colorf_t* color, float x, float y, float radius, float inner);
static void bblt(dc_t* dc, bitmap_t* bitmap, float x, float y);
static void luma(dc_t* dc, bitmap_t* bitmap, colorf_t* color, float x, float y, float s, float t, float w, float h);
static void poly(dc_t* dc, colorf_t* color, const pointf_t* vertecies, int count); // triangle fan

dc_t dc = {
    init,
    dispose,
    clear,
    fill,
    rect,
    line,
    ring,
    bblt,
    luma,
    poly,
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
    gl_check(glUniformMatrix4fv(shaders.fill_mvp, 1, false, (const GLfloat*)dc->mvp));
    gl_check(glUniform4fv(shaders.fill_rgba, 1, (const GLfloat*)color));
    gl_check(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices));
    gl_check(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
}

static void rect(dc_t* dc, colorf_t* color, float x, float y, float w, float h) {
}

static void line(dc_t* dc, colorf_t* color, float x0, float y0, float x1, float y1, float width) {
}

static void ring(dc_t* dc, colorf_t* color, float x, float y, float radius, float inner) {
}

static void bblt(dc_t* dc, bitmap_t* bitmap, float x, float y) {
}

static void luma(dc_t* dc, bitmap_t* bitmap, colorf_t* color, float x, float y, float s, float t, float w, float h) {
}

static void poly(dc_t* dc, colorf_t* color, const pointf_t* vertecies, int count) {
}

END_C
