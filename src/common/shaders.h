#pragma once
#include "rt.h"

BEGIN_C

// shaders to fill a polygon with color
// in vec2 xy       [0..1], [0..1]
// in vec4 rgba     color components in range [0..1]
const char* shader_fill_vx;
const char* shader_fill_px;

// uniform sampler2D tex (texture index e.g. 1 for GL_TEXTURE1)
const char* shader_tex_vx;
const char* shader_tex_px;

// uniform sampler2D tex (texture index e.g. 1 for GL_TEXTURE1)
// in vec4 rgba color components in range [0..1]
const char* shader_luma_vx;
const char* shader_luma_px;

// in vec4 quad with [x, y, t, s] where ts are 0, 1 interpolated
// in ri2 inner  radius ^ 2 (inclusive) [0..1]
// in ro2 outter radius ^ 2 (inclusive) [0..1]
// in vec4 rgba color components in range [0..1]
const char* shader_ring_vx;
const char* shader_ring_px;

enum { // gl_shader_source_t.type
    GL_SHADER_VERTEX = 0,
    GL_SHADER_FRAGMENT = 1
};

typedef struct gl_shader_source_s {
    int type;
    const char* name;
    const void* data;
    int bytes; // can be -1 but only for zero character terminated shaders
} gl_shader_source_t;

int shader_program_create_and_link(int* program, gl_shader_source_t sources[], int count);
int shader_program_dispose(int program);

typedef struct shaders_s {
    int fill;
    int tex;
    int luma; // 8 bit GL_ALPHA tex * rgba color
    int ring;
} shaders_t;

extern shaders_t shaders;

int  shaders_init();
void shaders_dispose();

END_C