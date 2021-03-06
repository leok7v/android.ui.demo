/* Copyright 2020 "Leo" Dmitry Kuznetsov https://leok7v.github.io/
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */
#include "app.h"
#include "droid_keys.h"
#include "droid_jni.h" // interface to Java only code (going via binder is too involving)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/configuration.h>
#include <android/sensor.h>
#include <android/asset_manager.h>
#include <android/window.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/log.h>
#include <android/hardware_buffer.h>
#include <android/hardware_buffer_jni.h>

// see:
// https://github.com/aosp-mirror/platform_frameworks_base/blob/master/core/java/android/app/NativeActivity.java
// https://github.com/aosp-mirror/platform_frameworks_base/blob/master/core/jni/android_app_NativeActivity.cpp

begin_c

#if !(ANDROID_API > 0)
#pragma message("Invalid ANDROID_API '" stringify(ANDROID_API)) "'"
#endif

typedef struct glue_s glue_t;

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

typedef struct app_state_s {
    byte data[4 * 1024]; // state data here. TODO: expose to app.h in more flexible manner
} packed app_state_t;

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
    ASensorManager* sensor_manager;
    const ASensor*  accelerometer_sensor;
    ASensorEventQueue* sensor_event_queue;
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
    EGLint max_tex_w;
    EGLint max_tex_h;
    bool  keyboad_present; // is keyboard connected?
    ALooper* looper; // The ALooper associated with the app's main event dispatch thread.
    AInputQueue* input_queue; // When non-NULL, this is the input queue from which the app will receive user input events.
    ANativeWindow* window; // When non-NULL, this is the window surface that the app can draw in.
    // This is non-zero when the application's activity is destroyed process should exit.
    int exit_requested;
    int exit_code; // status to exit() with
    int read_pipe;
    int write_pipe;
    android_poll_source_t command_poll_source;
    android_poll_source_t input_poll_source;
    android_poll_source_t accel_poll_source;
    droid_display_metrics_t dm;
    app_state_t state; // TODO: this is stub for testing on_save_state() on_create() state passing
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

static int logln(int level, const char* tag, const char* location, const char* format, va_list vl) {
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

static void enqueue_command(glue_t* glue, int8_t command) {
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
        return -1;
    }
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

static void process_configuration(glue_t* glue) {
    glue->keyboad_present = AConfiguration_getKeyboard(glue->config) == ACONFIGURATION_KEYBOARD_QWERTY;
}

static int init_display(glue_t* glue) {
    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_BLUE_SIZE,  8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE,   8,
//      EGL_SAMPLE_BUFFERS, 1, // this is needed for 4xMSAA which does not work...
//      EGL_SAMPLES, 4, // 4x MSAA (multisample anti-aliasing)
        EGL_NONE
    };
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, 0, 0);
    EGLint configs_count = 0;
    EGLConfig config = 0;
    eglChooseConfig(display, attribs, &config, 1, &configs_count);
    EGLint id = 0;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID,   &id);
    eglGetConfigAttrib(display, config, EGL_MAX_PBUFFER_WIDTH,  &glue->max_tex_w);
    eglGetConfigAttrib(display, config, EGL_MAX_PBUFFER_HEIGHT, &glue->max_tex_h);
    uint32_t format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
    ANativeWindow_setBuffersGeometry(glue->window, 0, 0, format);
    EGLSurface surface = eglCreateWindowSurface(display, config, glue->window, null);
    EGLContext context = EGL_NO_CONTEXT;
    for (int gles = 3; gles >= 2; gles--) {
        EGLint context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, gles, EGL_NONE };
        context = eglCreateContext(display, config, null, context_attributes);
    }
    if (context == EGL_NO_CONTEXT) {
        traceln("eglCreateContext() failed"); exit(1);
    }
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
    return 0;
}

static void focus(app_t* app, ui_t* u) {
    // ui can be null, thus cannot used ui->a
    if (u == app->focused) {
        // already focused
    } else if (u == null || u->focusable) {
        if (app->focused != null && app->focused->focus != null) {
            app->focused->focus(app->focused, false);
        }
        app->focused = u;
        if (u != null && u->focus != null) {
            u->focus(u, true);
        }
    }
}

