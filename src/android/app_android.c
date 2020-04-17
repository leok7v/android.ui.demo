#include "app.h"
#include "ui.h"
#include "button.h"
#include "android_jni.h" // interface to Java only code (going via binder is too involving)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/configuration.h>
#include <android/sensor.h>
#include <android/asset_manager.h>
#include <android/window.h>
#include <android/native_activity.h>
#include <android/log.h>

BEGIN_C

typedef struct glue_s glue_t;

static void  app_quit(app_t* app);
static void  app_exit(app_t* app, int code);
static void  app_animate(app_t* app, int animating);
static void  app_invalidate(app_t* app);
static void  focus(app_t* app, ui_t * ui);
static int   timer_add(app_t* app, timer_callback_t* tcb);
static void  timer_remove(app_t* app, timer_callback_t* tcb);
static void* asset_map(app_t* a, const char* name, const void* *data, int* bytes);
static void  asset_unmap(app_t* a, void* asset, const void* data, int bytes);
static void  vibrate(app_t* a, int vibration_effect);
static void  enqueue_command(glue_t* glue, int8_t command);

static app_t app = {
    /* that:    */ null,
    /* glue:    */ null,
    /* root:    */ &root,
    /* focused: */ null,
    /* xdpi: */ 0,
    /* ydpi: */ 0,
    /* keyboard_flags: */ 0,
    /* mouse_flags:    */ 0,
    /* last_mouse_x:   */ 0,
    /* last_mouse_y:   */ 0,
    /* time_in_nanoseconds */ 0ULL,
    /* trace_flags */ 0,
    /* init:    */ null,
    /* shown:   */ null,
    /* idle:    */ null,
    /* hidden:  */ null,
    /* pause:   */ null,
    /* stop:    */ null,
    /* resume:  */ null,
    /* destroy: */ null,
    app_quit,
    app_exit,
    app_invalidate,
    app_animate,
    focus,
    timer_add,
    timer_remove,
    asset_map,
    asset_unmap,
    vibrate
};

enum {
    // Looper data ID of commands coming from the app's main thread, which
    // is returned as an identifier from ALooper_pollOnce().  The data for this
    // identifier is a pointer to an android_poll_source structure.
    // These can be retrieved and processed with android_na_read_command()
    // and android_na_exec_command().
    LOOPER_ID_MAIN  = 1,
    // Looper data ID of events coming from the AInputQueue of the
    // application's window, which is returned as an identifier from
    // ALooper_pollOnce().  The data for this identifier is a pointer to an
    // android_poll_source structure.  These can be read via the input_queue object.
    LOOPER_ID_INPUT = 2,
    LOOPER_ID_ACCEL = 3,
    // Start of user-defined ALooper identifiers.
    LOOPER_ID_USER  = 4,
};

enum {
    COMMAND_REDRAW = 1,
    COMMAND_TIMER  = 2,
    COMMAND_QUIT   = 3
};

typedef struct android_poll_source_s android_poll_source_t;

typedef struct android_poll_source_s {
    int32_t id;   // The identifier of this source.  May be LOOPER_ID_MAIN or LOOPER_ID_INPUT.
    glue_t* glue; // The glue data this ident is associated with.
    // Function to call to perform the standard processing of data from this source.
    void (*process)(glue_t* app, android_poll_source_t* source);
} android_poll_source_t;

typedef struct glue_s {
    ANativeActivity* na;
    app_t* a;
    ASensorManager*  sensor_manager;
    const ASensor* accelerometer_sensor;
    ASensorEventQueue* sensor_event_queue;
    int animating;
    int running; // == 1 between on_resume() and on_pause()
    volatile int destroy_requested;
    uint64_t start_time_in_ns;
    timer_callback_t* timers[32]; // maximum 32 timers allowed to avoid alloc and linked list
    volatile uint64_t timer_next_ns; // time in ns till next COMMAND_TIMER
    pthread_t timer_thread;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    // android specific:
    AConfiguration* config;
    float inches_wide; // best guess for screen physical size
    float inches_high;
    float density; // do not use - Android dpi `bining` dpi
    bool  keyboad_present; // is keyboard connected?
    ALooper* looper; // The ALooper associated with the app's main event dispatch thread.
    AInputQueue* input_queue; // When non-NULL, this is the input queue from which the app will receive user input events.
    ANativeWindow* window; // When non-NULL, this is the window surface that the app can draw in.
    ARect content_rect;    // this is the area where the window's content should be placed to be seen by the user.
    // This is non-zero when the application's activity is destroyed process should exit.
    int exit_requested;
    int exit_code; // status to exit() with
    int read_pipe;
    int write_pipe;
    android_poll_source_t command_poll_source;
    android_poll_source_t input_poll_source;
    android_poll_source_t accel_poll_source;
} glue_t;

#define synchronized_wait(mutex, cond, timespec, code)      \
    pthread_mutex_lock(&(mutex));                           \
    pthread_cond_timedwait(&(cond), &(mutex), &(timespec)); \
    code;                                                   \
    pthread_mutex_unlock(&(mutex))

#define synchronized_signal(mutex, cond, code) \
    pthread_mutex_lock(&(mutex));              \
    code;                                      \
    pthread_cond_signal(&(cond));              \
    pthread_mutex_unlock(&(mutex))

static int log_vprintf(int level, const char* tag, const char* location, const char* format, va_list vl) {
    char fmt[1024];
    const char* f = format;
    if (location != null) {
        int n = (int)strlen(location) + 1;
        if (0 < n && n < countof(fmt) - strlen(format) - 2) {
            memcpy(fmt, location, n);
            fmt[n - 1] = 0x20; // " "
            strncpy(fmt + n, format, countof(fmt) - n);
            f = fmt;
        }
    }
    return __android_log_vprint(level, tag, f, vl);
}

int (*app_log)(int level, const char* tag, const char* location, const char* format, va_list vl) = log_vprintf;

#define case_return(id) case id: return #id;

static const char* id2str(int id) {
    switch (id) {
        case_return(LOOPER_ID_MAIN);
        case_return(LOOPER_ID_INPUT);
        case_return(LOOPER_ID_ACCEL);
        case_return(LOOPER_ID_USER);
        default: assertion(false, "id=%d", id); return "???";
    }
}

static const char* cmd2str(int command) {
    switch (command) {
        case_return(COMMAND_REDRAW);
        case_return(COMMAND_TIMER);
        case_return(COMMAND_QUIT);
        default: assertion(false, "command=%d", command); return "???";
    }
}

static uint64_t time_monotonic_ns() {
    struct timespec tm = {};
    clock_gettime(CLOCK_MONOTONIC, &tm);
    return NS_IN_SEC * (int64_t)tm.tv_sec + tm.tv_nsec;
}

