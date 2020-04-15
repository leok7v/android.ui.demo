#include "app.h"

BEGIN_C

static int logln(const char* filename, int line, const char* function, const char* format, va_list vl) {
    int r = 0;
    if (app_log != null) {
        const char* file = strrchr(filename, '/');
        if (file == null) { file = strrchr(filename, '\\'); }
        if (file == null) { file = filename; } else { file++; }
        char location[1024];
        snprintf(location, countof(location), "%s(%d) %s ", file, line, function);
        r = app_log(LOG_INFO, "@!@", location, format, vl);
    }
    return r;
}

int _traceln_(const char* filename, int line, const char* function, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    int r = logln(filename, line, function, format, vl);
    va_end(vl);
    return r;
}

int _assertion_(const char* filename, int line, const char* function, const char* a, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    char text[1024];
    vsnprintf(text, countof(text) - 1, format, vl); text[countof(text) - 1] = 0;
    va_end(vl);
    int r = _traceln_(filename, line, function, "assert(%s) failed %s", a, text);
    raise(SIGTRAP);
    return r;
}

int _assert_(const char* filename, int line, const char* function, const char* a) {
    int r = _traceln_(filename, line, function, "assert(%s) failed", a);
    raise(SIGTRAP);
    return r;
}

END_C
