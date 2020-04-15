#pragma once

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
#ifdef __MACH__
# include <mach/mach.h>
# include <mach/task.h> /* defines PAGE_SIZE to 4096 */
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wenum-compare"
#pragma GCC diagnostic warning "-Wunused-function"
#pragma GCC diagnostic error   "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmultichar"
#endif /* defined(__GNUC__) || defined(__clang__) */

#if defined(__GNUC__) || defined(__clang__)
#define attribute_packed __attribute__((packed))
#else
#define attribute_packed
#endif

#define null NULL
typedef uint8_t byte;
#define inline_c inline
#define packed __attribute__((packed))

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define countof(a) ((int)(sizeof(a) / sizeof((a)[0])))

// allocated guarantees that memore is zeroed out:
#define allocate(n) calloc(1, (n))
#define reallocate(p, n) realloc((p), (n))
#define deallocate(p) free((p))

#undef assert // if defined by any includes above
#define assertion(e, ...) do { if (!(e)) { _assertion_(__FILE__, __LINE__, __func__, #e, __VA_ARGS__); } } while (0)
#define assert(e) do { if (!(e)) { _assert_(__FILE__, __LINE__, __func__, #e); } } while (0)

#define static_init(foo) __attribute__((constructor)) static void init_ ## foo ##_constructor(void)

#ifdef __cplusplus
#define BEGIN_C extern "C" {
#define END_C } // extern "C"
#else
#define BEGIN_C
#define END_C
#endif

int _traceln_(const char* filename, int line, const char* function, const char* format, ...);
int _assertion_(const char* filename, int line, const char* function, const char* a, const char* format, ...);
int _assert_(const char* filename, int line, const char* function, const char* a);

#define traceln(...) (_traceln_(__FILE__, __LINE__, __func__, __VA_ARGS__))