static void* asset_map(app_t* a, const char* name, const void* *data, int *bytes) {
    glue_t* glue = (glue_t*)a->glue;
    AAsset* asset = AAssetManager_open(glue->na->assetManager, name, AASSET_MODE_BUFFER);
    assertion(asset != null, "asset not found \"%s\"", name);
    *data = null;
    *bytes = 0;
    if (asset != null) {
        *bytes = AAsset_getLength(asset);
        *data = AAsset_getBuffer(asset);
        if (data == null) {
            AAsset_close(asset);
            asset = null;
        }
    }
    return asset;
}

static void asset_unmap(app_t* a, void* asset, const void* data, int bytes) {
    AAsset_close(asset);
}

static int init_display(glue_t* glue) {
    // Unfortunately glDebugMessageCallback is only available in OpenGL ES 3.2 :(
    // https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glDebugMessageCallback.xhtml
    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_NONE
    };
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, 0, 0);
    EGLint configs_count = 0;
    EGLConfig config = 0;
    eglChooseConfig(display, attribs, &config, 1, &configs_count);
    EGLint format = 0;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(glue->window, 0, 0, format);
    EGLSurface surface = eglCreateWindowSurface(display, config, glue->window, null);
    const EGLint context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext context = eglCreateContext(display, config, null, context_attributes);
    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        traceln("eglMakeCurrent() failed");
        return -1;
    }
    EGLint w = 0;
    EGLint h = 0;
    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);
    glue->display = display;
    glue->context = context;
    glue->surface = surface;
    glue->a->root->w = w;
    glue->a->root->h = h;
    glue->a->xdpi = w / glue->inches_wide;
    glue->a->ydpi = h / glue->inches_high;
    gl_init(glue->a->root->w, glue->a->root->h, glue->a->projection);
    mat4x4_identity(glue->a->view);
    return 0;
}

static void focus(app_t* app, ui_t* ui) {
    // ui can be null, thus cannot used ui->a
    if (ui == app->focused) {
//      traceln("already focused %p", ui);
    } else if (ui == null || ui->focusable) {
        if (app->focused != null && app->focused->focus != null) {
            app->focused->focus(app->focused, false);
        }
        app->focused = ui;
        if (ui != null && ui->focus != null) {
            ui->focus(ui, true);
        }
    }
}

static bool dispatch_keyboard_shortcuts(ui_t* p, int flags, int keycode) {
    if (p->kind == UI_KIND_BUTTON && !p->hidden) {
        button_t* b = (button_t*)p;
        int kc = isalpha(keycode) ? tolower(keycode) : keycode;
        int k = isalpha(b->key) ? tolower(b->key) : b->key;
        if (!p->hidden && b->key_flags == flags && k == kc) {
            // TODO: (Leo) if 3 (or more) states checkboxes are required this is the place to do it.
            //       b->flip = (b->flip + 1) % b->flip_wrap_around;
            if (b->flip != null) { *b->flip = !*b->flip; }
            b->click(b);
            return true; // stop search
        }
    }
    ui_t* c = p->children;
    while (c != null) {
        if (!c->hidden) {
            if (dispatch_keyboard_shortcuts(c, flags, keycode)) { return true; }
        }
        c = c->next;
    }
    return false;
}

static void draw_frame(glue_t* glue) {
    if (glue->display != null) {
        if (!glue->a->root->hidden && glue->a->root->draw != null) {
            glue->a->root->draw(glue->a->root);
            // eglSwapBuffers performs an implicit flush operation on the context (glFlush for an OpenGL ES)
            bool swapped = eglSwapBuffers(glue->display, glue->surface);
            assertion(swapped, "eglSwapBuffers() failed"); (void)swapped;
//          traceln("eglSwapBuffers()=%d", swapped);
        }
    }
    if (glue->animating && glue->running) { glue->a->invalidate(glue->a); }
}

