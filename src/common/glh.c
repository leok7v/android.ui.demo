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
#include <GLES/gl.h>
#include <GLES2/gl2.h>

BEGIN_C

#ifdef DEBUG
#define gl_error() glGetError()
#else
#define gl_error() 0
#endif

void gl_ortho2D(mat4x4 m, float left, float right, float bottom, float top) {
    // this is basically from
    // http://en.wikipedia.org/wiki/Orthographic_projection_(geometry)
    const float znear = -1;
    const float zfar  = 1;
    const float inv_z = 1 / (zfar - znear);
    const float inv_y = 1 / (top - bottom);
    const float inv_x = 1 / (right - left);
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
    m[0][3] = -(right + left) * inv_x;
    m[1][3] = -(top + bottom) * inv_y;
    m[2][3] = -(zfar + znear) * inv_z;
    m[3][3] = 1;
}

static mat4x4 m4x4_zero;

int gl_init(int w, int h, mat4x4 projection_matrix) {
    int r = 0;
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
    traceln("GL_VERSION=%d.%d %s", major, minor, version);
    memcpy(projection_matrix, m4x4_zero, sizeof(m4x4_zero));
    gl_if_no_error(r, glViewport(0, 0, w, h));
    gl_if_no_error(r, glEnable(GL_BLEND));
    gl_if_no_error(r, glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    gl_if_no_error(r, glDisable(GL_DEPTH_TEST));
    gl_if_no_error(r, glDisable(GL_CULL_FACE));
    return r;
}

int gl_init_texture(int ti) {
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
        #define text_append(...) k = snprintf(s, n, __VA_ARGS__); s += k; n -= k
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

END_C