static void draw_frame(glue_t* glue) {
    if (glue->display != null) {
        ((app_t*)glue->a)->draw((app_t*)glue->a);
        // eglSwapBuffers performs an implicit flush operation on the context (glFlush for an OpenGL ES)
        bool swapped = eglSwapBuffers(glue->display, glue->surface);
        assertion(swapped, "eglSwapBuffers() failed"); (void)swapped;
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
    glue->display = EGL_NO_DISPLAY;
    glue->context = EGL_NO_CONTEXT;
    glue->surface = EGL_NO_SURFACE;
}

static int32_t handle_motion(glue_t* glue, AInputEvent* me) {
    assert(AInputEvent_getType(me) == AINPUT_EVENT_TYPE_MOTION);
    app_t* a = glue->a;
    int x = AMotionEvent_getX(me, 0);
    int y = AMotionEvent_getY(me, 0);
    int32_t act   = AMotionEvent_getAction(me);
    int32_t index = (act & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> 8; // fragile. >> 8 assumes AMOTION_EVENT_ACTION_POINTER_INDEX_MASK = 0xFF00
    act = act & AMOTION_EVENT_ACTION_MASK;
    int action = 0;
    int flags = a->touch_flags;
    switch (act) {
        case AMOTION_EVENT_ACTION_DOWN        : action = TOUCH_DOWN; flags |=  MOUSE_LBUTTON_FLAG; break;
        case AMOTION_EVENT_ACTION_UP          : action = TOUCH_UP;   flags &= ~MOUSE_LBUTTON_FLAG; break;
        case AMOTION_EVENT_ACTION_HOVER_MOVE  : action = TOUCH_MOVE; break;
        case AMOTION_EVENT_ACTION_POINTER_DOWN: break; // touch event, "index" is "finger index"
        case AMOTION_EVENT_ACTION_POINTER_UP  : break;
        // mouse wheel and deodorant ball generates scrolls:
        case AMOTION_EVENT_ACTION_SCROLL      : traceln("TODO: AMOTION_EVENT_ACTION_SCROLL"); break;
        default: break;
    }
    if (index == 0) {
        a->touch_flags = flags;
        a->last_touch_x = x;
        a->last_touch_y = y;
    }
    assertion(a->touch != null, "touch() cannot be null");
    a->touch(a, index, action, x, y);
    return 1;
}

static int32_t handle_key(glue_t* glue, AInputEvent* ke) {
    assert(AInputEvent_getType(ke) == AINPUT_EVENT_TYPE_KEY);
    int32_t key_code = AKeyEvent_getKeyCode(ke);
    int32_t act = AKeyEvent_getAction(ke);
    int32_t rc = 1;
    switch (act) {
        case AKEY_EVENT_ACTION_UP      : break;
        case AKEY_EVENT_ACTION_DOWN    : break;
        case AKEY_EVENT_ACTION_MULTIPLE: rc = AKeyEvent_getRepeatCount(ke); break;
        default: traceln("unknown action: %d", act);
    }
    int meta_state = AKeyEvent_getMetaState(ke);
    int flags = KEYBOARD_KEY_PRESSED;
    int kc = droid_keys_translate(key_code, meta_state, flags);
    if (kc != 0) {
        switch (act) {
            case AKEY_EVENT_ACTION_DOWN: flags |= KEYBOARD_KEY_PRESSED;  flags &= ~KEYBOARD_KEY_RELEASED; break;
            case AKEY_EVENT_ACTION_UP:   flags |= KEYBOARD_KEY_RELEASED; flags &= ~KEYBOARD_KEY_PRESSED; break;
            default: break;
        }
        app_t* a = glue->a;
        if (act == AKEY_EVENT_ACTION_MULTIPLE) {
            for (int i = 0; i < rc; i++) { a->key(a, flags | KEYBOARD_KEY_REPEAT, kc); }
        } else {
            a->key(a, flags, kc);
        }
    }
    return 1;
}

static int32_t handle_input(glue_t* glue, AInputEvent* ie) {
    if (AInputEvent_getType(ie) == AINPUT_EVENT_TYPE_MOTION) {
        return handle_motion(glue, ie);
    } else if (AInputEvent_getType(ie) == AINPUT_EVENT_TYPE_KEY) {
        return handle_key(glue, ie);
    } else {
        return 0;
    }
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
        int us = ASensor_getMinDelay(glue->accelerometer_sensor); // 10,000 microseconds = 100Hz
        ASensorEventQueue_setEventRate(glue->sensor_event_queue, glue->accelerometer_sensor, us);
    }
    glue->a->resume(glue->a); // it is up to application code to resume animation if necessary
    // Leo - added draw frame on ANDROID_NA_COMMAND_GAINED_FOCUS and ANDROID_NA_COMMAND_RESUME
    draw_frame(glue);
    enqueue_command(glue, COMMAND_TIMER); // timers were stopped on pause, wake them up
}

// It is unclear *when* Android Activity Manager actually calls on_save_state().
// Definetely not always but for sure on split screen rotation.

static void* on_save_state(ANativeActivity* na, size_t* bytes) {
    static const char test[] = "This is test of saved state which will come back on_create()";
    glue_t* glue = (glue_t*)na->instance;
    strncpy((char*)glue->state.data, test, countof(glue->state.data) - 1);
    glue->state.data[countof(glue->state.data) - 1] = 0; // always zero terminated even when truncated
    // code below calls free() on returned pointer:
    // https://github.com/aosp-mirror/platform_frameworks_base/blob/d0ebaa9e30cb6f359bb14c52cdf7f474b1816af5/core/jni/android_app_NativeActivity.cpp#L460
    app_state_t* s = (app_state_t*)malloc(sizeof(glue->state));
    *bytes = strlen(test) + 1;
    *s = glue->state;
    return s;
}

static void on_native_window_created(ANativeActivity* na, ANativeWindow* window) {
    glue_t* glue = (glue_t*)na->instance;
    app_t* a = glue->a;
    // The window is being shown, get it ready.
    assert(glue->window == null && window != null);
    glue->window = window;
    init_display(glue);
    assertion(a->shown != null, "shown() cannot be null");
    a->shown(a, ANativeWindow_getWidth(window), ANativeWindow_getHeight(window));
    sys.invalidate(a);
}

static void on_native_window_destroyed(ANativeActivity* na, ANativeWindow* window) {
    glue_t* glue = (glue_t*)na->instance;
    app_t* a = glue->a;
    assert(glue->window == window);
    term_display(glue);
    if (a->hidden != null) { a->hidden(a); }
    sys.focus(a, null); // focus is lost
    glue->window = null;
}

static void on_window_focus_changed(ANativeActivity* na, int gained) {
}

static void on_configuration_changed(ANativeActivity* na) {
    glue_t* glue = (glue_t*)na->instance;
    process_configuration(glue);
}

static void on_low_memory(ANativeActivity* na) {
    // TODO: pass it to the application
}

static void on_native_window_resized(ANativeActivity* na, ANativeWindow* window) {
    glue_t* glue = (glue_t*)na->instance;
    assert(glue->window == window);
    sys.invalidate(glue->a); // enqueue redraw command
}

static void on_native_window_redraw_needed(ANativeActivity* na, ANativeWindow* window) {
    glue_t* glue = (glue_t*)na->instance;
    assert(glue->window == window);
    sys.invalidate(glue->a); // enqueue redraw command
}

static int looper_callback(int fd, int events, void* data) {
    android_poll_source_t* ps = (android_poll_source_t*)data;
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
    app_t* a = glue->a;
    assertion(a->resized != null, "resized() cannot be null");
    a->resized(a, rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top);
    sys.invalidate(a);
}

static void invalidate(app_t* app) {
    glue_t* glue = (glue_t*)app->glue;
    enqueue_command(glue, COMMAND_REDRAW);
}

static void quit(app_t* app) {
    glue_t* glue = (glue_t*)app->glue;
    ANativeActivity_finish(glue->na);
}

static void exit_(app_t* app, int code) {
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
        );
        if (!glue->destroy_requested) {
            enqueue_command(glue, COMMAND_TIMER);
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
            }
        }
    }
    if (earliest < 0 || !glue->running) { earliest = FOREVER; } // wait `forever' or until next wakeup
    if (earliest != glue->timer_next_ns) {
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
    if (!droid_jni_vibrate_with_effect(glue->na, vibration_effect_to_string(effect))) {
        droid_jni_vibrate_milliseconds(glue->na, vibration_effect_to_milliseconds(effect));
    }
}