static void term_display(glue_t* glue) {
    if (glue->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(glue->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (glue->context != EGL_NO_CONTEXT) {
            eglDestroyContext(glue->display, glue->context);
        }
        if (glue->surface != EGL_NO_SURFACE) {
            eglDestroySurface(glue->display, glue->surface);
        }
        eglTerminate(glue->display);
    }
    assert(glue->running == 0);
    glue->display = EGL_NO_DISPLAY;
    glue->context = EGL_NO_CONTEXT;
    glue->surface = EGL_NO_SURFACE;
}

void app_trace_mouse(int mouse_flags, int mouse_action, int index, float x, float y) {
    char text[128] = {};
    if (mouse_action & MOUSE_LBUTTON_DOWN) {  strncat(text, "LBUTTON_DOWN|", countof(text)); }
    if (mouse_action & MOUSE_LBUTTON_UP)   {  strncat(text, "LBUTTON_UP|", countof(text)); }
    if (mouse_action & MOUSE_MOVE)         {  strncat(text, "MOVE|", countof(text)); }
    if (mouse_flags  & MOUSE_LBUTTON_FLAG) {  strncat(text, "MOUSE_LBUTTON_FLAG|", countof(text)); }
    traceln("mouse_flags=0x%08X %.*s index=%d x,y=%.1f,%.1f", mouse_flags, strlen(text) - 1, text, index, x, y);
}

void app_trace_key(int flags, int ch) {
    char text[128] = {};
    if (flags & KEYBOARD_KEY_PRESSED)  {  strncat(text, "PRESSED|", countof(text)); }
    if (flags & KEYBOARD_KEY_RELEASED) {  strncat(text, "RELEASED|", countof(text)); }
    if (flags & KEYBOARD_KEY_REPEAT)   {  strncat(text, "REPEAT|", countof(text)); }
    if (flags & KEYBOARD_SHIFT)    { strncat(text, "SHIFT|", countof(text)); }
    if (flags & KEYBOARD_ALT)      { strncat(text, "ALT|", countof(text)); }
    if (flags & KEYBOARD_CTRL)     { strncat(text, "CTRL|", countof(text)); }
    if (flags & KEYBOARD_SYM)      { strncat(text, "SYM|", countof(text)); }
    if (flags & KEYBOARD_FN)       { strncat(text, "FN|", countof(text)); }
    if (flags & KEYBOARD_NUMPAD)   { strncat(text, "NUMPAD|", countof(text)); }
    if (flags & KEYBOARD_NUMLOCK)  { strncat(text, "NUMLOCK|", countof(text)); }
    if (flags & KEYBOARD_CAPSLOCK) { strncat(text, "CAPSLOCK|", countof(text)); }
    const char* mnemonic = null;
    switch (ch) {
        case KEY_CODE_ENTER    : mnemonic = "ENTER"; break;
        case KEY_CODE_LEFT     : mnemonic = "LEFT"; break;
        case KEY_CODE_RIGHT    : mnemonic = "RIGHT"; break;
        case KEY_CODE_UP       : mnemonic = "UP"; break;
        case KEY_CODE_DOWN     : mnemonic = "DOWN"; break;
        case KEY_CODE_BACKSPACE: mnemonic = "BACKSPACE"; break;
        case KEY_CODE_PAGE_UP  : mnemonic = "PAGE_UP"; break;
        case KEY_CODE_PAGE_DOWN: mnemonic = "PAGE_DOWN"; break;
        case KEY_CODE_HOME     : mnemonic = "HOME"; break;
        case KEY_CODE_END      : mnemonic = "END"; break;
        case KEY_CODE_BACK     : mnemonic = "BACK"; break;
        case KEY_CODE_ESC      : mnemonic = "ESC"; break;
        case KEY_CODE_DEL      : mnemonic = "DEL"; break;
        case KEY_CODE_INS      : mnemonic = "INS"; break;
        case KEY_CODE_CENTER   : mnemonic = "CENTER"; break;
        case KEY_CODE_ALT      : mnemonic = "ALT"; break;
        case KEY_CODE_CTRL     : mnemonic = "CTRL"; break;
        case KEY_CODE_SHIFT    : mnemonic = "SHIFT"; break;
        case KEY_CODE_F1       : mnemonic = "F1"; break;
        case KEY_CODE_F2       : mnemonic = "F2"; break;
        case KEY_CODE_F3       : mnemonic = "F3"; break;
        case KEY_CODE_F4       : mnemonic = "F4"; break;
        case KEY_CODE_F5       : mnemonic = "F5"; break;
        case KEY_CODE_F6       : mnemonic = "F6"; break;
        case KEY_CODE_F7       : mnemonic = "F7"; break;
        case KEY_CODE_F8       : mnemonic = "F8"; break;
        case KEY_CODE_F9       : mnemonic = "F9"; break;
        case KEY_CODE_F10      : mnemonic = "F10"; break;
        case KEY_CODE_F11      : mnemonic = "F11"; break;
        case KEY_CODE_F12      : mnemonic = "F12"; break;
        default: break;
    }
    if (mnemonic == null) {
        traceln("flags=0x%08X %.*s ch=%d 0x%08X %c", flags, strlen(text) - 1, text, ch, ch, ch);
    } else {
        traceln("flags=0x%08X %.*s ch=%d 0x%08X %s", flags, strlen(text) - 1, text, ch, ch, mnemonic);
    }
}

static bool set_focus(ui_t* ui, int x, int y) {
    assert(ui != null);
    ui_t* child = ui->children;
    bool focus_was_set = false;
    while (child != null && !focus_was_set) {
        assert(child != ui);
        focus_was_set = set_focus(child, x, y);
        child = child->next;
    }
//  traceln("focus_was_set=%d on children of %p", focus_was_set, ui);
    if (!focus_was_set && ui->focusable) {
        const pointf_t pt = ui->screen_xy(ui);
//      traceln("%p %.1f,%.1f %.1fx%.1f focusable=%d mouse %d,%d  pt %.1f,%.1f", ui, ui->x, ui->y, ui->w, ui->h, ui->focusable, x, y, pt.x, pt.y);
        focus_was_set = pt.x <= x && x < pt.x + ui->w && pt.y <= y && y < pt.y + ui->h;
        if (focus_was_set) { ui->a->focus(ui->a, ui); }
    }
//  traceln("focus_was_set=%d on %p", focus_was_set, ui);
    return focus_was_set;
}

static void dispatch_keycode(glue_t* glue, int flags, int keycode) {
    glue->a->keyboard_flags = flags;
    if (glue->a->focused != null && glue->a->focused->keyboard != null) {
        glue->a->focused->keyboard(glue->a->focused, flags, keycode);
    }
    if (flags & KEYBOARD_KEY_PRESSED) {
        int f = flags & ~(KEYBOARD_KEY_PRESSED|KEYBOARD_SHIFT|KEYBOARD_NUMLOCK|KEYBOARD_CAPSLOCK);
        dispatch_keyboard_shortcuts(glue->a->root, f, keycode);
    }
}

static int32_t handle_input(glue_t* glue, AInputEvent* event) {
/*
    AMOTION_EVENT_TOOL_TYPE_FINGER = 1,
    AMOTION_EVENT_TOOL_TYPE_STYLUS = 2,
    AMOTION_EVENT_TOOL_TYPE_ERASER = 4,
*/
    if (AInputEvent_getSource(event) == AINPUT_SOURCE_MOUSE) {
//      traceln("AInputEvent_getSource AINPUT_SOURCE_MOUSE");
    }
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int x = AMotionEvent_getX(event, 0);
        int y = AMotionEvent_getY(event, 0);
        int32_t action = AMotionEvent_getAction(event);
        int32_t index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> 8; // fragile. >> 8 assumes AMOTION_EVENT_ACTION_POINTER_INDEX_MASK = 0xFF00
        action = action & AMOTION_EVENT_ACTION_MASK;
        if (index == 0) {
            int mouse_action = 0;
            int mouse_flags = glue->a->mouse_flags;
            switch (action) {
                case AMOTION_EVENT_ACTION_DOWN        : mouse_action = MOUSE_LBUTTON_DOWN; mouse_flags |=  MOUSE_LBUTTON_FLAG; break;
                case AMOTION_EVENT_ACTION_UP          : mouse_action = MOUSE_LBUTTON_UP;   mouse_flags &= ~MOUSE_LBUTTON_FLAG; break;
                case AMOTION_EVENT_ACTION_HOVER_MOVE  : mouse_action = MOUSE_MOVE; break;
                case AMOTION_EVENT_ACTION_POINTER_DOWN: break; // touch event, "index" is "finger index"
                case AMOTION_EVENT_ACTION_POINTER_UP  : break;
                case AMOTION_EVENT_ACTION_SCROLL      : traceln("TODO: AMOTION_EVENT_ACTION_SCROLL"); break; // mouse wheel generates it
                default: break;
            }
            glue->a->mouse_flags = mouse_flags;
            glue->a->last_mouse_x = x;
            glue->a->last_mouse_y = y;
            if (glue->a->trace_flags & APP_TRACE_MOUSE) { app_trace_mouse(glue->a->mouse_flags, mouse_action, index, x, y); }
            if (mouse_action & MOUSE_LBUTTON_DOWN) {
                if (!set_focus(glue->a->root, x, y)) {
                    glue->a->focus(glue->a, null); // kill focus if no focusable components were found
                }
            }
            if (glue->a->root->mouse != null) { glue->a->root->mouse(glue->a->root, mouse_action, x, y); }
            if (glue->a->root->screen_mouse != null) { glue->a->root->screen_mouse(glue->a->root, mouse_action, x, y); }
        } else {
            traceln("TODO: touch event [%d] x,y=%d,%d", index, x, y);
        }
        return 1;
    }
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
        int32_t key_val = AKeyEvent_getKeyCode(event);
//		int32_t key_flags = AKeyEvent_getFlags(event); // AKEY_EVENT_FLAG_WOKE_HERE, AKEY_EVENT_FLAG_LONG_PRESS ...
        int32_t action = AKeyEvent_getAction(event);
        int32_t times = 1;
        switch (action) {
            case AKEY_EVENT_ACTION_UP      : break;
            case AKEY_EVENT_ACTION_DOWN    : break;
            case AKEY_EVENT_ACTION_MULTIPLE: times = AKeyEvent_getRepeatCount(event); break;
            default: traceln("unknown action: %d", action);
        }
        (void)times; // unused
//      traceln("Received key event: %d\n", key_val);
        int meta_state = AKeyEvent_getMetaState(event);
        int flags = KEYBOARD_KEY_PRESSED; // default because AKEY_EVENT_ACTION_DOWN is not happening...
        int keycode = 0;
        int numpad = 1;
        int numlock = (meta_state & AMETA_NUM_LOCK_ON) != 0;
        int capslock = (meta_state & AMETA_CAPS_LOCK_ON) != 0;
        switch (key_val) {
            case AKEYCODE_SHIFT_LEFT     : keycode = KEY_CODE_SHIFT; break; // seems to be bug in Android
            case AKEYCODE_CTRL_LEFT      : keycode = KEY_CODE_CTRL; break;  // seems to be bug in Android
            case AKEYCODE_ALT_LEFT       : keycode = KEY_CODE_ALT; break;   // seems to be bug in Android
            case AKEYCODE_SOFT_LEFT:  case AKEYCODE_META_LEFT:
            case AKEYCODE_DPAD_LEFT      : keycode = KEY_CODE_LEFT; traceln("keyval=%d", key_val); break;
            case AKEYCODE_SOFT_RIGHT: case AKEYCODE_SHIFT_RIGHT:
            case AKEYCODE_ALT_RIGHT:  case AKEYCODE_META_RIGHT: case AKEYCODE_CTRL_RIGHT:
            case AKEYCODE_DPAD_RIGHT     : keycode = KEY_CODE_RIGHT;     break;
            case AKEYCODE_DPAD_UP        : keycode = KEY_CODE_UP;        break;
            case AKEYCODE_DPAD_DOWN      : keycode = KEY_CODE_DOWN;      break;
            case AKEYCODE_PAGE_UP        : keycode = KEY_CODE_PAGE_UP;   break;
            case AKEYCODE_PAGE_DOWN      : keycode = KEY_CODE_PAGE_DOWN; break;
            case AKEYCODE_MOVE_HOME      : keycode = KEY_CODE_HOME;      break;
            case AKEYCODE_MOVE_END       : keycode = KEY_CODE_END;       break;
            case AKEYCODE_ENTER          : keycode = KEY_CODE_ENTER;     break;
            case AKEYCODE_BACK           : keycode = KEY_CODE_BACK;      break;
            case AKEYCODE_ESCAPE         : keycode = KEY_CODE_ESC;       break;
            case AKEYCODE_FORWARD_DEL    : keycode = KEY_CODE_DEL;       break;
            case AKEYCODE_DEL            : keycode = KEY_CODE_BACKSPACE; break;
            case AKEYCODE_NUMPAD_0       : keycode = numlock ? '0' : KEY_CODE_INS;       break;
            case AKEYCODE_NUMPAD_1       : keycode = numlock ? '1' : KEY_CODE_END;       break;
            case AKEYCODE_NUMPAD_2       : keycode = numlock ? '2' : KEY_CODE_DOWN;      break;
            case AKEYCODE_NUMPAD_3       : keycode = numlock ? '3' : KEY_CODE_PAGE_DOWN; break;
            case AKEYCODE_NUMPAD_4       : keycode = numlock ? '4' : KEY_CODE_LEFT;      break;
            case AKEYCODE_NUMPAD_5       : keycode = numlock ? '5' : KEY_CODE_CENTER;    break;
            case AKEYCODE_NUMPAD_6       : keycode = numlock ? '6' : KEY_CODE_RIGHT;     break;
            case AKEYCODE_NUMPAD_7       : keycode = numlock ? '7' : KEY_CODE_HOME;      break;
            case AKEYCODE_NUMPAD_8       : keycode = numlock ? '8' : KEY_CODE_UP;        break;
            case AKEYCODE_NUMPAD_9       : keycode = numlock ? '9' : KEY_CODE_PAGE_UP;   break;
            case AKEYCODE_NUMPAD_DOT     : keycode = numlock ? '.' : KEY_CODE_DEL;       break;
            case AKEYCODE_NUMPAD_ENTER   : keycode = KEY_CODE_ENTER; break;
            case AKEYCODE_NUMPAD_DIVIDE  : keycode = '/'; break;
            case AKEYCODE_NUMPAD_MULTIPLY: keycode = '*'; break;
            case AKEYCODE_NUMPAD_SUBTRACT: keycode = '-'; break;
            case AKEYCODE_NUMPAD_ADD     : keycode = '+'; break;
            case AKEYCODE_NUMPAD_COMMA   : keycode = ','; break;
            case AKEYCODE_NUMPAD_EQUALS  : keycode = '='; break;
            default: numpad = 0; break;
        }
        if (numpad) { flags |= KEYBOARD_NUMPAD; }
        if (meta_state & AMETA_SYM_ON)       { flags |= KEYBOARD_SYM; }
        if (meta_state & AMETA_SHIFT_ON)     { flags |= KEYBOARD_SHIFT; }
        if (meta_state & AMETA_ALT_ON)       { flags |= KEYBOARD_ALT; }
        if (meta_state & AMETA_CTRL_ON)      { flags |= KEYBOARD_CTRL; }
        if (meta_state & AMETA_FUNCTION_ON)  { flags |= KEYBOARD_FN; }
        if (meta_state & AMETA_NUM_LOCK_ON)  { flags |= KEYBOARD_NUMLOCK; }
        if (meta_state & AMETA_CAPS_LOCK_ON) { flags |= KEYBOARD_CAPSLOCK; }
//	    traceln("mate_state 0x%08X flags=0x%08X\n", meta_state, flags);
        if (keycode == 0) {
            if (AKEYCODE_A <= key_val && key_val <= AKEYCODE_Z) {
                int caps = ((flags & KEYBOARD_SHIFT) != 0) ^ capslock;
                keycode = (key_val - AKEYCODE_A) + (caps ? 'A' : 'a');
            } else if (AKEYCODE_0 <= key_val && key_val <= AKEYCODE_9) {
                keycode = (key_val - AKEYCODE_0) + '0';
            } else if (AKEYCODE_F1 <= key_val && key_val <= AKEYCODE_F12) {
                keycode = (key_val - AKEYCODE_F1) * 0x01000000 + KEY_CODE_F1;
            } else {
                switch (key_val) {
                    case AKEYCODE_SPACE:                keycode = 0x20; break;
                    case AKEYCODE_MINUS:                keycode = '-';  break;
                    case AKEYCODE_PLUS :                keycode = '+';  break;
                    case AKEYCODE_EQUALS:               keycode = '=';  break;
                    case AKEYCODE_COMMA:                keycode = ',';  break;
                    case AKEYCODE_PERIOD:               keycode = '.';  break;
                    case AKEYCODE_SEMICOLON:            keycode = ';';  break;
                    case AKEYCODE_APOSTROPHE:           keycode = '\''; break;
                    case AKEYCODE_GRAVE:                keycode = '`';  break;
                    case AKEYCODE_TAB:                  keycode = '\t'; break;
                    case AKEYCODE_LEFT_BRACKET:         keycode = '[';  break;
                    case AKEYCODE_RIGHT_BRACKET:        keycode = ']';  break;
                    case AKEYCODE_BACKSLASH:            keycode = '\\'; break;
                    case AKEYCODE_SLASH:                keycode = '/';  break;
                    case AKEYCODE_AT:                   keycode = '@';  break;
                    case AKEYCODE_POUND:                keycode = '#';  break;
                    case AKEYCODE_NUMPAD_LEFT_PAREN:    keycode = '(';  break;
                    case AKEYCODE_NUMPAD_RIGHT_PAREN:   keycode = ')';  break;
                    default: break;
                }
            }
        }
        if (flags & KEYBOARD_SHIFT) {
            if ((flags & KEYBOARD_NUMPAD) == 0) {
                switch (keycode) { // US keyboard specific
                    case '1': keycode = '!'; break;
                    case '3': keycode = '#'; break;
                    case '4': keycode = '$'; break;
                    case '5': keycode = '%'; break;
                    case '6': keycode = '^'; break;
                    case '7': keycode = '&'; break;
                    case '8': keycode = '*'; break;
                    case '9': keycode = '('; break;
                    case '0': keycode = ')'; break;
                    case ',': keycode = '<'; break;
                    case '.': keycode = '>'; break;
                    case '/': keycode = '?'; break;
                    case '=': keycode = '+'; break;
                    case '-': keycode = '_'; break;
                    default: break;
                }
            }
            switch (keycode) { // US keyboard specific
                case '`': keycode = '~'; break;
                case '[': keycode = '{'; break;
                case ']': keycode = '}'; break;
                case ';': keycode = ':'; break;
                case '\\': keycode = '|'; break;
                case '\'': keycode = '"'; break;
                default: break;
            }
        }
//      traceln("keycode=%d 0x%08X keyval=%d 0x%08X %c", keycode, keycode, key_val, key_val, key_val);
        if (keycode != 0) {
            switch (action) {
                case AKEY_EVENT_ACTION_DOWN: flags |= KEYBOARD_KEY_PRESSED;  flags &= ~KEYBOARD_KEY_RELEASED; break;
                case AKEY_EVENT_ACTION_UP:   flags |= KEYBOARD_KEY_RELEASED; flags &= ~KEYBOARD_KEY_PRESSED; break;
                default: break;
            }
            if (action == AKEY_EVENT_ACTION_MULTIPLE) {
                int rc = AKeyEvent_getRepeatCount(event);
                traceln("AKEY_EVENT_ACTION_MULTIPLE repeat=%d", rc);
                for (int i = 0; i < rc; i++) { dispatch_keycode(glue, flags | KEYBOARD_KEY_REPEAT, keycode); }
            } else {
                dispatch_keycode(glue, flags, keycode);
            }
        }
        return 1;
    }
    return 0;
}

