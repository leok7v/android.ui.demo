#include "app.h"
#include "ui.h"
#include "button.h"
#include "android_na.h"  // Android Native Activity
#include "android_jni.h" // interface to Java only code (going via binder is too involving)
#include <EGL/egl.h>
#include <android/sensor.h>
#include <android/asset_manager.h>
#include <android/log.h>

BEGIN_C

typedef struct glue_s {
    android_na_t* a;
    ASensorManager* sensor_manager;
    const ASensor* accelerometer_sensor;
    ASensorEventQueue* sensor_event_queue;
    int animating;
    int redraw;
    int shown; // app.shown() has been called
    timer_callback_t* timers[32]; // maximum 32 timers allowed to avoid alloc and linked list
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    ALooper* looper;
} glue_t;

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

static void  app_quit();
static void  app_exit(int code);
static void  app_animate(int animating);
static void  app_invalidate();
static void  focus(ui_t * ui);
static int   timer_add(app_t* app, timer_callback_t* tcb);
static void  timer_remove(app_t* app, timer_callback_t* tcb);
static void* asset_map(app_t* a, const char* name, const void* *data, int* bytes);
static void  asset_unmap(app_t* a, void* asset, const void* data, int bytes);
static void  vibrate(app_t* a, int vibration_effect);

static app_t app = {
    /* that:    */ null,
    /* context: */ null,
    /* root:    */ &root,
    /* focused: */ null,
    /* xdpi: */ 0,
    /* ydpi: */ 0,
    /* keyboard_flags: */ 0,
    /* mouse_flags:    */ 0,
    /* last_mouse_x:   */ 0,
    /* last_mouse_y:   */ 0,
    /* time_in_nanoseconds */ 0LL,
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

static void* asset_map(app_t* a, const char* name, const void* *data, int *bytes) {
    glue_t* glue = (glue_t*)a->context;
    AAsset* asset = AAssetManager_open(glue->a->activity->assetManager, name, AASSET_MODE_BUFFER);
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
    ANativeWindow_setBuffersGeometry(glue->a->window, 0, 0, format);
    EGLSurface surface = eglCreateWindowSurface(display, config, glue->a->window, null);
    EGLContext context = eglCreateContext(display, config, null, null);
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
    app.root->w = w;
    app.root->h = h;
    app.xdpi = w / glue->a->inches_wide;
    app.ydpi = h / glue->a->inches_high;
    return 0;
}

static void focus(ui_t* ui) {
    // ui can be null, thus cannot used ui->a
    if (ui == app.focused) {
//      traceln("already focused %p", ui);
    } else if (ui == null || ui->focusable) {
        if (app.focused != null && app.focused->focus != null) {
            app.focused->focus(app.focused, false);
        }
        app.focused = ui;
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
        glue->redraw = 0;
        if (!glue->shown) {
            if (app.shown != null) { app.shown(&app); }
            gl_init(app.root->w, app.root->h);
            glue->shown = 1;
        }
        if (!app.root->hidden && app.root->draw != null) {
            app.root->draw(app.root);
            // eglSwapBuffers performs an implicit flush operation on the context (glFlush for an OpenGL ES)
            bool swapped = eglSwapBuffers(glue->display, glue->surface);
            assertion(swapped, "eglSwapBuffers() failed"); (void)swapped;
//          traceln("eglSwapBuffers()=%d", swapped);
            gl_set_color(&gl_color_invalid); // this is needed for glColorf() calls optimization
        }
    }
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
    glue->animating = 0;
    glue->redraw = 0;
    glue->display = EGL_NO_DISPLAY;
    glue->context = EGL_NO_CONTEXT;
    glue->surface = EGL_NO_SURFACE;
    glue->shown = 0;
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
        if (focus_was_set) { ui->a->focus(ui); }
    }
//  traceln("focus_was_set=%d on %p", focus_was_set, ui);
    return focus_was_set;
}

static void dispatch_keycode(int flags, int keycode) {
    app.keyboard_flags = flags;
    if (app.focused != null && app.focused->keyboard != null) {
        app.focused->keyboard(app.focused, flags, keycode);
    }
    if (flags & KEYBOARD_KEY_PRESSED) {
        int f = flags & ~(KEYBOARD_KEY_PRESSED|KEYBOARD_SHIFT|KEYBOARD_NUMLOCK|KEYBOARD_CAPSLOCK);
        dispatch_keyboard_shortcuts(app.root, f, keycode);
    }
}