static void show_keyboard(app_t* app, bool on) {
    glue_t* glue = (glue_t*)app->glue;
    if (on) { // ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED means "even if physical keyboard is present"
        droid_jni_show_keyboard(glue->na, true, ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED);
    } else {
        droid_jni_show_keyboard(glue->na, false, 0);
    }
}

static void process_input(glue_t* glue, android_poll_source_t* source) {
    AInputEvent* ie = null;
    while (AInputQueue_getEvent(glue->input_queue, &ie) >= 0) {
        if (!AInputQueue_preDispatchEvent(glue->input_queue, ie)) {
            glue->a->time_in_nanoseconds = time_monotonic_ns() - glue->start_time_in_ns;
            int32_t handled = handle_input(glue, ie);
            AInputQueue_finishEvent(glue->input_queue, ie, handled);
        }
    }
}

static void process_command(glue_t* glue, android_poll_source_t* source) {
    int8_t command = dequeue_command(glue);
    switch (command) {
        case COMMAND_REDRAW: draw_frame(glue); break;
        case COMMAND_TIMER : on_timer(glue);   break;
        case COMMAND_QUIT  :
            ANativeActivity_finish(glue->na);
            exit(glue->exit_code);
            break;
        default: traceln("unhandled command=%d %s", command, cmd2str(command));
    }
}

