#include "glh.h"
#include <GLES/gl.h>
#include <GLES3/gl3.h>

BEGIN_C

#ifdef DEBUG
#define gl_error() glGetError()
#else
#define gl_error() 0
#endif

const colorf_t gl_color_invalid = { -1, -1, -1, -1 };

colorf_t gl_color = { -1, -1, -1, -1 };

colorf_t gl_rgb2colorf(int argb) {
    int a = (argb >> 24) & 0xFF;
    colorf_t f = { ((argb >> 16) & 0xFF) / 255.0f, ((argb >> 8) & 0xFF) / 255.0f,
                   ((argb >>  0) & 0xFF) / 255.0f, a == 0 ? 1 : a / 255.0f };
    return f;
}

int gl_set_color(const colorf_t* c) {
    int r = 0;
    if (c != null) {
        if (memcmp(&gl_color, c, sizeof(gl_color)) != 0) {
            gl_color = *c;
            if (c != &gl_color_invalid) { gl_if_no_error(r, glColor4f(c->r, c->g, c->b, c->a)); }
//          traceln("color := %.3f %.3f %.3f %.3f", c->r, c->g, c->b, c->a);
        } else {
//          traceln("same color %.3f %.3f %.3f %.3f", c->r, c->g, c->b, c->a);
        }
    }
    return r;
}

void gl_ortho2D(float* mat, float left, float right, float bottom, float top) {
    // this is basically from
    // http://en.wikipedia.org/wiki/Orthographic_projection_(geometry)
    const float znear = -1;
    const float zfar  = 1;
    const float inv_z = 1 / (zfar - znear);
    const float inv_y = 1 / (top - bottom);
    const float inv_x = 1 / (right - left);
    *mat++ = 2 * inv_x; // first column
    *mat++ = 0;
    *mat++ = 0;
    *mat++ = 0;
    *mat++ = 0; // second
    *mat++ = 2 * inv_y;
    *mat++ = 0;
    *mat++ = 0;
    *mat++ = 0; // third
    *mat++ = 0;
    *mat++ = -2 * inv_z;
    *mat++ = 0;
    *mat++ = -(right + left) * inv_x;  // fourth
    *mat++ = -(top + bottom) * inv_y;
    *mat++ = -(zfar + znear) * inv_z;
    *mat   = 1;
}

static mat4x4 m4x4_zero;

int gl_init(int w, int h, mat4x4 projection_matrix) {
    int r = 0;
    const char* gl_version = (const char*)glGetString(GL_VERSION); (void)gl_version;
    traceln("GL_VERSION=%s", gl_version);
    memcpy(projection_matrix, m4x4_zero, sizeof(m4x4_zero));
//  These hints were removed in OpenGL ES 3.x and above
//  gl_if_no_error(r, glHint(GL_POINT_SMOOTH_HINT, GL_NICEST));
//  gl_if_no_error(r, glHint(GL_LINE_SMOOTH_HINT, GL_NICEST));
//  gl_if_no_error(r, glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST));
//  gl_if_no_error(r, glEnable(GL_TEXTURE_2D));
//  gl_if_no_error(r, glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));
    gl_if_no_error(r, glEnable(GL_BLEND));
    gl_if_no_error(r, glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
//  gl_if_no_error(r, glShadeModel(GL_SMOOTH));
    gl_if_no_error(r, glDisable(GL_CULL_FACE));
//  gl_if_no_error(r, glDisable(GL_LIGHTING));
    gl_if_no_error(r, glDisable(GL_DEPTH_TEST));
    gl_if_no_error(r, glViewport(0, 0, w, h));
//  gl_if_no_error(r, glMatrixMode(GL_PROJECTION));
//  gl_if_no_error(r, glLoadIdentity());
//  gl_if_no_error(r, gl_ortho2D(projection_matrix, 0, w, h, 0));/* near -1 far +1 */
//  gl_if_no_error(r, glMultMatrixf(projection_matrix));
//  gl_if_no_error(r, glMatrixMode(GL_MODELVIEW));
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

int gl_texture_draw_quad(float x0, float y0, float x1, float y1, float s0, float t0, float s1, float t1) {
    GLfloat vertices[] = { x0,y0, x1,y0, x1,y1, x1,y1, x0,y1, x0,y0 };
    GLfloat texture_coordinates[]  = { s0,t0, s1,t0, s1,t1, s1,t1, s0,t1, s0,t0 };
    int r = 0;
    gl_if_no_error(r, glEnableClientState(GL_VERTEX_ARRAY));
    gl_if_no_error(r, glEnableClientState(GL_TEXTURE_COORD_ARRAY));
    gl_if_no_error(r, glVertexPointer(2, GL_FLOAT, 0, vertices));
    gl_if_no_error(r, glTexCoordPointer(2, GL_FLOAT, 0, texture_coordinates));
    gl_if_no_error(r, glDrawArrays(GL_TRIANGLES, 0, 6));
    gl_if_no_error(r, glDisableClientState(GL_TEXTURE_COORD_ARRAY));
    gl_if_no_error(r, glDisableClientState(GL_VERTEX_ARRAY));
    return r;
}

int gl_draw_texture_quads(const float* vertices, const float* texture_coordinates, int n) {
    int r = 0;
    gl_if_no_error(r, glEnableClientState(GL_VERTEX_ARRAY));
    gl_if_no_error(r, glEnableClientState(GL_TEXTURE_COORD_ARRAY));
    gl_if_no_error(r, glVertexPointer(2, GL_FLOAT, 0, vertices));
    gl_if_no_error(r, glTexCoordPointer(2, GL_FLOAT, 0, texture_coordinates));
    gl_if_no_error(r, glDrawArrays(GL_TRIANGLES, 0, n * 6));
    gl_if_no_error(r, glDisableClientState(GL_TEXTURE_COORD_ARRAY));
    gl_if_no_error(r, glDisableClientState(GL_VERTEX_ARRAY));
    return r;
}

int gl_draw_texture(int ti, float x0, float y0, float x1, float y1, float s0, float t0, float s1, float t1) {
    int r = 0;
    // GL_DECAL ignores the primary color and just shows the texel
    gl_if_no_error(r, glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL));
    gl_if_no_error(r, glBindTexture(GL_TEXTURE_2D, ti));
    gl_if_no_error(r, gl_texture_draw_quad(x0, y0, x1, y1, s0, t0, s1, t1));
    gl_if_no_error(r, glBindTexture(GL_TEXTURE_2D, 0));
    return r;
}

int gl_blend_texture(int ti, float x0, float y0, float x1, float y1, float s0, float t0, float s1, float t1) {
    int r = 0;
    // GL_MODULE multiplies primary color and texel and shows the result.
    gl_if_no_error(r, glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));
    gl_if_no_error(r, glBindTexture(GL_TEXTURE_2D, ti));
    gl_texture_draw_quad(x0, y0, x1, y1, s0, t0, s1, t1);
    gl_if_no_error(r, glBindTexture(GL_TEXTURE_2D, 0));
    return r;
}

