#pragma once
#include "rt.h"
#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_activity.h>

BEGIN_C // android_na_ stands for Android Native Activity

/* The native activity interface provided by <android/native_activity.h>
 * is based on a set of application-provided callbacks that will be called
 * by the Activity's main thread when certain events occur.
 *
 * This means that each one of this callbacks _should_ _not_ block, or they
 * risk having the system force-close the application. This programming
 * model is direct, lightweight, but constraining.
 *
 * The 'threaded_native_app' static library is used to provide a different
 * execution model where the application can implement its own main event
 * loop in a different thread instead. Here's how it works:
 *
 * 1/ The application must provide a function named "android_main()" that
 *    will be called when the activity is created, in a new thread that is
 *    distinct from the activity's main thread.
 *
 * 2/ android_main() receives a pointer to a valid "android_app" structure
 *    that contains references to other important objects, e.g. the
 *    ANativeActivity object instance the application is running in.
 *
 * 3/ the "android_app" object holds an ALooper instance that already
 *    listens to two important things:
 *
 *      - activity lifecycle events (e.g. "pause", "resume"). See ANDROID_NA_COMMAND_XXX
 *        declarations below.
 *
 *      - input events coming from the AInputQueue attached to the activity.
 *
 *    Each of these correspond to an ALooper identifier returned by
 *    ALooper_pollOnce with values of LOOPER_ID_MAIN and LOOPER_ID_INPUT,
 *    respectively.
 *
 *    Your application can use the same ALooper to listen to additional
 *    file-descriptors.  They can either be callback based, or with return
 *    identifiers starting with LOOPER_ID_USER.
 *
 * 4/ Whenever you receive a LOOPER_ID_MAIN or LOOPER_ID_INPUT event,
 *    the returned data will point to an android_poll_source structure.  You
 *    can call the process() function on it, and fill in android_app->on_command
 *    and android_app->on_input_event to be called for your own processing
 *    of the event.
 *
 *    Alternatively, you can call the low-level functions to read and process
 *    the data directly...  look at the process_command() and process_input()
 *    implementations in the glue to see how to do this.
 *
 * See the sample named "native-activity" that comes with the NDK with a
 * full usage example.  Also look at the JavaDoc of NativeActivity.
 */

typedef struct android_na_s android_na_t;

// Data associated with an ALooper fd that will be returned as the "outData"
// when that source has data ready.

typedef struct android_poll_source_s android_poll_source_t;

typedef struct android_poll_source_s {
    int32_t id; // The identifier of this source.  May be LOOPER_ID_MAIN or LOOPER_ID_INPUT.
    android_na_t* app; // The android_app this ident is associated with.
    // Function to call to perform the standard processing of data from this source.
    void (*process)(android_na_t* app, android_poll_source_t* source);
} android_poll_source_t;

// This is the interface for the standard glue code of a threaded
// application.  In this model, the application's code is running
// in its own thread separate from the main thread of the process.
// It is not required that this thread be associated with the Java
// VM, although it will need to be in order to make JNI calls any
// Java objects.