static void process_accel(glue_t* glue, android_poll_source_t* source) {
}

static void on_destroy(ANativeActivity* na) {
    traceln("pid/tid=%d/%d na=%p", getpid(), gettid(), na);
    glue_t* glue = (glue_t*)na->instance;
    app_t* a = glue->a;
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
    close(glue->read_pipe);  glue->read_pipe = 0;
    close(glue->write_pipe); glue->write_pipe = 0;
    AConfiguration_delete(glue->config); glue->config = 0;
    glue->sensor_manager = null; // shared instance do not hold on to
    glue->accelerometer_sensor = null;
    if (a->done != null) { a->done(a); }
    assert(glue->display == null);
    assert(glue->surface == null);
    assert(glue->context == null);
    a->focused = null;
    glue->na = null;
}

static void display_real_size(glue_t* glue, ANativeActivity* na) {
    app_t* a = glue->a;
    droid_jni_get_display_real_size(na, &glue->dm);
    glue->density = glue->dm.scaled_density;
    a->xdpi = glue->dm.xdpi;
    a->ydpi = glue->dm.ydpi;
    a->sw = glue->dm.w;
    a->sh = glue->dm.h;
    glue->inches_wide = glue->dm.w / glue->dm.xdpi;
    glue->inches_high = glue->dm.h / glue->dm.ydpi;
    traceln("DISPLAY REAL SIZE: %dx%d dpi: %d %.1fx%.1f density: %.1f scaled %.1f physical: %.3fx%.3f inches",
            glue->dm.w, glue->dm.h, glue->dm.dpi, glue->dm.xdpi, glue->dm.ydpi,
            glue->dm.density, glue->dm.scaled_density, glue->inches_wide, glue->inches_high);
}

static void set_window_flags(ANativeActivity* na) {
    static const int add    = AWINDOW_FLAG_KEEP_SCREEN_ON|
                              AWINDOW_FLAG_TURN_SCREEN_ON|
                              AWINDOW_FLAG_FULLSCREEN|
                              AWINDOW_FLAG_LAYOUT_IN_SCREEN|
                              AWINDOW_FLAG_LAYOUT_INSET_DECOR;
    static const int remove = AWINDOW_FLAG_SCALED|
                              AWINDOW_FLAG_DITHER|
                              AWINDOW_FLAG_LAYOUT_NO_LIMITS|
                              AWINDOW_FLAG_FORCE_NOT_FULLSCREEN;
    ANativeActivity_setWindowFlags(na, add, remove);
}

static void init_timer_thread(glue_t* glue) {
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
    pthread_cond_init(&glue->cond, &cond_attr);
    pthread_mutex_init(&glue->mutex, null);
    assert(glue->timer_thread == 0);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); // PTHREAD_CREATE_DETACHED
    int r = pthread_create(&glue->timer_thread, &attr, timer_thread, glue);
    assert(r == 0);
    pthread_attr_destroy(&attr);
}

static void init_looper(glue_t* glue, ANativeActivity* na) {
    int pipe_ends[2] = {};
    if (pipe(pipe_ends)) {
        traceln("pipe() failed: %s", strerror(errno));
        ANativeActivity_finish(na);
        abort();
    } else {
        glue->read_pipe  = pipe_ends[0];
        glue->write_pipe = pipe_ends[1];
        glue->running = false;
        glue->command_poll_source.id = LOOPER_ID_MAIN;
        glue->command_poll_source.glue = glue;
        glue->command_poll_source.process = process_command;
        glue->input_poll_source.id = LOOPER_ID_INPUT;
        glue->input_poll_source.glue = glue;
        glue->input_poll_source.process = process_input;
        glue->accel_poll_source.id = LOOPER_ID_ACCEL;
        glue->accel_poll_source.glue = glue;
        glue->accel_poll_source.process = process_accel;
        glue->looper = ALooper_forThread(); // ALooper_prepare(0);
        int r = ALooper_addFd(glue->looper, glue->read_pipe, LOOPER_ID_MAIN,
            ALOOPER_EVENT_INPUT, looper_callback, &glue->command_poll_source);
        assert(r == 1);
        if (r != 1) { traceln("ALooper_addFd() failed"); }
    }
}

