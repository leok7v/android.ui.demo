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
#include "rt.h"
#include <GLES/gl.h>
#include <GLES3/gl3.h>

begin_c

#ifdef DEBUG
#define gl_error() glGetError()
#else
#define gl_error() 0
#endif

static int init_texture(int ti) {
    int r = 0;
    gl_if_no_error(r, glBindTexture(GL_TEXTURE_2D, ti));
    gl_if_no_error(r, glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    gl_if_no_error(r, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)); // !no interpolation please!
    gl_if_no_error(r, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST)); // vs GL_LINEAR
    gl_if_no_error(r, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    gl_if_no_error(r, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    gl_if_no_error(r, glBindTexture(GL_TEXTURE_2D, 0));
    return r;
}

int gl_allocate_texture(int *ti) {
    int r = 0;
    assertion(*ti == 0, "is texture already allocated ti=%d?", *ti);
    if (*ti != 0) {
        r = EINVAL;
    } else {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        // cannot call glGetError() here because it can be a) not in valid gl context, b) has accumulated previous errors
        if (tex == 0) {
            r = ENOMEM;
        } else {
            assertion(tex > 0, "negative ti=0x%08X (should texture be uint32_t?)", *ti);
            r = init_texture(tex);
            if (r == 0) { *ti = tex; } else { gl_delete_texture(tex); }
        }
    }
    return r;
}

int gl_update_texture(int ti, int w, int h, int bpp, const void* data) {
    int r = 0;
    int c = bpp - 1;
    static const int formats[] = { GL_ALPHA, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
    assertion(0 <= c && c < countof(formats), "invalid number of byte per pixel components: %d", bpp);
    if (0 <= c && c < countof(formats)) {
        int format = formats[c];
        gl_if_no_error(r, glBindTexture(GL_TEXTURE_2D, ti));
        gl_if_no_error(r, glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data));
        gl_if_no_error(r, glBindTexture(GL_TEXTURE_2D, 0));
    } else {
        r = EINVAL;
    }
    return r;
}

int gl_delete_texture(int ti) {
    int r = 0;
    assertion(ti > 0, "texture was not allocated ti=%d", ti);
    GLuint tex = ti;
    if (tex != 0) {
        gl_if_no_error(r, glDeleteTextures(1, &tex));
    } else {
        r = EINVAL;
    }
    return r;
}

const char* gl_strerror(int gle) { // glGetError() is in range 0x0500..0x0505 while posix error is 1..1xx
    switch (gle) {
        case GL_NO_ERROR         : return "";
        case GL_INVALID_ENUM     : return "INVALID_ENUM";
        case GL_INVALID_VALUE    : return "INVALID_VALUE";
        case GL_INVALID_OPERATION: return "INVALID_OPERATION";
        case GL_STACK_OVERFLOW   : return "STACK_OVERFLOW";
        case GL_STACK_UNDERFLOW  : return "STACK_UNDERFLOW";
        case GL_OUT_OF_MEMORY    : return "OUT_OF_MEMORY";
        default: return strerror(gle);
    }
}

int gl_trace_errors_(const char* file, int line, const char* func, const char* call, int gle) {
    // https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glGetError.xhtml
    // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetError.xhtml
    if (gle != 0) {
        char text[1024];
        int n = countof(text);
        int k = 0;
        char* s = text;
        const char* f = strrchr(file, '/');
        if (f == null) { f = strrchr(file, '\\'); }
        if (f != null) { f++; } else { f = file; }
        #define text_append(...) k = snprintf0(s, __VA_ARGS__); s += k; n -= k
        text_append("0x%04X=%s", gle, gl_strerror(gle));
        // old and buggy Windows opengl is capable of returning error on glGetError() (e.g. when context is not current)
        for (int i = 0; i < 8; i++) {
            int e = glGetError(); // "If glGetError itself generates an error, it returns 0."
            if (e == GL_NO_ERROR) { break; }
            text_append(" 0x%04X=%s", e, gl_strerror(e));
            gle = e; // last error
        }
        text[countof(text) - 1] = 0;
//      _traceln_(f, line, func, "%s failed %s", call, text);
        _assertion_(f, line, func, call, "failed %s", text);
    }
    return gle;
}

int gl_trace_errors_return_int_(const char* file, int line, const char* func,
                                int* result, const char* call, int r) {
    int gle = glGetError();
    if (gle != 0) {
        if (result != null) { *result = gle; }
        gl_trace_errors_(file, line, func, call, gle);
    }
    return r;
}

static_init(glh) {
    assert(sizeof(GLuint)     == sizeof(uint32_t));
    assert(sizeof(GLint)      == sizeof(int32_t));
    assert(sizeof(GLushort)   == sizeof(uint16_t));
    assert(sizeof(GLshort)    == sizeof(int16_t));
    assert(sizeof(GLfloat)    == sizeof(float));
    assert(sizeof(GLchar)     == sizeof(char));
    assert(sizeof(GLbyte)     == sizeof(char));
    assert(sizeof(GLubyte)    == sizeof(byte));
    assert(sizeof(GLsizei)    == sizeof(int));
    assert(sizeof(GLintptr)   == sizeof(uintptr_t));
    assert(sizeof(GLsizeiptr) == sizeof(GLsizeiptr));
}

end_c
