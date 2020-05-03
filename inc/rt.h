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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <netdb.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "c.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wenum-compare"
#pragma GCC diagnostic warning "-Wunused-function"
#pragma GCC diagnostic error   "-Wuninitialized"
#endif /* defined(__GNUC__) || defined(__clang__) */

#define null NULL
typedef uint8_t byte;
#define inline_c inline

#if defined(__GNUC__) || defined(__clang__)
#define packed __attribute__((packed))
#else
#define packed // TODO: grep packed on Windows and surround with pragmas
#endif

#undef min
#undef max
#undef countof
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define countof(a) ((int)(sizeof(a) / sizeof((a)[0])))

// allocated guarantees that memore is zeroed out:
#define allocate(n) calloc(1, (n))
#define reallocate(p, n) realloc((p), (n))
#define deallocate(p) free((p))

#undef assert // if defined by any includes above
#define assertion(e, ...) do { if (!(e)) { _assertion_(__FILE__, __LINE__, __func__, #e, ##__VA_ARGS__); } } while (0)
#define assert(e) do { if (!(e)) { _assert_(__FILE__, __LINE__, __func__, #e); } } while (0)

#define static_init(foo) __attribute__((constructor)) static void init_ ## foo ##_constructor(void)

int _traceln_(const char* filename, int line, const char* function, const char* format, ...);
int _assertion_(const char* filename, int line, const char* function, const char* a, const char* format, ...);
int _assert_(const char* filename, int line, const char* function, const char* a);
int strzt(char* text, int count, int call); // make sure truncated string is zero terminated after call()

#define traceln(...) (_traceln_(__FILE__, __LINE__, __func__, __VA_ARGS__))

#define vsnprintf0(text, f, vl) (strzt((text), countof(text), vsnprintf((text), countof(text) - 1, f, vl)))
#define snprintf0(text, f, ...) (strzt((text), countof(text), snprintf((text), countof(text) - 1, f, ##__VA_ARGS__)))
