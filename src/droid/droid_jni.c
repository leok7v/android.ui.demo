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
#include "droid_jni.h"
#include <jni.h>

BEGIN_C

// ANativeActivity.clazz is NativeActivity object handle.
// IMPORTANT NOTE: This member is mis-named. It should really be named 'activity' instead of 'clazz',
// since it's a reference to the NativeActivity instance created by the system for you.
// We unfortunately cannot change this without breaking NDK source-compatibility.

static jobject activity_this(ANativeActivity* na) {
    jobject activity = na->clazz; // which is misnamed in NDK code with the note above
    return activity;              // exquisite attention to details was always paramount in Android code...
}

static bool check_and_clear_exception(JNIEnv* env) {
    bool exception_occured = (*env)->ExceptionCheck(env);
    if (exception_occured) { (*env)->ExceptionClear(env); }
    return exception_occured;
}

#define getMethod(...)            (!supported ? null : (*env)->GetMethodID(env, __VA_ARGS__))
#define getStaticMethod(...)      (!supported ? null : (*env)->GetStaticMethodID(env, __VA_ARGS__))
#define findClass(...)            (!supported ? null : (*env)->FindClass(env, __VA_ARGS__))
#define newObject(...)            (!supported ? null : (*env)->NewObject(env, __VA_ARGS__))
#define getObjectClass(...)       (!supported ? null : (*env)->GetObjectClass(env, __VA_ARGS__))
#define getFieldId(...)           (!supported ? null : (*env)->GetFieldID(env, __VA_ARGS__))
#define getStaticFieldId(...)     (!supported ? null : (*env)->GetStaticFieldID(env, __VA_ARGS__))
#define getStaticObjectField(...) (!supported ? null : (*env)->GetStaticObjectField(env, __VA_ARGS__))
#define getIntField(...)          (!supported ? 0    : (*env)->GetIntField(env, __VA_ARGS__))
#define getFloatField(...)        (!supported ? 0    : (*env)->GetFloatField(env, __VA_ARGS__))
#define getStaticIntField(...)    (!supported ? 0    : (*env)->GetStaticIntField(env, __VA_ARGS__))
#define callObjectMethod(...)     (!supported ? null : (*env)->CallObjectMethod(env, __VA_ARGS__))
#define callBooleanMethod(...)    (!supported ? false : (*env)->CallBooleanMethod(env, __VA_ARGS__))
#define callVoidMethod(...) do { if (supported) { (*env)->CallVoidMethod(env, __VA_ARGS__); } } while (0)

#define not_null(p) (!check_and_clear_exception(env) && (p) != null)
#define check(p) do { supported = not_null(p); } while (0)