static void on_start(ANativeActivity* na) {
}

static void on_stop(ANativeActivity* na) {
}

static void on_pause(ANativeActivity* na) {
    glue_t* glue = (glue_t*)na->instance;
    assert(glue->running == 1);
    glue->running = 0;
    if (glue->accelerometer_sensor != null) {
        ASensorEventQueue_disableSensor(glue->sensor_event_queue, glue->accelerometer_sensor);
    }
    if (glue->a->pause != null) { glue->a->pause(glue->a); }
}

static void on_resume(ANativeActivity* na) {
    glue_t* glue = (glue_t*)na->instance;
    assert(glue->running == 0);
    glue->running = 1;
    if (glue->accelerometer_sensor != null) {
        ASensorEventQueue_enableSensor(glue->sensor_event_queue, glue->accelerometer_sensor);
        int us = ASensor_getMinDelay(glue->accelerometer_sensor); // microseconds
//      traceln("accel microseconds=%d", us); // 10,000us = 100Hz
        ASensorEventQueue_setEventRate(glue->sensor_event_queue, glue->accelerometer_sensor, us);
    }
    glue->a->resume(glue->a); // it is up to application code to resume animation if necessary
    // Leo - added draw frame on ANDROID_NA_COMMAND_GAINED_FOCUS and ANDROID_NA_COMMAND_RESUME
    draw_frame(glue);
}

