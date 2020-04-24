#pragma once
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
#include "rt.h"

BEGIN_C

#ifdef DEBUG
#define gl_check(call) (call, gl_trace_errors_(__FILE__, __LINE__, __func__, #call, glGetError()))
#define gl_check_call_int(call) gl_trace_errors_return_int_(__FILE__, __LINE__, __func__, null, #call, (call))
#define gl_check_int_call(r, call) gl_trace_errors_return_int_(__FILE__, __LINE__, __func__, &(r), #call, (call))
#else
#define gl_check(call) ((call), 0) // glError() is very expensive use only in DEBUG build
#define gl_check_call_int(call) (call)
#endif

#define gl_if_no_error(r, call) do { if (r == 0) { r = gl_check(call); } } while (0)

// all functions 0 in success or last glGetError() if failed

int gl_allocate_texture(int *ti);
int gl_update_texture(int ti, int w, int h, int bpp, const void* data); // bpp - bytes per pixel
int gl_delete_texture(int ti);

const char* gl_strerror(int gle);
int gl_trace_errors_(const char* file, int line, const char* func, const char* call, int gle); // returns last glGetError()
int gl_trace_errors_return_int_(const char* file, int line, const char* func, int* r, const char* call, int result_of_call); // returns int result of call()

END_C