static jobject get_system_service(ANativeActivity* na, const char* label) {
    JNIEnv* env = na->env;
    bool supported = true;
    jclass context_class = findClass("android/content/Context");
    check(context_class);
    jfieldID service_name_fid = getStaticFieldId(context_class, label, "Ljava/lang/String;");
    check(service_name_fid);
    jstring service_name_string = getStaticObjectField(context_class, service_name_fid);
    check(service_name_string);
    jmethodID get_system_service_mid = getMethod(context_class, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    check(get_system_service_mid);
    jobject service = callObjectMethod(activity_this(na), get_system_service_mid, service_name_string);
    check(service);
    return !supported ? null : service;
}

bool droid_jni_hide_navigation_bar(ANativeActivity* na) {
    JNIEnv* env = na->env;
    bool supported = true;
    jclass    activity_class   = findClass("android/app/NativeActivity");
    jmethodID getWindow_mid    = getMethod(activity_class, "getWindow", "()Landroid/view/Window;");
    jclass    window_class     = findClass("android/view/Window");
    jmethodID getDecorView_mid = getMethod(window_class, "getDecorView", "()Landroid/view/View;");
    jclass    viewClass        = findClass("android/view/View");
    jmethodID set_system_ui_visibility = getMethod(viewClass, "setSystemUiVisibility", "(I)V");
    jobject   window           = callObjectMethod(activity_this(na), getWindow_mid);
    jobject   decorView        = callObjectMethod(window, getDecorView_mid);
    jfieldID  flagFullscreen_fid      = getStaticFieldId(viewClass, "SYSTEM_UI_FLAG_FULLSCREEN", "I");
    jfieldID  flagHideNavigation_fid  = getStaticFieldId(viewClass, "SYSTEM_UI_FLAG_HIDE_NAVIGATION", "I");
    jfieldID  flagImmersiveSticky_fid = getStaticFieldId(viewClass, "SYSTEM_UI_FLAG_IMMERSIVE_STICKY", "I");
    const int flagFullscreen      = getStaticIntField(viewClass, flagFullscreen_fid);
    const int flagHideNavigation  = getStaticIntField(viewClass, flagHideNavigation_fid);
    const int flagImmersiveSticky = getStaticIntField(viewClass, flagImmersiveSticky_fid);
    const int flag = flagFullscreen | flagHideNavigation | flagImmersiveSticky;
    callVoidMethod(decorView, set_system_ui_visibility, flag);
    return !check_and_clear_exception(env);
}

bool droid_jni_vibrate_with_effect(ANativeActivity* na, const char* effect) { // API 26+
    JNIEnv* env = na->env;
    jobject vibrator_service = get_system_service(na, "VIBRATOR_SERVICE");
    bool supported = vibrator_service != null;
    jclass vibrator_service_class = getObjectClass(vibrator_service);
    check(vibrator_service_class);
    jclass vibration_effect_class = findClass("android/os/VibrationEffect");
    check(vibration_effect_class);
    jfieldID effect_fid = getStaticFieldId(vibration_effect_class, effect, "I");
    check(effect_fid);
    const int effect_int = getStaticIntField(vibration_effect_class, effect_fid);
    supported = !check_and_clear_exception(env) && supported;
    jmethodID vibrate_mid = getMethod(vibrator_service_class, "vibrate", "(Landroid/os/VibrationEffect)V");
    check(vibrate_mid);
    callVoidMethod(vibrator_service, vibrate_mid, effect_int);
    return !check_and_clear_exception(env) && supported;
}

bool droid_jni_vibrate_milliseconds(ANativeActivity* na, int milliseconds) {
    JNIEnv* env = na->env;
    jobject vibrator_service = get_system_service(na, "VIBRATOR_SERVICE");
    bool supported = vibrator_service != null;
    jclass vibrator_service_class = getObjectClass(vibrator_service);
    check(vibrator_service_class);
    jmethodID vibrate_mid = getMethod(vibrator_service_class, "vibrate", "(J)V");
    check(vibrate_mid);
    jlong ms = milliseconds;
    callVoidMethod(vibrator_service, vibrate_mid, ms);
    return !check_and_clear_exception(env) && supported;
}

bool droid_jni_show_keyboard(ANativeActivity* na, bool on, int flags) {
    // see: https://github.com/aosp-mirror/platform_frameworks_base/blob/master/core/java/android/app/NativeActivity.java
    JNIEnv* env = na->env;
    jobject activity = activity_this(na);
    bool supported = true;
    jclass nac = getObjectClass(activity);
    jobject ims = get_system_service(na, "INPUT_METHOD_SERVICE");
    check(ims);
    jclass ims_class = getObjectClass(ims);
    check(ims_class);
    jmethodID show_soft_input_mid = getMethod(ims_class, "showSoftInput", "(Landroid/view/View;I)Z");
    check(show_soft_input_mid);
    jmethodID hide_soft_input_mid = getMethod(ims_class, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z");
    check(hide_soft_input_mid);
    jmethodID get_window_mid = getMethod(nac, "getWindow", "()Landroid/view/Window;");
    check(get_window_mid);
    jclass window_class = findClass("android/view/Window");
    check(window_class);
    jmethodID get_decor_view_mid = getMethod(window_class, "getDecorView", "()Landroid/view/View;");
    check(get_decor_view_mid);
    jobject window = callObjectMethod(activity, get_window_mid);
    check(window);
    jobject decor_view = callObjectMethod(window, get_decor_view_mid);
    check(decor_view);
    bool done = false;
    if (supported) {
        if (on) {
            done = callBooleanMethod(ims, show_soft_input_mid, decor_view, 0);
        } else {
            jclass view_class = findClass("android/view/View");
            check(view_class);
            jmethodID get_window_token_mid = getMethod(view_class, "getWindowToken", "()Landroid/os/IBinder;");
            check(get_window_token_mid);
            jobject binder = callObjectMethod(decor_view, get_window_token_mid);
            check(binder);
            done = callBooleanMethod(ims, hide_soft_input_mid, binder, 0);
        }
        if (!done) { traceln("done=%d happens on back button", done); }
    }
    return !check_and_clear_exception(env) && supported;
}

jobject new_object(JNIEnv* env, const char* class_name) {
    bool supported = true;
    jobject cls = findClass(class_name);
    check(cls);
    jmethodID ctor_mid = getMethod(cls, "<init>", "()V");
    check(ctor_mid);
    jobject object = newObject(cls, ctor_mid);
    check(object);
    return supported ? object : null;
}

void droid_jni_get_display_real_size(ANativeActivity* na, droid_display_metrics_t* m) {
    JNIEnv* env = na->env;
    bool supported = true;
    jobject ws = get_system_service(na, "WINDOW_SERVICE");
    check(ws);
    jclass ws_class = getObjectClass(ws);
    check(ws_class);
    jobject display_metrics = new_object(env, "android/util/DisplayMetrics");
    check(display_metrics);
    jmethodID get_default_display_mid = getMethod(ws_class, "getDefaultDisplay", "()Landroid/view/Display;");
    check(get_default_display_mid);
    jobject display = callObjectMethod(ws, get_default_display_mid);
    check(display);
    jobject display_class = getObjectClass(display);
    check(display_class);
    jmethodID get_real_metrics_mid = getMethod(display_class, "getRealMetrics", "(Landroid/util/DisplayMetrics;)V");
    check(get_real_metrics_mid);
    jobject dm = new_object(env, "android/util/DisplayMetrics");
    check(dm);
    callVoidMethod(display, get_real_metrics_mid, dm);
    if (supported) {
        jclass dm_class = getObjectClass(dm); check(dm_class);
        jfieldID xdpi_fid = getFieldId(dm_class, "xdpi", "F"); check(xdpi_fid);
        jfieldID ydpi_fid = getFieldId(dm_class, "ydpi", "F"); check(ydpi_fid);
        jfieldID width_pixels_fid   = getFieldId(dm_class, "widthPixels", "I");   check(width_pixels_fid);
        jfieldID height_pixels_fid  = getFieldId(dm_class, "heightPixels", "I");  check(height_pixels_fid);
        jfieldID density_fid        = getFieldId(dm_class, "density", "F");       check(density_fid);
        jfieldID density_dpi_fid    = getFieldId(dm_class, "densityDpi", "I");    check(density_dpi_fid);
        jfieldID scaled_density_fid = getFieldId(dm_class, "scaledDensity", "F"); check(scaled_density_fid);
        m->w       = getIntField(dm, width_pixels_fid);
        m->h       = getIntField(dm, height_pixels_fid);
        m->xdpi    = getFloatField(dm, xdpi_fid);
        m->ydpi    = getFloatField(dm, ydpi_fid);
        m->dpi     = getIntField(dm, density_dpi_fid);
        m->density = getFloatField(dm, density_fid);
        m->scaled_density = getFloatField(dm, scaled_density_fid);
    } else {
        memset(m, 0, sizeof(*m));
    }
}

END_C