static void* on_save_instance_state(ANativeActivity* na, size_t* bytes) {
    static const char* state = "This is saved state which will come back on_create";
    *bytes = sizeof(state); // including zero terminating char
    return (void*)state;
}

static void on_native_window_created(ANativeActivity* na, ANativeWindow* window) {
    glue_t* glue = (glue_t*)na->instance;
    // The window is being shown, get it ready.
    assert(glue->window == null && window != null);
    glue->window = window;
    init_display(glue);
    if (glue->a->shown != null) { glue->a->shown(glue->a); }
    glue->a->invalidate(glue->a);
}

static void on_native_window_destroyed(ANativeActivity* na, ANativeWindow* window) {
    glue_t* glue = (glue_t*)na->instance;
    assert(glue->window == window);
    term_display(glue);
    if (glue->a->hidden != null) { glue->a->hidden(glue->a); }
    glue->window = null;
}

static void on_window_focus_changed(ANativeActivity* na, int gained) {
}

static void on_configuration_changed(ANativeActivity* na) {
}

static void on_low_memory(ANativeActivity* na) {
}

static void on_native_window_resized(ANativeActivity* na, ANativeWindow* window) {
    glue_t* glue = (glue_t*)na->instance;
    assert(glue->window == window); // otherwise need to go thru window destroyed/created hoops
}

static void on_native_window_redraw_needed(ANativeActivity* na, ANativeWindow* window) {
    glue_t* glue = (glue_t*)na->instance;
    assert(glue->window == window); // otherwise need to go thru window destroyed/created hoops
}

