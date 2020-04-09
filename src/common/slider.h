#pragma once
#include "ui.h"

BEGIN_C

typedef struct slider_s slider_t;

typedef struct slider_s {
    ui_t ui; // if ui.focusable - slider has [+]/[-] buttons, otherwise it is buttonless progress indicator
    ui_expo_t* expo;
    void (*notify)(slider_t* self); // on [+]/[-] click, Ctrl+Click (+/- 10), Shift+Click  (+/- 100), Ctrl+Shift+Click  (+/- 1000)
    int* minimum; // may be changed by caller on the fly, thus pointer
    int* maximum;
    int* current;
    const char*  label;     // can be null
    colorf_t* color_slider; // color for slider itself
    // internal state:
    int timer_id;
    timer_callback_t timer_callback;
} slider_t;

slider_t* slider_create(ui_t* parent, void* that, ui_expo_t* expo,
                        colorf_t* color_slider,
                        const char* label, float x, float y, float w, float h,
                        int* minimum, int* maximum, int* current);

void slider_dispose(slider_t* s);

END_C
