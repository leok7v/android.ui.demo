#pragma once
#include "color.h"
#include "linmath.h"

BEGIN_C

#ifdef DEBUG
#define gl_check(call) (call, gl_trace_errors_(__FILE__, __LINE__, __func__, #call, glGetError()))
#define gl_check_call_int(call) gl_trace_errors_return_int_(__FILE__, __LINE__, __func__, #call, (call))
#else
#define gl_check(call) ((call), 0) // glError() is very expensive use only in DEBUG build
#define gl_check_call_int(call) (call)
#endif

#define gl_if_no_error(r, call) do { if (r == 0) { r = gl_check(call); } } while (0)

// all functions 0 in success or last glGetError() if failed

int gl_init(int w, int h, mat4x4 projection_matrix);

int gl_init_texture(int ti);

// gl_color must be reset to gl_color_invalid on [e]glSwapBuffers by caller code
extern const colorf_t gl_color_invalid;
extern colorf_t gl_color;

colorf_t gl_rgb2colorf(int argb);

int gl_set_color(const colorf_t* c); // if c != null set global variable gl_color and calls glColor4f()

int gl_texture_draw_quad(float x0, float y0, float x1, float y1, float s0, float t0, float s1, float t1);
int gl_draw_texture_quads(const float* vertices, const float* texture_coordinates, int n);
int gl_draw_texture(int texture, float x0, float y0, float x1, float y1, float s0, float t0, float s1, float t1); // no blend!
int gl_blend_texture(int texture, float x0, float y0, float x1, float y1, float s0, float t0, float s1, float t1);
int gl_draw_points(const colorf_t* c, const float* xy, int count);
int gl_draw_line(const colorf_t* color, float x1, float y1, float x2, float y2, float width); // line "width" in pixels
int gl_draw_rect(const colorf_t* c, float x, float y, float w, float h); // filled
int gl_draw_rectangle(const colorf_t* c, float x0, float y0, float w, float h, float width); // outline, line "width" in pixels

void gl_ortho2D(float* mat4x4, float left, float right, float bottom, float top);

const char* gl_strerror(int gle);
int gl_trace_errors_(const char* file, int line, const char* func, const char* call, int gle); // returns last glGetError()
int gl_trace_errors_return_int_(const char* file, int line, const char* func, const char* call, int result_of_call); // returns int result of call()

END_C