static int looper_callback(int fd, int events, void* data) {
    android_poll_source_t* ps = (android_poll_source_t*)data;
//  traceln("events=%d ps->id=%s", events, id2str(ps->id));
    uint64_t now = time_monotonic_ns();
    ps->glue->a->time_in_nanoseconds = now - ps->glue->start_time_in_ns;
    ps->process(ps->glue, ps); (void)id2str;
    return 1; // continue to receive callbacks
}

static void on_input_queue_created(ANativeActivity* na, AInputQueue* queue) {
    glue_t* glue = (glue_t*)na->instance;
    assert(glue->input_queue == null);
    if (glue->input_queue != null) { AInputQueue_detachLooper(glue->input_queue); }
    glue->input_queue = queue;
    if (glue->input_queue != null) {
        AInputQueue_attachLooper(glue->input_queue, glue->looper, LOOPER_ID_INPUT, looper_callback, &glue->input_poll_source);
    }
}

static void on_input_queued_destroyed(ANativeActivity* na, AInputQueue* queue) {
    glue_t* glue = (glue_t*)na->instance;
    assert(glue->input_queue == queue);
    if (glue->input_queue != null) { AInputQueue_detachLooper(glue->input_queue); }
    glue->input_queue = null;
}

static void on_content_rect_changed(ANativeActivity* na, const ARect* rc) {
    glue_t* glue = (glue_t*)na->instance;
    glue->content_rect = *rc;
    app_invalidate(glue->a);
}

static void app_animate(app_t* app, int animating) {
    glue_t* glue = (glue_t*)app->glue;
    glue->animating = animating;
    if (animating) { app_invalidate(glue->a); }
}

static void app_invalidate(app_t* app) {
    glue_t* glue = (glue_t*)app->glue;
    enqueue_command(glue, COMMAND_REDRAW);
}

static void app_quit(app_t* app) {
    glue_t* glue = (glue_t*)app->glue;
    ANativeActivity_finish(glue->na);
}

static void app_exit(app_t* app, int code) {
    glue_t* glue = (glue_t*)app->glue;
    glue->exit_code = code;
    glue->exit_requested = 1;
    enqueue_command(glue, COMMAND_QUIT);
}

static const uint64_t FOREVER = 86400UL * NS_IN_SEC; // 24 hours

static struct timespec ns_to_timespec(uint64_t ns) {
    struct timespec ts = { (int)(ns / NS_IN_SEC), (long)(ns % NS_IN_SEC)};
    return ts;
}

static void* timer_thread(void* p) {
    glue_t* glue = (glue_t*)p;
    uint64_t wait_ns = FOREVER; // 'forever' or until next wakeup
    struct timespec ts = ns_to_timespec(time_monotonic_ns() + wait_ns);
    while (!glue->destroy_requested) {
        synchronized_wait(glue->mutex, glue->cond, ts,
            wait_ns = glue->timer_next_ns;
            ts = ns_to_timespec(time_monotonic_ns() + wait_ns);
//          traceln("timed_wait=%.3f", glue->timer_next_ns / (double)NS_IN_MS);
        );
        if (!glue->destroy_requested) {
            enqueue_command(glue, COMMAND_TIMER);
//          traceln("COMMAND_TIMER");
        }
    }
    return null;
}

static void on_timer(glue_t* glue) {
    int64_t earliest = -1;
    for (int i = 1; i < countof(glue->timers); i++) {
        timer_callback_t* tc = glue->timers[i];
        if (tc != null) {
            const uint64_t now = time_monotonic_ns();
            if (tc->last_fired == 0) {
                // first time event is seen not actually calling timer callback here
                tc->last_fired = now;
            } else {
                assert(tc->ns > 0);
                uint64_t next = tc->last_fired + tc->ns;
                if (now >= next) {
                    tc->callback(tc);
                    // timer might have been removed and deallocated
                    // inside callback call (not a good practice)
                    if (tc == glue->timers[i]) { tc->last_fired = now; }
                }
            }
            if (earliest < 0 || tc->ns < earliest) {
                earliest = tc->ns;
//              traceln("id=%d earliest := %.6fms", tc->id, earliest / (double)NS_IN_MS);
            }
        }
    }
    if (earliest < 0) { earliest = FOREVER; } // wait `forever' or until next wakeup
    if (earliest != glue->timer_next_ns) {
//      traceln("WAKEUP: earliest = %.6fms", earliest / (double)NS_IN_MS);
        synchronized_signal(glue->mutex, glue->cond, glue->timer_next_ns = earliest);
    }
}

static int timer_add(app_t* app, timer_callback_t* tcb) {
    glue_t* glue = (glue_t*)app->glue;
    assertion(tcb->last_fired == 0, "last_fired=%lld != 0", tcb->last_fired);
    assertion(tcb->ns > 0, "ns=%d must be > 0", tcb->ns);
    if (tcb->ns == 0) { return -1; }
    tcb->last_fired = 0;
    int id = 0;
    // IMPORTANT: id == 0 intentionally not used to allow timer_id = 0 initialization
    for (int i = 1; i < countof(glue->timers); i++) {
        if (glue->timers[i] == null) {
            id = i;
            break;
        }
    }
    if (id > 0) {
//      traceln("glue.timers[%d]=%p ns=%lld", id, tcb, tcb->ns);
        glue->timers[id] = tcb;
        enqueue_command(glue, COMMAND_TIMER); // will recalculate 'earliest' -> 'timer_next_ns'
    } else {
        assertion(false, "too many timers");
    }
    tcb->id = id;
    return id;
}

static void timer_remove(app_t* app, timer_callback_t* tcb) {
    glue_t* glue = (glue_t*)app->glue;
    int id = tcb->id;
    assertion(0 <= id && id < countof(glue->timers), "id=%d out of range [0..%d]", id, countof(glue->timers));
    if (0 < id && id < countof(glue->timers)) {
//      traceln("glue.timers[%d]", id);
        assertion(glue->timers[id] != null, "app->timers[%d] already null", id);
        if (glue->timers[id] != null) {
            glue->timers[id] = null;
            tcb->id = 0;
            tcb->last_fired = 0; // indicates that timer is not set see set_timer above
        }
        enqueue_command(glue, COMMAND_TIMER); // will recalculate 'earliest' -> 'timer_next_ns'
    }
}

static int vibration_effect_to_milliseconds(int effect) {
    switch (effect) {
        case DEFAULT_AMPLITUDE  : return 60;
        case EFFECT_CLICK       : return 30;
        case EFFECT_DOUBLE_CLICK: return 50;
        case EFFECT_TICK        : return 10;
        case EFFECT_HEAVY_TICK  : return 20;
        default: return 60;
    }
}

