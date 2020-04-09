#include "android_na.h"
#include "android_jni.h" // interface to Java only code (going via binder is too involving)
#include <android/log.h>
#include <android/window.h>
#include <android/asset_manager.h>

// slightly reworked NativeActivityGlue in C
// https://github.com/android/ndk-samples/blob/master/native-activity/app/src/main/cpp/main.cpp

BEGIN_C

static android_na_t na;

static void free_saved_state(android_na_t* a) {
    pthread_mutex_lock(&a->mutex);
    if (a->saved_state != null) {
        deallocate(a->saved_state);
        a->saved_state = null;
        a->saved_state_size = 0;
    }
    pthread_mutex_unlock(&a->mutex);
}

static int8_t dequeue_command(android_na_t* a) {
    int8_t command;
    if (read(a->read_pipe, &command, sizeof(command)) == sizeof(command)) {
        switch (command) {
            case ANDROID_NA_COMMAND_SAVE_STATE: free_saved_state(a); break;
            default: break;
        }
        return command;
    } else {
        traceln("No data in command pipe");
        return -1;
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

static void trace_current_configuration(android_na_t* a) {
    char lang[2], country[2];
    AConfiguration_getLanguage(a->config, lang);
    AConfiguration_getCountry(a->config, country);
    if (trace_config) {
        traceln("config mcc=%d mnc=%d lang=%c%c cnt=%c%c smallest=%ddp %dx%ddp "
                "orien=%d touch=%d dens=%d %s "
                "keys=%d nav=%d keysHid=%d navHid=%d sdk=%d size=%d %s long=%d "
                "modetype=%d %s modenight=%d",
                AConfiguration_getMcc(a->config),
                AConfiguration_getMnc(a->config),
                lang[0], lang[1], country[0], country[1],
                AConfiguration_getSmallestScreenWidthDp(a->config),
                AConfiguration_getScreenWidthDp(a->config),
                AConfiguration_getScreenHeightDp(a->config),
                AConfiguration_getOrientation(a->config),
                AConfiguration_getTouchscreen(a->config),
                AConfiguration_getDensity(a->config),
                density(AConfiguration_getDensity(a->config)),
                AConfiguration_getKeyboard(a->config),
                AConfiguration_getNavigation(a->config),
                AConfiguration_getKeysHidden(a->config),
                AConfiguration_getNavHidden(a->config),
                AConfiguration_getSdkVersion(a->config),
                AConfiguration_getScreenSize(a->config),
                screen_size(AConfiguration_getScreenSize(a->config)),
                AConfiguration_getScreenLong(a->config),
                AConfiguration_getUiModeType(a->config),
                mode_type(AConfiguration_getUiModeType(a->config)),
                AConfiguration_getUiModeNight(a->config));
    }
}

static struct { int command; const char* name; } commands[] = {
    {ANDROID_NA_COMMAND_INPUT_CHANGED,        "COMMAND_INPUT_CHANGED" },
    {ANDROID_NA_COMMAND_INIT_WINDOW,          "COMMAND_INIT_WINDOW" },
    {ANDROID_NA_COMMAND_TERM_WINDOW,          "COMMAND_TERM_WINDOW" },
    {ANDROID_NA_COMMAND_WINDOW_RESIZED,       "COMMAND_WINDOW_RESIZED" },
    {ANDROID_NA_COMMAND_WINDOW_REDRAW_NEEDED, "COMMAND_WINDOW_REDRAW_NEEDED" },
    {ANDROID_NA_COMMAND_CONTENT_RECT_CHANGED, "COMMAND_CONTENT_RECT_CHANGED" },
    {ANDROID_NA_COMMAND_GAINED_FOCUS,         "COMMAND_GAINED_FOCUS" },
    {ANDROID_NA_COMMAND_LOST_FOCUS,           "COMMAND_LOST_FOCUS" },
    {ANDROID_NA_COMMAND_CONFIG_CHANGED,       "COMMAND_CONFIG_CHANGED" },
    {ANDROID_NA_COMMAND_LOW_MEMORY,           "COMMAND_LOW_MEMORY" },
    {ANDROID_NA_COMMAND_CREATE,               "COMMAND_CREATE" },
    {ANDROID_NA_COMMAND_START,                "COMMAND_START" },
    {ANDROID_NA_COMMAND_RESUME,               "COMMAND_RESUME" },
    {ANDROID_NA_COMMAND_SAVE_STATE,           "COMMAND_SAVE_STATE" },
    {ANDROID_NA_COMMAND_PAUSE,                "COMMAND_PAUSE" },
    {ANDROID_NA_COMMAND_STOP,                 "COMMAND_STOP" },
    {ANDROID_NA_COMMAND_DESTROY,              "COMMAND_DESTROY" }
};

static const char* cmd2str(int command) {
    for (int i = 0; i < countof(commands); i++) {
        if (command == commands[i].command) { return commands[i].name; }
    }
    static char text[128];
    snprintf(text, countof(text), "unknown command %d", command);
    return text;
}

static void process_configuration(android_na_t* a) {
    trace_current_configuration(a);
    // I do not know a way to distuinguish between Androids with build in QWERTY keyboars
    // and external connected keyboards:
    a->keyboad_present = AConfiguration_getKeyboard(a->config) == ACONFIGURATION_KEYBOARD_QWERTY;
    // AConfiguration_getSmallestScreenWidthDp() is actually the real width in DP
    // the name is of the function is very confusing (prehaps Android G1 landscape legacy)...
    int32_t dp_w_min = AConfiguration_getSmallestScreenWidthDp(a->config);
    int dp_w = AConfiguration_getScreenWidthDp(a->config);
    int dp_h = max(AConfiguration_getScreenHeightDp(a->config), dp_w_min);
//  traceln("%dx%ddp SmallestScreenWidth %d", dp_w, dp_h, dp_w_min);
    a->inches_wide = dp_w / 120.0f; // legacy but still true up to Android Pixel 3a
    a->inches_high = dp_h / 120.0f;
    a->density = AConfiguration_getDensity(a->config); // obscure and not true DPI
//  traceln("keyboard=%d density=%d %.2fx%.2f inches", a->keyboad_present, AConfiguration_getDensity(a->config), a->inches_wide, a->inches_high);
}

// pre_exec_command() is called with the command returned by dequeue_command() to do the
// initial pre-processing of the given command.  You can perform your own
// actions for the command after calling this function.

void pre_exec_command(android_na_t* a, int8_t command) {
    switch (command) {
        case ANDROID_NA_COMMAND_CREATE:
            break;
        case ANDROID_NA_COMMAND_INPUT_CHANGED:
            pthread_mutex_lock(&a->mutex);
            if (a->input_queue != null) {
                AInputQueue_detachLooper(a->input_queue);
            }
            a->input_queue = a->pending_input_queue;
            if (a->input_queue != null) {
                AInputQueue_attachLooper(a->input_queue, a->looper, LOOPER_ID_INPUT, null, &a->input_poll_source);
            }
            pthread_cond_broadcast(&a->cond);
            pthread_mutex_unlock(&a->mutex);
            break;
        case ANDROID_NA_COMMAND_CONTENT_RECT_CHANGED:
            a->content_rect = a->pending_content_rect;
            break;
        case ANDROID_NA_COMMAND_INIT_WINDOW:
            pthread_mutex_lock(&a->mutex);
            a->window = a->pending_window;
            pthread_cond_broadcast(&a->cond);
            pthread_mutex_unlock(&a->mutex);
            break;
        case ANDROID_NA_COMMAND_TERM_WINDOW:
            pthread_cond_broadcast(&a->cond);
            break;
        case ANDROID_NA_COMMAND_RESUME:
        case ANDROID_NA_COMMAND_START:
        case ANDROID_NA_COMMAND_PAUSE:
        case ANDROID_NA_COMMAND_STOP:
            pthread_mutex_lock(&a->mutex);
            a->activity_state = command;
            pthread_cond_broadcast(&a->cond);
            pthread_mutex_unlock(&a->mutex);
            break;
        case ANDROID_NA_COMMAND_CONFIG_CHANGED:
            AConfiguration_fromAssetManager(a->config, a->activity->assetManager);
            process_configuration(a);
            break;
        case ANDROID_NA_COMMAND_DESTROY:
            a->destroy_requested = 1;
            break;
        case ANDROID_NA_COMMAND_WINDOW_RESIZED:
        case ANDROID_NA_COMMAND_WINDOW_REDRAW_NEEDED:
        case ANDROID_NA_COMMAND_GAINED_FOCUS:
        case ANDROID_NA_COMMAND_LOST_FOCUS:
            break;
        default:
            traceln("command=%d %s", command, cmd2str(command));
            (void)cmd2str;
            break;
    }
}

// post_exec_command() is called with the command returned by dequeue_command() to do the
// final post-processing of the given command.  You must have done your own
// actions for the command before calling this function.

static void post_exec_command(android_na_t* a, int8_t command) {
    switch (command) {
        case ANDROID_NA_COMMAND_TERM_WINDOW:
            pthread_mutex_lock(&a->mutex);
            a->window = null;
            pthread_cond_broadcast(&a->cond);
            pthread_mutex_unlock(&a->mutex);
            break;
        case ANDROID_NA_COMMAND_SAVE_STATE:
            pthread_mutex_lock(&a->mutex);
            a->state_saved = 1;
            pthread_cond_broadcast(&a->cond);
            pthread_mutex_unlock(&a->mutex);
            break;
        case ANDROID_NA_COMMAND_RESUME:
            free_saved_state(a);
            break;
        case ANDROID_NA_COMMAND_CREATE:
        case ANDROID_NA_COMMAND_DESTROY:
        case ANDROID_NA_COMMAND_START:
        case ANDROID_NA_COMMAND_PAUSE:
        case ANDROID_NA_COMMAND_STOP:
        case ANDROID_NA_COMMAND_INPUT_CHANGED:
        case ANDROID_NA_COMMAND_INIT_WINDOW:
        case ANDROID_NA_COMMAND_WINDOW_RESIZED:
        case ANDROID_NA_COMMAND_WINDOW_REDRAW_NEEDED:
        case ANDROID_NA_COMMAND_GAINED_FOCUS:
        case ANDROID_NA_COMMAND_LOST_FOCUS:
        case ANDROID_NA_COMMAND_CONTENT_RECT_CHANGED:
            break;
        default:
            traceln("command=%d %s", command, cmd2str(command));
            (void)cmd2str;
            break;
    }
}

static void destroy(android_na_t* a) {
    free_saved_state(a);
    pthread_mutex_lock(&a->mutex);
    if (a->input_queue != null) {
        AInputQueue_detachLooper(a->input_queue);
    }
    AConfiguration_delete(a->config);
    a->config = null;
    a->destroyed = 1;
    pthread_cond_broadcast(&a->cond);
    pthread_mutex_unlock(&a->mutex);
    // Can't touch a object after this.
}

static void process_input(android_na_t* app, android_poll_source_t* source) {
    AInputEvent* event = null;
    while (AInputQueue_getEvent(app->input_queue, &event) >= 0) {
//      traceln("new input event: type=%d", AInputEvent_getType(event));
        if (AInputQueue_preDispatchEvent(app->input_queue, event)) {
            continue;
        }
        int32_t handled = 0;
        if (app->on_input_event != null) { handled = app->on_input_event(app, event); }
        AInputQueue_finishEvent(app->input_queue, event, handled);
    }
}

static void process_command(android_na_t* app, android_poll_source_t* source) {
    int8_t command = dequeue_command(app);
    pre_exec_command(app, command);
    if (app->on_command != null) { app->on_command(app, command); }
    post_exec_command(app, command);
}

static void* thread_proc(void* param) {
//  traceln("pid/tid=%d/%d", getpid(), gettid());
    pthread_setname_np(pthread_self(), "main");
    android_na_t* a = (android_na_t*)param;
    android_jni_attach_current_thread_as_daemon(a->activity->env);
    a->config = AConfiguration_new();
    AConfiguration_fromAssetManager(a->config, a->activity->assetManager);
    process_configuration(a);
    a->command_poll_source.id = LOOPER_ID_MAIN;
    a->command_poll_source.app = a;
    a->command_poll_source.process = process_command;
    a->input_poll_source.id = LOOPER_ID_INPUT;
    a->input_poll_source.app = a;
    a->input_poll_source.process = process_input;
    ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ALooper_addFd(looper, a->read_pipe, LOOPER_ID_MAIN, ALOOPER_EVENT_INPUT, null, &a->command_poll_source);
    a->looper = looper;
    pthread_mutex_lock(&a->mutex);
    a->running = 1;
    pthread_cond_broadcast(&a->cond);
    pthread_mutex_unlock(&a->mutex);
    android_main(a);
    android_jni_dettach_current_thread(a->activity->env);
    if (a->destroy_requested) { destroy(a); }
    return null;
}

// Native activity interaction (called from main thread)

// Call when ALooper_pollAll() returns LOOPER_ID_MAIN, reading the next app command message.

static void enqueue_command(android_na_t* a, int8_t command) {
    if (write(a->write_pipe, &command, sizeof(command)) != sizeof(command)) {
        traceln("Failure writing a command: %s", strerror(errno));
    }
}

static android_na_t* create_activity(ANativeActivity* activity, void* saved_state, int saved_state_size) {
    android_na_t* a = &na;
    memset(a, 0, sizeof(android_na_t));
    a->activity = activity;
    pthread_mutex_init(&a->mutex, null);
    pthread_cond_init(&a->cond, null);
    if (saved_state != null) {
        a->saved_state = allocate(saved_state_size);
        a->saved_state_size = saved_state_size;
        memcpy(a->saved_state, saved_state, saved_state_size);
    }
    int pipe_ends[2];
    if (pipe(pipe_ends)) {
        traceln("could not create pipe: %s", strerror(errno));
        return null;
    }
    a->read_pipe  = pipe_ends[0];
    a->write_pipe = pipe_ends[1];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&a->thread, &attr, thread_proc, a);
    // Wait for thread to start.
    pthread_mutex_lock(&a->mutex);
    while (!a->running) {
        pthread_cond_wait(&a->cond, &a->mutex);
    }
    enqueue_command(a, ANDROID_NA_COMMAND_CREATE);
    pthread_mutex_unlock(&a->mutex);
    return a;
}

static void set_input(android_na_t* a, AInputQueue* input_queue) {
    pthread_mutex_lock(&a->mutex);
    a->pending_input_queue = input_queue;
    enqueue_command(a, ANDROID_NA_COMMAND_INPUT_CHANGED);
    while (a->input_queue != a->pending_input_queue) {
        pthread_cond_wait(&a->cond, &a->mutex);
    }
    pthread_mutex_unlock(&a->mutex);
}

static void set_window(android_na_t* a, ANativeWindow* window) {
    pthread_mutex_lock(&a->mutex);
    if (a->pending_window != null) {
        enqueue_command(a, ANDROID_NA_COMMAND_TERM_WINDOW);
    }
    a->pending_window = window;
    if (window != null) {
        enqueue_command(a, ANDROID_NA_COMMAND_INIT_WINDOW);
    }
    while (a->window != a->pending_window) {
        pthread_cond_wait(&a->cond, &a->mutex);
    }
    pthread_mutex_unlock(&a->mutex);
}

static void set_activity_state(android_na_t* a, int8_t command) {
    pthread_mutex_lock(&a->mutex);
    enqueue_command(a, command);
    while (a->activity_state != command) {
        pthread_cond_wait(&a->cond, &a->mutex);
    }
    pthread_mutex_unlock(&a->mutex);
}

static void dispose(android_na_t* a) {
    pthread_mutex_lock(&a->mutex);
    enqueue_command(a, ANDROID_NA_COMMAND_DESTROY);
    while (!a->destroyed) {
        pthread_cond_wait(&a->cond, &a->mutex);
    }
    pthread_mutex_unlock(&a->mutex);
    close(a->read_pipe);
    close(a->write_pipe);
    pthread_cond_destroy(&a->cond);
    pthread_mutex_destroy(&a->mutex);
}

static void onDestroy(ANativeActivity* activity) {
    android_na_t* a = (android_na_t*)activity->instance;
    int exit_requested = a->exit_requested;
    int exit_code = a->exit_code;
    dispose(a);
    if (exit_requested) {
//      traceln("exit_code=%d", exit_code);
        exit(exit_code);
    }
}

static void onStart(ANativeActivity* activity) {
    set_activity_state((android_na_t*)activity->instance, ANDROID_NA_COMMAND_START);
}

static void onResume(ANativeActivity* activity) {
    set_activity_state((android_na_t*)activity->instance, ANDROID_NA_COMMAND_RESUME);
}

static void* onSaveInstanceState(ANativeActivity* activity, size_t* outLen) {
    android_na_t* a = (android_na_t*)activity->instance;
    void* saved_state = null;
    pthread_mutex_lock(&a->mutex);
    a->state_saved = 0;
    enqueue_command(a, ANDROID_NA_COMMAND_SAVE_STATE);
    while (!a->state_saved) {
        pthread_cond_wait(&a->cond, &a->mutex);
    }
    if (a->saved_state != null) {
        saved_state = a->saved_state;
        *outLen = a->saved_state_size;
        a->saved_state = null;
        a->saved_state_size = 0;
    }
    pthread_mutex_unlock(&a->mutex);
    return saved_state;
}

static void onPause(ANativeActivity* activity) {
    set_activity_state((android_na_t*)activity->instance, ANDROID_NA_COMMAND_PAUSE);
}

static void onStop(ANativeActivity* activity) {
    set_activity_state((android_na_t*)activity->instance, ANDROID_NA_COMMAND_STOP);
}

static void onConfigurationChanged(ANativeActivity* activity) {
    android_na_t* a = (android_na_t*)activity->instance;
    enqueue_command(a, ANDROID_NA_COMMAND_CONFIG_CHANGED);
}

static void onLowMemory(ANativeActivity* activity) {
    android_na_t* a = (android_na_t*)activity->instance;
    enqueue_command(a, ANDROID_NA_COMMAND_LOW_MEMORY);
}

static void onWindowFocusChanged(ANativeActivity* activity, int focused) {
    enqueue_command((android_na_t*)activity->instance,
            focused ? ANDROID_NA_COMMAND_GAINED_FOCUS : ANDROID_NA_COMMAND_LOST_FOCUS);
    android_jni_hide_navigation_bar(activity);
    if (!android_jni_vibrate_with_effect(activity, "EFFECT_TICK")) {
        android_jni_vibrate_milliseconds(activity, 100);
        android_jni_vibrate_milliseconds(activity, 20);
        android_jni_vibrate_milliseconds(activity, 100);
    }
}

static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window) {
    set_window((android_na_t*)activity->instance, window);
}