int gl_draw_points(const colorf_t* c, const float* xy, int count) {
    int r = 0;
    gl_if_no_error(r, gl_set_color(c));
    gl_if_no_error(r, glEnableClientState(GL_VERTEX_ARRAY));
    gl_if_no_error(r, glVertexPointer(2, GL_FLOAT, 0, xy));
    gl_if_no_error(r, glDrawArrays(GL_POINTS, 0, count));
    gl_if_no_error(r, glDisableClientState(GL_VERTEX_ARRAY));
    return r;
}

int gl_draw_line(const colorf_t* c, float x0, float y0, float x1, float y1, float width) {
    int r = 0;
    float line_vertex[]= { x0, y0, x1, y1 };
    gl_if_no_error(r, gl_set_color(c));
    gl_if_no_error(r, glEnableClientState(GL_VERTEX_ARRAY));
    gl_if_no_error(r, glLineWidth(width));
    gl_if_no_error(r, glVertexPointer(2, GL_FLOAT, 0, line_vertex));
    gl_if_no_error(r, glDrawArrays(GL_LINES, 0, 2));
    gl_if_no_error(r, glDisableClientState(GL_VERTEX_ARRAY));
    return r;
}

int gl_draw_rect(const colorf_t* c, float x0, float y0, float w, float h) {
    int r = 0;
    float x1 = x0 + w;
    float y1 = y0 + h;
    GLfloat vertices[] = { x0, y0, x1, y0, x1, y1, x1, y1, x0, y1, x0, y0 };
    gl_if_no_error(r, gl_set_color(c));
    gl_if_no_error(r, glEnableClientState(GL_VERTEX_ARRAY));
    gl_if_no_error(r, glVertexPointer(2, GL_FLOAT, 0, vertices));
    gl_if_no_error(r, glDrawArrays(GL_TRIANGLES, 0, 6));
    gl_if_no_error(r, glDisableClientState(GL_VERTEX_ARRAY));
    return r;
}

int gl_draw_rectangle(const colorf_t* c, float x0, float y0, float w, float h, float width) {
    int r = 0;
    gl_if_no_error(r, gl_draw_line(c, x0, y0, x0 + w - width, y0, width));
    gl_if_no_error(r, gl_draw_line(null, x0 + w, y0, x0 + w, y0 + h - width, width));
    gl_if_no_error(r, gl_draw_line(null, x0 + w, y0 + h, x0 + width, y0 + h, width));
    gl_if_no_error(r, gl_draw_line(null, x0, y0 + h, x0, y0 - width, width));
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

int gl_trace_errors_return_int_(const char* file, int line, const char* func, const char* call, int r) {
    int gle = glGetError();
    if (gle != 0) { gl_trace_errors_(file, line, func, call, gle); }
    return r;
}

END_C
