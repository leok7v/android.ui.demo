#include "app.h"
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

begin_c

static int logln(const char* filename, int line, const char* function, const char* format, va_list vl) {
    int r = 0;
    if (app->logln != null) {
        const char* file = strrchr(filename, '/');
        if (file == null) { file = strrchr(filename, '\\'); }
        if (file == null) { file = filename; } else { file++; }
        char location[1024];
        snprintf0(location, "%s(%d) \t%s ", file, line, function);
        r = app->logln(LOG_INFO, "@!@", location, format, vl);
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
    vsnprintf0(text, format, vl);
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

int _ensure_zero_terminated_(char* text, int count, int call) {
    // [v]snprintf() and alike do NOT zero terminate result on overflow/truncate
    text[count - 1] = 0; // make sure it is zero terminated
    return call;
}

end_c