static void onNativeWindowResized(ANativeActivity* activity, ANativeWindow* window) {
    enqueue_command((android_na_t*)activity->instance, ANDROID_NA_COMMAND_WINDOW_RESIZED);
}

static void onNativeWindowRedrawNeeded(ANativeActivity* activity, ANativeWindow* window) {
    enqueue_command((android_na_t*)activity->instance, ANDROID_NA_COMMAND_WINDOW_REDRAW_NEEDED);
}

static void onContentRectChanged(ANativeActivity* activity, const ARect* rect) {
    android_na_t* a = (android_na_t*)activity->instance;
    a->pending_content_rect = *rect;
    enqueue_command(a, ANDROID_NA_COMMAND_CONTENT_RECT_CHANGED);
}

static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window) {
    set_window((android_na_t*)activity->instance, null);
}

static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue) {
    set_input((android_na_t*)activity->instance, queue);
}

static void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue) {
    set_input((android_na_t*)activity->instance, null);
    android_jni_done(activity);
}

void ANativeActivity_onCreate(ANativeActivity* activity, void* saved_state, size_t saved_state_size) {
    android_jni_init(activity);
    activity->callbacks->onDestroy = onDestroy;
    activity->callbacks->onStart = onStart;
    activity->callbacks->onResume = onResume;
    activity->callbacks->onSaveInstanceState = onSaveInstanceState;
    activity->callbacks->onPause = onPause;
    activity->callbacks->onStop = onStop;
    activity->callbacks->onConfigurationChanged = onConfigurationChanged;
    activity->callbacks->onLowMemory = onLowMemory;
    activity->callbacks->onWindowFocusChanged = onWindowFocusChanged;
    activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    activity->callbacks->onNativeWindowResized = onNativeWindowResized;
    activity->callbacks->onNativeWindowRedrawNeeded = onNativeWindowRedrawNeeded;
    activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
    activity->callbacks->onInputQueueCreated = onInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
    activity->callbacks->onContentRectChanged = onContentRectChanged;
    activity->instance = create_activity(activity, saved_state, saved_state_size);
    static const int add = AWINDOW_FLAG_KEEP_SCREEN_ON|
                           AWINDOW_FLAG_TURN_SCREEN_ON|
                           AWINDOW_FLAG_FULLSCREEN|
                           AWINDOW_FLAG_LAYOUT_IN_SCREEN|
                           AWINDOW_FLAG_LAYOUT_INSET_DECOR;
    static const int remove = AWINDOW_FLAG_SCALED|
                              AWINDOW_FLAG_DITHER|
                              AWINDOW_FLAG_FORCE_NOT_FULLSCREEN|
                              AWINDOW_FLAG_LAYOUT_NO_LIMITS;
    ANativeActivity_setWindowFlags(activity, add, remove);
}

END_C