static int32_t handle_input(android_na_t* a, AInputEvent* event) {
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
            int mouse_flags = app.mouse_flags;
            switch (action) {
                case AMOTION_EVENT_ACTION_DOWN        : mouse_action = MOUSE_LBUTTON_DOWN; mouse_flags |=  MOUSE_LBUTTON_FLAG; break;
                case AMOTION_EVENT_ACTION_UP          : mouse_action = MOUSE_LBUTTON_UP;   mouse_flags &= ~MOUSE_LBUTTON_FLAG; break;
                case AMOTION_EVENT_ACTION_HOVER_MOVE  : mouse_action = MOUSE_MOVE; break;
                case AMOTION_EVENT_ACTION_POINTER_DOWN: break; // touch event, "index" is "finger index"
                case AMOTION_EVENT_ACTION_POINTER_UP  : break;
                case AMOTION_EVENT_ACTION_SCROLL      : traceln("TODO: AMOTION_EVENT_ACTION_SCROLL"); break; // mouse wheel generates it
                default: break;
            }
            app.mouse_flags = mouse_flags;
            app.last_mouse_x = x;
            app.last_mouse_y = y;
            if (app.trace_flags & APP_TRACE_MOUSE) { app_trace_mouse(app.mouse_flags, mouse_action, index, x, y); }
            if (mouse_action & MOUSE_LBUTTON_DOWN) {
                if (!set_focus(app.root, x, y)) {
                    app.focus(null); // kill focus if no focusable components were found
                }
            }
            if (app.root->mouse != null) { app.root->mouse(app.root, mouse_action, x, y); }
            if (app.root->screen_mouse != null) { app.root->screen_mouse(app.root, mouse_action, x, y); }
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
                for (int i = 0; i < rc; i++) { dispatch_keycode(flags | KEYBOARD_KEY_REPEAT, keycode); }
            } else {
                dispatch_keycode(flags, keycode);
            }
        }
        return 1;
    }
    return 0;
}

static void handle_command(android_na_t* a, int32_t command) { // Process next command
    glue_t* glue = (glue_t*)a->that;
    switch (command) {
        case ANDROID_NA_COMMAND_CREATE:
            if (app.init != null) {
                app.context = a->that;
                app.init(&app);
            }
            break;
        case ANDROID_NA_COMMAND_SAVE_STATE:
            break;
        case ANDROID_NA_COMMAND_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (glue->a->window != null) {
                init_display(glue);
                draw_frame(glue);
            }
            break;
        case ANDROID_NA_COMMAND_TERM_WINDOW:
            term_display(glue);
            if (app.hidden != null) {
                app.hidden(&app);
            }
            break;
        case ANDROID_NA_COMMAND_GAINED_FOCUS:
        case ANDROID_NA_COMMAND_RESUME:
            // When our app gains focus, we start monitoring the accelerometer.
            if (glue->accelerometer_sensor != null) {
                ASensorEventQueue_enableSensor(glue->sensor_event_queue, glue->accelerometer_sensor);
                // ask for 60 events per second (in microseconds).
                ASensorEventQueue_setEventRate(glue->sensor_event_queue, glue->accelerometer_sensor, 1000 * 1000 / 60);
            }
            if (command == ANDROID_NA_COMMAND_RESUME && app.resume != null) {
                app.resume(&app); // Leo: it is up to application code to resume animation if necessary
            }
            // Leo - added draw frame on ANDROID_NA_COMMAND_GAINED_FOCUS and ANDROID_NA_COMMAND_RESUME
            glue->redraw = 1;
            draw_frame(glue);
//          ANativeActivity_showSoftInput(glue->a->activity, ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED);
            break;
        case ANDROID_NA_COMMAND_LOST_FOCUS:
        case ANDROID_NA_COMMAND_PAUSE:
        case ANDROID_NA_COMMAND_STOP:
//          ANativeActivity_hideSoftInput(glue->a->activity, ANATIVEACTIVITY_HIDE_SOFT_INPUT_NOT_ALWAYS);
            // When app loses focus, we stop monitoring the accelerometer. This is to avoid consuming battery while not being used.
            if (glue->accelerometer_sensor != null) {
                ASensorEventQueue_disableSensor(glue->sensor_event_queue, glue->accelerometer_sensor);
            }
            glue->animating = 0; // Also stop animating.
            glue->redraw = 0;
            draw_frame(glue); // Leo: experimentally needed to redraw activity manager on pause activity... Not clear why...
            if (command == ANDROID_NA_COMMAND_PAUSE && app.pause != null) {
                app.pause(&app);
            }
            if (command == ANDROID_NA_COMMAND_STOP && app.stop != null) {
                app.stop(&app);
            }
            break;
        case ANDROID_NA_COMMAND_DESTROY:
            if (app.destroy != null) {
                app.destroy(&app);
            }
            app.context = null;
            break;
        default:
            break;
    }
}

static glue_t glue;

static int timer_add(app_t* app, timer_callback_t* tcb) {
    assertion(tcb->last_fired_milliseconds == 0, "last_fired_milliseconds=%lld != 0", tcb->last_fired_milliseconds);
    assertion(tcb->milliseconds > 0, "milliseconds=%lld must be > 0", tcb->milliseconds);
    if (tcb->milliseconds < 0) { return -1; }
    tcb->last_fired_milliseconds = 0;
    int id = 0;
    // IMPORTANT: id == 0 intentionally not used to allow timer_id = 0 initialization
    for (int i = 1; i < countof(glue.timers); i++) {
        if (glue.timers[i] == null) {
            id = i;
            break;
        }
    }
    if (id > 0) {
//      traceln("glue.timers[%d]=%p", id, tcb);
        glue.timers[id] = tcb;
    }
    tcb->id = id;
    return id;
}