static void init_accelerometer(glue_t* glue) {
    assert(glue->sensor_manager == null);
#if ANDROID_API >= 26
    glue->sensor_manager = ASensorManager_getInstanceForPackage("app-droid");
#else
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    glue->sensor_manager = ASensorManager_getInstance();
    #pragma GCC diagnostic warning "-Wdeprecated-declarations"
#endif
    assert(glue->accelerometer_sensor == null);
    glue->accelerometer_sensor = ASensorManager_getDefaultSensor(glue->sensor_manager,
        ASENSOR_TYPE_ACCELEROMETER);
    assert(glue->sensor_event_queue == null);
    glue->sensor_event_queue = ASensorManager_createEventQueue(glue->sensor_manager,
        glue->looper, LOOPER_ID_ACCEL, looper_callback, &glue->accel_poll_source);
#if ANDROID_API >= 26
    int us = ASensor_getMinDelay(glue->accelerometer_sensor); // microseconds
    ASensorEventQueue_registerSensor(glue->sensor_event_queue,
        glue->accelerometer_sensor, us, 0); // 0 means "streaming"
#endif
}

static void init_state(glue_t* glue, const void* data, int bytes) {
    memset(&glue->state, 0, sizeof(glue->state));
    if (data != null && bytes > 0) {
        // https://github.com/aosp-mirror/platform_frameworks_base/blob/d9b11b058c6a50fa25b75d6534a2deaf0e62d4b3/core/java/android/app/NativeActivity.java#L170
        assert(bytes <= sizeof(glue->state.data));
        app_state_t* s = (app_state_t*)data;
        memcpy(glue->state.data, s->data, min(bytes, sizeof(glue->state.data)));
    }
}

static void init_callbacks(ANativeActivityCallbacks* cb) {
    cb->onDestroy                  = on_destroy;
    cb->onStart                    = on_start;
    cb->onResume                   = on_resume;
    cb->onSaveInstanceState        = on_save_state;
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
}

static void create_activitiy(glue_t* glue, ANativeActivity* na, void* data, size_t bytes) {
    assert(glue->na == null);
    assert(glue->a = app);
    assert(app->glue == glue);
    assert(app->focused == null);
    na->instance = glue;
    glue->destroy_requested = false;
    glue->na = na;
    glue->start_time_in_ns = time_monotonic_ns();
    droid_jni_hide_navigation_bar(na);
    display_real_size(glue, na);
    init_state(glue, data, bytes);
    glue->config = AConfiguration_new();
    AConfiguration_fromAssetManager(glue->config, na->assetManager);
    process_configuration(glue);
    init_callbacks(na->callbacks);
    set_window_flags(na);
    init_looper(glue, na);
    init_accelerometer(glue);
    init_timer_thread(glue);
    assertion(app->init != null, "init() cannot be null");
    app->init(app);
}

bool app_dispatch_key(app_t* a, int flags, int keycode);
bool app_dispatch_touch(app_t* a, int index, int action, int x, int y);

const sys_t sys = {
    quit,
    exit_,
    app_dispatch_key,
    app_dispatch_touch,
    invalidate,
    focus,
    timer_add,
    timer_remove,
    asset_map,
    asset_unmap,
    vibrate,
    show_keyboard,
    logln
};

static void init_vtable(glue_t* glue) {
    app->glue = glue;
    glue->a = app;
}

app_t* app;

void ANativeActivity_onCreate(ANativeActivity* na, void* data, size_t bytes) {
    static glue_t glue;
    if (glue.a == null) { init_vtable(&glue); }
    traceln("pid/tid=%d/%d na=%p saved_state=%p[%d]", getpid(), gettid(), na, data, bytes);
    if (glue.na != null) {
        traceln("attempt to instantiate two activities - refused");
        ANativeActivity_finish(na);
    } else {
        create_activitiy(&glue, na, data, bytes);
    }
}

end_c