static const char* vibration_effect_to_string(int effect) {
    switch (effect) {
        case DEFAULT_AMPLITUDE  : return "DEFAULT_AMPLITUDE";
        case EFFECT_CLICK       : return "EFFECT_CLICK";
        case EFFECT_DOUBLE_CLICK: return "EFFECT_DOUBLE_CLICK";
        case EFFECT_TICK        : return "EFFECT_TICK";
        case EFFECT_HEAVY_TICK  : return "EFFECT_HEAVY_TICK";
        default: return "DEFAULT_AMPLITUDE";
    }
}

static void vibrate(app_t* app, int effect) {
    glue_t* glue = (glue_t*)app->glue;
    if (!android_jni_vibrate_with_effect(glue->na, vibration_effect_to_string(effect))) {
        android_jni_vibrate_milliseconds(glue->na, vibration_effect_to_milliseconds(effect));
    }
}

static const char* screen_size(int i) {
    switch (i) {
        case ACONFIGURATION_SCREENSIZE_ANY   : return "ANY";
        case ACONFIGURATION_SCREENSIZE_SMALL : return "SMALL";
        case ACONFIGURATION_SCREENSIZE_NORMAL: return "NORMAL";
        case ACONFIGURATION_SCREENSIZE_LARGE : return "LARGE";
        case ACONFIGURATION_SCREENSIZE_XLARGE: return "XLARGE";
        default: return "???";
    }
}

static const char* density(int i) {
    switch (i) {
        case ACONFIGURATION_DENSITY_DEFAULT: return "DEFAULT";
        case ACONFIGURATION_DENSITY_LOW    : return "LOW"; // 120dpi
        case ACONFIGURATION_DENSITY_MEDIUM : return "MEDIUM"; // 160dpi
        case ACONFIGURATION_DENSITY_TV     : return "TV"; // 213
        case ACONFIGURATION_DENSITY_HIGH   : return "HIGH"; // 240
        case ACONFIGURATION_DENSITY_XHIGH  : return "XHIGH"; // 320
        case ACONFIGURATION_DENSITY_XXHIGH : return "XXHIGH"; // 480
        case ACONFIGURATION_DENSITY_XXXHIGH: return "XXXHIGH"; // 640
        case ACONFIGURATION_DENSITY_ANY    : return "ANY";
        case ACONFIGURATION_DENSITY_NONE   : return "NONE";
        default: return "???";
    }
}

static const char* mode_type(int i) {
    switch (i) {
        case ACONFIGURATION_UI_MODE_TYPE_ANY       : return "ANY";
        case ACONFIGURATION_UI_MODE_TYPE_NORMAL    : return "NORMAL";
        case ACONFIGURATION_UI_MODE_TYPE_DESK      : return "DESK";
        case ACONFIGURATION_UI_MODE_TYPE_CAR       : return "CAR";
        case ACONFIGURATION_UI_MODE_TYPE_TELEVISION: return "TELEVISION";
        case ACONFIGURATION_UI_MODE_TYPE_APPLIANCE : return "APPLIANCE";
        case ACONFIGURATION_UI_MODE_TYPE_WATCH     : return "WATCH";
        default: return "???";
    }
}

static int trace_config;

static void trace_current_configuration(glue_t* glue) {
    char lang[2], country[2];
    AConfiguration_getLanguage(glue->config, lang);
    AConfiguration_getCountry(glue->config, country);
    if (trace_config) {
        traceln("config mcc=%d mnc=%d lang=%c%c cnt=%c%c smallest=%ddp %dx%ddp "
                "orien=%d touch=%d dens=%d %s "
                "keys=%d nav=%d keysHid=%d navHid=%d sdk=%d size=%d %s long=%d "
                "modetype=%d %s modenight=%d",
                AConfiguration_getMcc(glue->config),
                AConfiguration_getMnc(glue->config),
                lang[0], lang[1], country[0], country[1],
                AConfiguration_getSmallestScreenWidthDp(glue->config),
                AConfiguration_getScreenWidthDp(glue->config),
                AConfiguration_getScreenHeightDp(glue->config),
                AConfiguration_getOrientation(glue->config),
                AConfiguration_getTouchscreen(glue->config),
                AConfiguration_getDensity(glue->config),
                density(AConfiguration_getDensity(glue->config)),
                AConfiguration_getKeyboard(glue->config),
                AConfiguration_getNavigation(glue->config),
                AConfiguration_getKeysHidden(glue->config),
                AConfiguration_getNavHidden(glue->config),
                AConfiguration_getSdkVersion(glue->config),
                AConfiguration_getScreenSize(glue->config),
                screen_size(AConfiguration_getScreenSize(glue->config)),
                AConfiguration_getScreenLong(glue->config),
                AConfiguration_getUiModeType(glue->config),
                mode_type(AConfiguration_getUiModeType(glue->config)),
                AConfiguration_getUiModeNight(glue->config));
    }
}

static void process_configuration(glue_t* glue) {
    trace_current_configuration(glue);
    // I do not know a way to distuinguish between Androids with build in QWERTY keyboars
    // and external connected keyboards:
    glue->keyboad_present = AConfiguration_getKeyboard(glue->config) == ACONFIGURATION_KEYBOARD_QWERTY;
    // AConfiguration_getSmallestScreenWidthDp() is actually the real width in DP
    // the name is of the function is very confusing (prehaps Android G1 landscape legacy)...
    int32_t dp_w_min = AConfiguration_getSmallestScreenWidthDp(glue->config);
    int dp_w = AConfiguration_getScreenWidthDp(glue->config);
    int dp_h = max(AConfiguration_getScreenHeightDp(glue->config), dp_w_min);
//  traceln("%dx%ddp SmallestScreenWidth %d", dp_w, dp_h, dp_w_min);
    glue->inches_wide = dp_w / 120.0f; // legacy but still true up to Android Pixel 3a
    glue->inches_high = dp_h / 120.0f;
    glue->density = AConfiguration_getDensity(glue->config); // obscure and not true DPI
//  traceln("keyboard=%d density=%d %.2fx%.2f inches", glue->keyboad_present, AConfiguration_getDensity(glue->config), glue->inches_wide, glue->inches_high);
}

static void process_input(glue_t* glue, android_poll_source_t* source) {
    AInputEvent* event = null;
    while (AInputQueue_getEvent(glue->input_queue, &event) >= 0) {
//      int i = AInputEvent_getType(event);
//      const char* s = "???";
//      switch (i) {
//          case AINPUT_EVENT_TYPE_KEY   : s = "KEY";    break;
//          case AINPUT_EVENT_TYPE_MOTION: s = "MOTION"; break;
//          default: assertion(false, "i=%d", i); break;
//      }
//      traceln("input event: type=%s %d", s, i);
        if (AInputQueue_preDispatchEvent(glue->input_queue, event)) {
            continue;
        }
        glue->a->time_in_nanoseconds = time_monotonic_ns() - glue->start_time_in_ns;
        int32_t handled = handle_input(glue, event);
        AInputQueue_finishEvent(glue->input_queue, event, handled);
    }
}

