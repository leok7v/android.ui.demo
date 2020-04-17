#include "shaders.h"
#include "glh.h"
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

BEGIN_C

// to move to gles 3.2:
//  #version 320 es
//  out highp vec4 fc
//  void main() {
//      fc = texture(tex, ts);  // instead of gl_FragColor = texture2D()
//  }
//  replace attribute -> in
//  replace varying -> out

// All shaders are parametrized with:
// uniform mat4 mvp model * view * projection matrix

// shaders.fill fill a polygon with color
// in vec2 xy       [0..w], [0..h]
// in vec4 rgba     color components in range [0..1]

const char* shader_fill_vx = "\
    #version 100            \n\
    uniform highp mat4 mvp; \n\
    attribute vec2 xy;      \n\
    void main() {           \n\
        gl_Position = vec4(xy, 0.0, 1.0) * mvp; \n\
    }";

const char* shader_fill_px = "\
    #version 100                     \n\
    uniform highp vec4 rgba;         \n\
    void main() {                    \n\
        gl_FragColor = rgba;         \n\
    }";

// shaders.bblt `bit' block transfer or 4 component texture
// uniform sampler2D tex (texture index e.g. 1 for GL_TEXTURE1)
// in vec4 xyts       [0..w] [0..h] [0..1], [0..1]

const char* shader_bblt_vx = "\
    #version 100            \n\
    uniform highp mat4 mvp; \n\
    attribute vec4 xyts;    \n\
    varying highp vec2 ts;  \n\
    void main() {           \n\
        gl_Position = vec4(xyts.x, xyts.y, 0.0, 1.0) * mvp; \n\
        ts = vec2(xyts[2], xyts[3]); \n\
    }";

const char* shader_bblt_px = "\
    #version 100                     \n\
    uniform sampler2D tex;           \n\
    varying highp vec2 ts;           \n\
    void main() {                    \n\
        gl_FragColor = texture2D(tex, ts); \n\
    }";

// shaders.luma blend 1 component alpha texture with rgba color
// uniform sampler2D tex (texture index e.g. 1 for GL_TEXTURE1)
// uniform rgba
// in vec4 xyts       [0..w] [0..h] [0..1], [0..1]

const char* shader_luma_vx = "\
    #version 100            \n\
    uniform highp mat4 mvp; \n\
    attribute vec4 xyts;    \n\
    varying highp vec2 ts;  \n\
    void main() {           \n\
        gl_Position = vec4(xyts.x, xyts.y, 0.0, 1.0) * mvp; \n\
        ts = vec2(xyts[2], xyts[3]); \n\
    }";

const char* shader_luma_px = "\
    #version 100             \n\
    uniform highp vec4 rgba; \n\
    uniform sampler2D  tex;  \n\
    varying highp vec2 ts;   \n\
    void main() {                                                       \n\
        highp vec4 c = texture2D(tex, ts);                              \n\
        gl_FragColor = vec4(rgba[0], rgba[1], rgba[2], rgba[3] * c[3]); \n\
    }";

// shaders.ring
// in vec4 quad with [x, y, t, s] where ts are 0, 1 interpolated
// in ri2 inner  radius ^ 2 (inclusive) [0..1]
// in ro2 outter radius ^ 2 (inclusive) [0..1]
// in vec4 rgba color components in range [0..1]
// IMPORTANT: radius is measured as ration of [x0, y0, x1, y1] square

const char* shader_ring_vx = "\
    #version 100            \n\
    uniform highp mat4 mvp; \n\
    attribute vec4 xyts;    \n\
    varying highp vec2 ts;  \n\
    void main() {           \n\
        gl_Position = vec4(xyts.x, xyts.y, 0.0, 1.0) * mvp; \n\
        ts = vec2(xyts[2], xyts[3]);                        \n\
    }";

const char* shader_ring_px = "\
    #version 100             \n\
    precision highp float;   \n\
    uniform highp vec4 rgba; \n\
    uniform float ro2;       \n\
    uniform float ri2;       \n\
    varying highp vec2 ts;   \n\
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

static int create_and_link_program(int *program,
                                   const char* vertex, const char* vs,
                                   const char* fragment, const char* fs) {
    *program = 0;
    int r = 0;
    gl_shader_source_t sources[] = {
        {GL_SHADER_VERTEX,   vertex,   vs, (int)strlen(vs)},
        {GL_SHADER_FRAGMENT, fragment, fs, (int)strlen(fs)}
    };
    r = shader_program_create_and_link(program, sources, countof(sources));
    assert(r == 0);
    return r;
}

#define create_program(p, v, f) create_and_link_program(p, #v, v, #f, f)

int shaders_init() {
    int r = 0;
    if (r == 0) { r = create_program(&shaders.fill, shader_fill_vx, shader_fill_px); }
    if (r == 0) { r = create_program(&shaders.bblt, shader_bblt_vx, shader_bblt_px); }
    if (r == 0) { r = create_program(&shaders.luma, shader_luma_vx, shader_luma_px); }
    if (r == 0) { r = create_program(&shaders.ring, shader_ring_vx, shader_ring_px); }
    if (r == 0) { // glsl compiler removes unused uniforms and in/out (attributes/varyings)
        shaders.fill_mvp  = gl_check_int_call(r, glGetUniformLocation(shaders.fill, "mvp"));
        shaders.fill_rgba = gl_check_int_call(r, glGetUniformLocation(shaders.fill, "rgba"));
        assert(shaders.fill_mvp >= 0 && shaders.fill_rgba >= 0);
        shaders.bblt_mvp  = gl_check_int_call(r, glGetUniformLocation(shaders.bblt, "mvp"));
        shaders.bblt_tex  = gl_check_int_call(r, glGetUniformLocation(shaders.bblt, "tex"));
        assert(shaders.bblt_mvp >= 0 && shaders.bblt_tex  >= 0);
        shaders.luma_mvp  = gl_check_int_call(r, glGetUniformLocation(shaders.luma, "mvp"));
        shaders.luma_tex  = gl_check_int_call(r, glGetUniformLocation(shaders.luma, "tex"));
        shaders.luma_rgba = gl_check_int_call(r, glGetUniformLocation(shaders.luma, "rgba"));
        assert(shaders.luma_mvp >= 0 && shaders.luma_rgba >= 0);
        assert(shaders.luma_tex >= 0);
        shaders.ring_mvp  = gl_check_int_call(r, glGetUniformLocation(shaders.ring, "mvp"));
        shaders.ring_rgba = gl_check_int_call(r, glGetUniformLocation(shaders.ring, "rgba"));
        shaders.ring_ro2  = gl_check_int_call(r, glGetUniformLocation(shaders.ring, "ro2"));
        shaders.ring_ri2  = gl_check_int_call(r, glGetUniformLocation(shaders.ring, "ri2"));
        assert(shaders.ring_mvp >= 0 && shaders.ring_rgba >= 0);
        assert(shaders.ring_ro2 >= 0 && shaders.ring_ri2 >= 0);
    }
    assert(r == 0);
    return r;
}

void shaders_dispose() { // glDeleteProgram(0) is legal NOOP
    shader_program_dispose(shaders.fill);
    shader_program_dispose(shaders.bblt);
    shader_program_dispose(shaders.luma);
    shader_program_dispose(shaders.ring);
    memset(&shaders, 0, sizeof(shaders));
}

END_C
