#pragma once
#include "rt.h"

BEGIN_C

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
    int fill_mvp; // mvp "in" location
    int fill_rgba;
    int bblt;
    int bblt_mvp;
    int bblt_tex;
    int luma; // 8 bit GL_ALPHA tex * rgba color
    int luma_mvp;
    int luma_tex;
    int luma_rgba;
    int ring;
    int ring_mvp;
    int ring_rgba;
    int ring_ro2;
    int ring_ri2;
} shaders_t;

extern shaders_t shaders;

int  shaders_init();
void shaders_dispose();

END_C