typedef struct android_na_s {
    void* that; // The application can place a pointer to its own state object.
    // Fill this in with the function to process main app commands (ANDROID_NA_COMMAND_*)
    void (*on_command)(android_na_t* app, int32_t command);
    // Fill this in with the function to process input events.  At this point
    // the event has already been pre-dispatched, and it will be finished upon
    // return.  Return 1 if you have handled the event, 0 for any default
    // dispatching.
    int32_t (*on_input_event)(android_na_t* app, AInputEvent* event);
    // The ANativeActivity object instance that this app is running in.
    ANativeActivity* activity;
    // The current configuration the app is running in.
    AConfiguration* config;
    float inches_wide; // best guess for screen physical size
    float inches_high;
    float density; // do not use - Android dpi `bining` dpi
    int keyboad_present; // is keyboard connected?
    // This is the last instance's saved state, as provided at creation time.
    // It is NULL if there was no state.  You can use this as you need; the
    // memory will remain around until you call android_na_exec_command() for
    // ANDROID_NA_COMMAND_RESUME, at which point it will be freed and saved_state set to NULL.
    // These variables should only be changed when processing a ANDROID_NA_COMMAND_SAVE_STATE,
    // at which point they will be initialized to NULL and you can malloc your
    // state and place the information here.  In that case the memory will be
    // freed for you later.
    void* saved_state;
    int saved_state_size;
    // The ALooper associated with the app's thread.
    ALooper* looper;
    // When non-NULL, this is the input queue from which the app will
    // receive user input events.
    AInputQueue* input_queue;
    // When non-NULL, this is the window surface that the app can draw in.
    ANativeWindow* window;
    // Current content rectangle of the window; this is the area where the
    // window's content should be placed to be seen by the user.
    ARect content_rect;
    // Current state of the app's activity.  May be either ANDROID_NA_COMMAND_START,
    // ANDROID_NA_COMMAND_RESUME, ANDROID_NA_COMMAND_PAUSE, or ANDROID_NA_COMMAND_STOP; see below.
    int activity_state;
    // This is non-zero when the application's NativeActivity is being
    // destroyed and waiting for the app thread to complete.
    int destroy_requested;
    // This is non-zero when the application's activity is destroyed process should exit.
    int exit_requested;
    int exit_code; // status to exit() with
    // Below are "private" implementation of the glue code.
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int read_pipe;
    int write_pipe;
    pthread_t thread;
    android_poll_source_t command_poll_source;
    android_poll_source_t input_poll_source;
    int running;
    int state_saved;
    int destroyed;
    int redraw_needed;
    AInputQueue* pending_input_queue;
    ANativeWindow* pending_window;
    ARect pending_content_rect;
} android_na_t;

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
    // android_poll_source structure.  These can be read via the input_queue
    // object of android_app.
    LOOPER_ID_INPUT = 2,
    // Start of user-defined ALooper identifiers.
    LOOPER_ID_USER  = 3,
};

enum { // All commands are from main thread
    // The AInputQueue has changed.  Upon processing
    // this command, android_app->input_queue will be updated to the new queue (or NULL).
    ANDROID_NA_COMMAND_INPUT_CHANGED        =  0,
    // a new ANativeWindow is ready for use.  Upon
    // receiving this command, android_app->window will contain the new window surface.
    ANDROID_NA_COMMAND_INIT_WINDOW          =  1,
    // The existing ANativeWindow needs to be
    // terminated.  Upon receiving this command, android_app->window still
    // contains the existing window; after calling android_na_exec_cmd it will be set to NULL.
    ANDROID_NA_COMMAND_TERM_WINDOW          =  2,
    ANDROID_NA_COMMAND_WINDOW_RESIZED       =  3, // The current ANativeWindow has been resized. Please redraw with its new size.
    // The system needs that the current ANativeWindow
    // be redrawn.  You should redraw the window before handing this to
    // android_na_exec_cmd() in order to avoid transient drawing glitches.
    ANDROID_NA_COMMAND_WINDOW_REDRAW_NEEDED =  4,
    // The content area of the window has changed,
    // such as from the soft input window being shown or hidden.  You can
    // find the new content rect in android_app::content_rect.
    ANDROID_NA_COMMAND_CONTENT_RECT_CHANGED =  5,
    ANDROID_NA_COMMAND_GAINED_FOCUS         =  6, // window has gained input focus.
    ANDROID_NA_COMMAND_LOST_FOCUS           =  7, // window has lost input focus.
    ANDROID_NA_COMMAND_CONFIG_CHANGED       =  8, // The current device configuration has changed.
    ANDROID_NA_COMMAND_LOW_MEMORY           =  9, // The system is running low on memory. Try to reduce your memory use.
    ANDROID_NA_COMMAND_CREATE               = 10, // activity is being created
    ANDROID_NA_COMMAND_START                = 11, // activity has been started.
    ANDROID_NA_COMMAND_RESUME               = 12, // activity has been resumed.
    // The app should generate a new saved state
    // for itself, to restore from later if needed.  If you have saved state,
    // allocate it with malloc and place it in android_app.saved_state with
    // the size in android_app.saved_state_size.  The will be freed for you later.
    ANDROID_NA_COMMAND_SAVE_STATE           = 13,
    ANDROID_NA_COMMAND_PAUSE                = 14, // activity has been paused.
    ANDROID_NA_COMMAND_STOP                 = 15, // activity has been stopped.
    ANDROID_NA_COMMAND_DESTROY              = 16  // activity is being destroyed, and waiting for the app thread to clean up and exit before proceeding.
};

// This is the function that application code must implement, representing the main entry to the app.
extern void android_main(android_na_t* app);

END_C