static void enqueue_command(glue_t* glue, int8_t command) {
//  traceln("%s", cmd2str(command));
    if (write(glue->write_pipe, &command, sizeof(command)) != sizeof(command)) {
        traceln("Failure writing a command: %s", strerror(errno));
    }
    ALooper_wake(glue->looper);
}

static int8_t dequeue_command(glue_t* glue) {
    int8_t command;
    if (read(glue->read_pipe, &command, sizeof(command)) == sizeof(command)) {
        return command;
    } else {
        traceln("No data in command pipe");
        return -1;
    }
}

static void process_command(glue_t* glue, android_poll_source_t* source) {
    int8_t command = dequeue_command(glue);
//  traceln("%.3fms %s", glue->a->time_in_nanoseconds / (double)NS_IN_SEC, cmd2str(command));
    switch (command) {
        case COMMAND_REDRAW: draw_frame(glue); break;
        case COMMAND_TIMER : on_timer(glue);   break;
        case COMMAND_QUIT  :
            ANativeActivity_finish(glue->na);
            traceln("exit(%d)", glue->exit_code);
            exit(glue->exit_code);
            break;
        default: traceln("unhandled command=%d %s", command, cmd2str(command));
    }
}

static void process_accel(glue_t* glue, android_poll_source_t* source) {
}

static void on_create(ANativeActivity* na, void* saved_state_data, size_t saved_state_bytes) {
    glue_t* glue = (glue_t*)na->instance;
    if (glue->a->init != null) { glue->a->init(glue->a); }
}

static void on_destroy(ANativeActivity* na) {
    glue_t* glue = (glue_t*)na->instance;
    assert(glue->sensor_event_queue != null);
    if (glue->sensor_event_queue != null) {
        ASensorManager_destroyEventQueue(glue->sensor_manager, glue->sensor_event_queue);
        glue->sensor_event_queue = null;
    }
    glue->destroy_requested = true;
    if (glue->timer_thread != 0) {
        synchronized_signal(glue->mutex, glue->cond, glue->timer_next_ns = 0); // wake up timer_thread
        pthread_join(glue->timer_thread, null); glue->timer_thread = 0;
    }
    pthread_mutex_destroy(&glue->mutex);
    pthread_cond_destroy(&glue->cond);
}

static glue_t glue;

void ANativeActivity_onCreate(ANativeActivity* na, void* saved_state_data, size_t saved_state_bytes) {
    traceln("pid/tid=%d/%d", getpid(), gettid());
    glue.na = na;
    glue.a = &app;
    na->instance = &glue;
    app.glue = &glue;
    glue.start_time_in_ns = time_monotonic_ns();
    ANativeActivityCallbacks* cb = na->callbacks;
    cb->onDestroy                  = on_destroy;
    cb->onStart                    = on_start;
    cb->onResume                   = on_resume;
    cb->onSaveInstanceState        = on_save_instance_state;
    cb->onPause                    = on_pause;
    cb->onStop                     = on_stop;
    cb->onConfigurationChanged     = on_configuration_changed;
    cb->onLowMemory                = on_low_memory;
    cb->onWindowFocusChanged       = on_window_focus_changed;
    cb->onNativeWindowCreated      = on_native_window_created;
    cb->onNativeWindowResized      = on_native_window_resized;
    cb->onNativeWindowRedrawNeeded = on_native_window_redraw_needed;
    cb->onNativeWindowDestroyed    = on_native_window_destroyed;
    cb->onInputQueueCreated        = on_input_queue_created;
    cb->onInputQueueDestroyed      = on_input_queued_destroyed;
    cb->onContentRectChanged       = on_content_rect_changed;
    static const int add    = AWINDOW_FLAG_KEEP_SCREEN_ON|
                              AWINDOW_FLAG_TURN_SCREEN_ON|
                              AWINDOW_FLAG_FULLSCREEN|
                              AWINDOW_FLAG_LAYOUT_IN_SCREEN|
                              AWINDOW_FLAG_LAYOUT_INSET_DECOR;
    static const int remove = AWINDOW_FLAG_SCALED|
                              AWINDOW_FLAG_DITHER|
                              AWINDOW_FLAG_FORCE_NOT_FULLSCREEN|
                              AWINDOW_FLAG_LAYOUT_NO_LIMITS;
    ANativeActivity_setWindowFlags(na, add, remove);
    int pipe_ends[2];
    if (pipe(pipe_ends)) {
        traceln("could not create pipe: %s", strerror(errno));
        ANativeActivity_finish(na);
        abort();
        return;
    }
    glue.read_pipe  = pipe_ends[0];
    glue.write_pipe = pipe_ends[1];
    glue.animating = 0;
    glue.config = AConfiguration_new();
    AConfiguration_fromAssetManager(glue.config, na->assetManager);
    process_configuration(&glue);
    glue.command_poll_source.id = LOOPER_ID_MAIN;
    glue.command_poll_source.glue = &glue;
    glue.command_poll_source.process = process_command;
    glue.input_poll_source.id = LOOPER_ID_INPUT;
    glue.input_poll_source.glue = &glue;
    glue.input_poll_source.process = process_input;
    glue.accel_poll_source.id = LOOPER_ID_ACCEL;
    glue.accel_poll_source.glue = &glue;
    glue.accel_poll_source.process = process_accel;
    glue.looper = ALooper_forThread(); // ALooper_prepare(0);
    ALooper_addFd(glue.looper, glue.read_pipe, LOOPER_ID_MAIN, ALOOPER_EVENT_INPUT, looper_callback, &glue.command_poll_source);
    glue.sensor_manager = ASensorManager_getInstance();
    glue.accelerometer_sensor = ASensorManager_getDefaultSensor(glue.sensor_manager, ASENSOR_TYPE_ACCELEROMETER);
    glue.sensor_event_queue = ASensorManager_createEventQueue(glue.sensor_manager, glue.looper, LOOPER_ID_ACCEL, looper_callback, &glue.accel_poll_source);
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
    pthread_cond_init(&glue.cond, &cond_attr);
    pthread_mutex_init(&glue.mutex, null);
    assert(glue.timer_thread == 0);
    int r = pthread_create(&glue.timer_thread, null, timer_thread, &glue);
    assert(r == 0);
    on_create(na, saved_state_data, saved_state_bytes);
}

static_init(app_android) {
    app_create(&app);
    root.a = &app;
}

END_C
