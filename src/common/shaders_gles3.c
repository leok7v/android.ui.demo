#include "shaders.h"
#include "glh.h"
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

BEGIN_C

// gles 3.2 differences:
//  #version 320 es
//  out highp vec4 fc // instead of: varying out highp vec4 gl_FragColor;
//  void main() {
//      fc = texture(tex, ts);  // instead of gl_FragColor = texture2D()
//  }

const char* shader_fill_vx = "\
    #version 100            \n\
    uniform highp mat4 mvp; \n\
    in vec2 xy;             \n\
    void main() {           \n\
        gl_Position = vec4(xy, 0.0, 1.0) * mvp; \n\
    }";

const char* shader_fill_px = "\
    #version 100                         \n\
    uniform highp vec4 rgba;             \n\
    varying out highp vec4 gl_FragColor; \n\
    void main() {                        \n\
        gl_FragColor = rgba;             \n\
    }";

const char* shader_tex_vx = "\
    #version 100               \n\
    uniform highp mat4 mvp;    \n\
    in  vec4 xyts;             \n\
    varying out highp vec2 ts; \n\
    void main() {              \n\
        gl_Position = vec4(xyts.x, xyts.y, 0.0, 1.0) * mvp; \n\
        ts = vec2(xyts[2], xyts[3]); \n\
    }";

const char* shader_tex_px = "\
    #version 100                           \n\
    uniform sampler2D tex;                 \n\
    in highp vec2 ts;                      \n\
    varying out highp vec4 gl_FragColor;   \n\
    void main() {                          \n\
        gl_FragColor = texture2D(tex, ts); \n\
    }";

const char* shader_luma_vx = "\
    #version 100               \n\
    uniform highp mat4 mvp;    \n\
    in vec4 xyts;              \n\
    varying out highp vec2 ts; \n\
    void main() {              \n\
        gl_Position = vec4(xyts.x, xyts.y, 0.0, 1.0) * mvp; \n\
        ts = vec2(xyts[2], xyts[3]); \n\
    }";

const char* shader_luma_px = "\
    #version 100             \n\
    uniform highp vec4 rgba; \n\
    uniform sampler2D tex;   \n\
    in highp vec2 ts;        \n\
    varying out highp vec4 gl_FragColor;                                \n\
    void main() {                                                       \n\
        highp vec4 c = texture2D(tex, ts);                              \n\
        gl_FragColor = vec4(rgba[0], rgba[1], rgba[2], rgba[3] * c[3]); \n\
    }";

const char* shader_ring_vx = "\
    #version 100            \n\
    uniform highp mat4 mvp; \n\
    in vec4 xyts;           \n\
    varying out highp vec2 ts;                              \n\
    void main() {                                           \n\
        gl_Position = vec4(xyts.x, xyts.y, 0.0, 1.0) * mvp; \n\
        ts = vec2(xyts[2], xyts[3]);                        \n\
    }";

const char* shader_ring_px = "\
    #version 100             \n\
    precision highp float;   \n\
    uniform float ro2;       \n\
    uniform float ri2;       \n\
    uniform highp vec4 rgba; \n\
    in highp vec2 ts;        \n\
    varying out highp vec4 gl_FragColor;    \n\
    void main() {                           \n\
        float x = ts.x * 2.0 - 1.0;         \n\
        float y = ts.y * 2.0 - 1.0;         \n\
        float x2_y2 = x * x + y * y;        \n\
        if (ri2 <= x2_y2 && x2_y2 <= ro2) { \n\
            gl_FragColor = rgba;            \n\
        } else {                            \n\
            discard;                        \n\
        }                                   \n\
    }";

static void trace_glsl_compile_errors(const char* name, int shader, const char* code, int bytes) {
    GLint info_log_length = 0;
    gl_check(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length));
    GLsizei length = 0;
    GLchar message[info_log_length + 1];
    gl_check(glGetShaderInfoLog(shader, info_log_length, &length, message));
    message[countof(message) - 1] = 0;
    traceln("%s:\n%.*s compile error: %s", name, bytes, code, message);
}

static int shader_create_compile_and_attach(int program, gl_shader_source_t* s) {
    int r = 0;
    GLenum types[] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    assert(0 <= s->type && s->type < countof(types));
	GLuint shader = 0 <= s->type && s->type < countof(types) ? gl_check_call_int(glCreateShader(types[s->type])) : 0;
    if (shader == 0) {
        r = ENOMEM;
    }
    if (r == 0) {
        const char* code = (const char*)s->data;
        GLint bytes = s->bytes;
        assert(bytes > 0);
	    gl_if_no_error(r, glShaderSource(shader, 1, &code, &bytes));
	    gl_if_no_error(r, glCompileShader(shader));
        GLint fragment_compiled = false;
        gl_if_no_error(r, glGetShaderiv(shader, GL_COMPILE_STATUS, &fragment_compiled));
        if (!fragment_compiled) {
            trace_glsl_compile_errors(s->name, shader, code, bytes);
            r = EINVAL;
        } else {
            gl_if_no_error(r, glAttachShader(program, shader));
        }
        gl_check(glDeleteShader(shader)); // marks shader to be deleted when detached
        // glDeleteProgram() will automatically detach shaders and they will be deleted
    }
    return r;
}

int shader_program_create_and_link(int* program, gl_shader_source_t sources[], int count) {
    int r = 0;
    GLint p = gl_check_call_int(glCreateProgram());
    *program = p;
    if (p == 0) {
        r = ENOMEM;
    }
    if (r == 0) {
        for (int i = 0; i < count && r == 0; i++) {
            r = shader_create_compile_and_attach(p, &sources[i]);
        }
    }
    gl_if_no_error(r, glLinkProgram(p));
    if (r != 0) { shader_program_dispose(p); *program = 0; }
    return r;
}

int shader_program_dispose(int program) {
    int r = 0;
    gl_if_no_error(r, glDeleteProgram(program)); // shaders will be automatically detached
    return r;
}

shaders_t shaders;

static int create_and_link_program(int *program, const char* vertex, const char* fragment) {
    *program = 0;
    int r = 0;
    gl_shader_source_t sources[] = {
        {GL_SHADER_VERTEX,   "vertex",   vertex,   (int)strlen(vertex)},
        {GL_SHADER_FRAGMENT, "fragment", fragment, (int)strlen(fragment)}
    };
    r = shader_program_create_and_link(program, sources, countof(sources));
    assert(r == 0);
    return r;
}

int shaders_init() {
    int r = 0;
    if (r == 0) { r = create_and_link_program(&shaders.fill, shader_fill_vx, shader_fill_px); }
    if (r == 0) { r = create_and_link_program(&shaders.tex,  shader_tex_vx,  shader_tex_px);  }
    if (r == 0) { r = create_and_link_program(&shaders.luma, shader_luma_vx, shader_luma_px); }
    if (r == 0) { r = create_and_link_program(&shaders.ring, shader_ring_vx, shader_ring_px); }
    return r;
}

void shaders_dispose() { // glDeleteProgram(0) is legal NOOP
    shader_program_dispose(shaders.fill);  shaders.fill = 0;
    shader_program_dispose(shaders.tex);   shaders.tex  = 0;
    shader_program_dispose(shaders.luma);  shaders.luma = 0;
    shader_program_dispose(shaders.ring);  shaders.ring = 0;
}

END_C