static void timer_remove(app_t* app, timer_callback_t* tcb) {
    int id = tcb->id;
    assertion(0 <= id && id < countof(glue.timers), "id=%d out of range [0..%d]", id, countof(glue.timers));
    if (0 < id && id < countof(glue.timers)) {
        assertion(glue.timers[id] != null, "app->timers[%d] already null", id);
        if (glue.timers[id] != null) {
            glue.timers[id]->last_fired_milliseconds = 0; // indicates that timer is not set see set_timer above
            glue.timers[id] = null;
            tcb->id = 0;
            tcb->last_fired_milliseconds = 0;
        }
    }
}

static void app_animate(int animating) {
    glue.animating = animating;
}

static void app_invalidate() {
    glue.redraw = true;
    ALooper_wake(glue.looper);
}

static void app_quit() {
    ANativeActivity_finish(glue.a->activity);
}

static void app_exit(int code) {
    glue.a->exit_code = code;
    glue.a->exit_requested = 1;
    ANativeActivity_finish(glue.a->activity);
}

enum {
    NS_IN_MS = 1000000,
    NS_IN_SEC = NS_IN_MS * 1000
};

static uint64_t time_ns() {
    struct timespec tm = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &tm);
    return NS_IN_SEC * (int64_t)tm.tv_sec + tm.tv_nsec;
}

void android_main(android_na_t* a) {
    a->that = &glue;
    a->on_command = handle_command;
    a->on_input_event = handle_input;
    glue.a = a;
    glue.looper = ALooper_forThread();
    // Prepare to monitor accelerometer
    glue.sensor_manager = ASensorManager_getInstance();
    glue.accelerometer_sensor = ASensorManager_getDefaultSensor(glue.sensor_manager, ASENSOR_TYPE_ACCELEROMETER);
    glue.sensor_event_queue = ASensorManager_createEventQueue(glue.sensor_manager, a->looper, LOOPER_ID_USER, null, null);
    glue.animating = 0;
    uint64_t start_time_in_nanoseconds = time_ns();
    while (!a->destroy_requested) {
        android_poll_source_t* source;
        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        int count = 0;
        int events = 0;
        bool timer_present = false;
        for (int i = 1; i < countof(glue.timers) && !timer_present; i++) {
            timer_present = glue.timers[i] != null;
        }
        int milliseconds = glue.animating || glue.redraw ? 0 : (timer_present ? 1 : 8);
        int ident = ALooper_pollAll(milliseconds, null, &events, (void**)&source);
        app.time_in_nanoseconds = (int64_t)(time_ns() - start_time_in_nanoseconds);
        assertion(app.time_in_nanoseconds >= 0, "application is running for too long app.time_in_nanoseconds=%llX wrap around", (long long)app.time_in_nanoseconds);
        while (ident >= 0) {
            // Process this event.
            if (source != null) {
                source->process(a, source);
            }
            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {
                if (glue.accelerometer_sensor != null) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(glue.sensor_event_queue, &event, 1) > 0) {
//                      traceln("accelerometer: x=%f y=%f z=%f", event.acceleration.x, event.acceleration.y, event.acceleration.z);
                    }
                }
            }
            if (a->destroy_requested) { break; }
            ident = ALooper_pollAll(glue.animating || glue.redraw ? 0 : 8, null, &events, (void**)&source);
        }
        if (timer_present) {
            int64_t now = app.time_in_nanoseconds / NS_IN_MS;
            for (int i = 1; i < countof(glue.timers); i++) {
                timer_callback_t* tc = glue.timers[i];
                if (tc != null) {
                    if (tc->last_fired_milliseconds == 0) {
                        // first time event is seen not actually calling timer callback here
                        tc->last_fired_milliseconds = now;
                    } else if (now > tc->last_fired_milliseconds + tc->milliseconds) {
                        tc->callback(tc);
                        // timer maight have been removed and deallocated
                        // inside callback call (not a good practice)
                        if (tc == glue.timers[i]) { tc->last_fired_milliseconds = now; }
                    }
                }
            }
        }
        if (glue.animating || glue.redraw) {
            draw_frame(&glue);
        }
        if (count == 0 && app.idle != null) {
            app.idle(&app);
        }
    }
    term_display(&glue);
}

static int vibration_effect_to_milliseconds(int effect) {
    switch (effect) {
        case DEFAULT_AMPLITUDE  : return 100;
        case EFFECT_CLICK       : return 60;
        case EFFECT_DOUBLE_CLICK: return 120;
        case EFFECT_TICK        : return 30;
        case EFFECT_HEAVY_TICK  : return 50;
        default: return 100;
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

static void vibrate(app_t* a, int effect) {
    if (false) { //  This does not work because of two threaded NativeActivity model
        if (!android_jni_vibrate_with_effect(glue.a->activity, vibration_effect_to_string(effect))) {
            android_jni_vibrate_milliseconds(glue.a->activity, vibration_effect_to_milliseconds(effect));
        }
    }
}

static_init(app_android) {
    app_create(&app);
    root.a = &app;
}

END_C
