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
#define callBooleanMethodV(obj, mid, vl)    (!supported ? false : (*env)->CallBooleanMethodV(env, obj, mid, vl))
#define callObjectMethodV(obj, mid, vl) (!supported ? null : (*env)->CallObjectMethodV(env, obj, mid, vl))
#define callVoidMethodV(obj, mid, vl) do { if (supported) { (*env)->CallVoidMethodV(env, obj, mid, vl); } } while (0)

#define not_null(p) (!check_and_clear_exception(env) && (p) != null)
#define check(p) do { supported = not_null(p); } while (0)

jobject new_object(JNIEnv* env, const char* class_name) {
    bool supported = true;
    jobject cls = findClass(class_name); check(cls);
    jmethodID ctor_mid = getMethod(cls, "<init>", "()V"); check(ctor_mid);
    jobject object = newObject(cls, ctor_mid); check(object);
    return supported ? object : null;
}

static jint get_static_int_(JNIEnv* env, jclass cls, const char* label, bool* ok) {
    bool supported = true;
    jfieldID fid = getStaticFieldId(cls, label, "I"); check(fid);
    jint value = getStaticIntField(cls, fid);
    *ok = supported;
    return value;
}

#define get_static_int(cls, label) (!supported ? 0 : get_static_int_(env, cls, label, &supported))

static jint get_int_field_(JNIEnv* env, jobject obj, const char* label, bool* ok) {
    bool supported = true;
    jclass cls = getObjectClass(obj); check(cls);
    jfieldID fid = getFieldId(cls, label, "I"); check(fid);
    jint value = getIntField(obj, fid);
    *ok = supported;
    return value;
}

#define get_int_field(cls, label) (!supported ? 0 : get_int_field_(env, cls, label, &supported))

static jfloat get_float_field_(JNIEnv* env, jobject obj, const char* label, bool* ok) {
    bool supported = true;
    jclass cls = getObjectClass(obj); check(cls);
    jfieldID fid = getFieldId(cls, label, "F"); check(fid);
    jfloat value = getFloatField(obj, fid);
    *ok = supported;
    return value;
}

#define get_float_field(cls, label) (!supported ? 0 : get_float_field_(env, cls, label, &supported))

static bool call_void_method_(JNIEnv* env, jobject obj, const char* label, const char* signature, ...) {
    bool supported = true;
    jclass cls = getObjectClass(obj); check(cls);
    jmethodID mid = getMethod(cls, label, signature); check(mid);
    va_list vl;
    va_start(vl, signature);
    callVoidMethodV(obj, mid, vl);
    va_end(vl);
    return !check_and_clear_exception(env) && supported;
}

#define call_void_method(obj, label, signature, ...) \
    supported = supported && call_void_method_(env, obj, label, signature, ##__VA_ARGS__)

static jobject call_obj_method_(JNIEnv* env, jobject obj, const char* label, const char* signature,
                                bool *ok, ...) {
    bool supported = true;
    jclass cls = getObjectClass(obj); check(cls);
    jmethodID mid = getMethod(cls, label, signature); check(mid);
    va_list vl;
    va_start(vl, ok);
    jobject r = callObjectMethodV(obj, mid, vl);
    va_end(vl);
    *ok = !check_and_clear_exception(env) && supported;
    return r;
}

#define call_obj_method(obj, label, signature, ...) \
    (!supported ? null : call_obj_method_(env, obj, label, signature, &supported, ##__VA_ARGS__))

static jboolean call_bool_method_(JNIEnv* env, jobject obj, const char* label, const char* signature,
                                  bool *ok, ...) {
    bool supported = true;
    jclass cls = getObjectClass(obj); check(cls);
    jmethodID mid = getMethod(cls, label, signature); check(mid);
    va_list vl;
    va_start(vl, ok);
    jboolean r = callBooleanMethodV(obj, mid, vl);
    va_end(vl);
    *ok = !check_and_clear_exception(env) && supported;
    return r;
}

#define call_bool_method(obj, label, signature, ...) \
    (!supported ? false : call_bool_method_(env, obj, label, signature, &supported, ##__VA_ARGS__))

static jobject get_system_service(ANativeActivity* na, const char* label) {
    JNIEnv* env = na->env;
    bool supported = true;
    jclass context_class = findClass("android/content/Context"); check(context_class);
    jfieldID service_name_fid = getStaticFieldId(context_class, label, "Ljava/lang/String;");
    check(service_name_fid);
    jstring service_name_string = getStaticObjectField(context_class, service_name_fid);
    check(service_name_string);
    jobject service = call_obj_method(activity_this(na), "getSystemService",
                        "(Ljava/lang/String;)Ljava/lang/Object;", service_name_string);
    check(service);
    return !supported ? null : service;
}

bool droid_jni_hide_navigation_bar(ANativeActivity* na) {
    JNIEnv* env = na->env;
    bool supported = true;
    jobject window = call_obj_method(activity_this(na), "getWindow", "()Landroid/view/Window;");
    check(window);
    jobject decor_view = call_obj_method(window, "getDecorView", "()Landroid/view/View;");
    check(decor_view);
    jclass cls = findClass("android/view/View"); check(cls);
    const int fullscreen       = get_static_int(cls, "SYSTEM_UI_FLAG_FULLSCREEN");
    const int hide_navigation  = get_static_int(cls, "SYSTEM_UI_FLAG_HIDE_NAVIGATION");
    const int immersive        = get_static_int(cls, "SYSTEM_UI_FLAG_IMMERSIVE");
    const int immersive_sticky = get_static_int(cls, "SYSTEM_UI_FLAG_IMMERSIVE_STICKY");
    const int layout_stable    = get_static_int(cls, "SYSTEM_UI_FLAG_LAYOUT_STABLE");
    const int layout_fulscreen = get_static_int(cls, "SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN");
    const int layout_hide_navigation = get_static_int(cls, "SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION");
    const int flag = fullscreen | hide_navigation | immersive | immersive_sticky |
                     layout_fulscreen | layout_hide_navigation| layout_stable;
    call_void_method(decor_view, "setSystemUiVisibility", "(I)V", flag);
    return supported;
}

bool droid_jni_vibrate_with_effect(ANativeActivity* na, const char* effect) { // API 26+
    JNIEnv* env = na->env;
    jobject vibrator_service = get_system_service(na, "VIBRATOR_SERVICE");
    bool supported = vibrator_service != null;
    jclass vibration_effect_class = findClass("android/os/VibrationEffect");
    check(vibration_effect_class);
    const int effect_int = get_static_int(vibration_effect_class, effect);
    supported = !check_and_clear_exception(env) && supported;
    call_void_method(vibrator_service, "vibrate", "(Landroid/os/VibrationEffect)V", effect_int);
    return supported;
}

bool droid_jni_vibrate_milliseconds(ANativeActivity* na, int milliseconds) {
    JNIEnv* env = na->env;
    jobject vibrator_service = get_system_service(na, "VIBRATOR_SERVICE");
    bool supported = vibrator_service != null;
    jlong ms = milliseconds;
    call_void_method(vibrator_service, "vibrate", "(J)V", ms);
    return supported;
}

bool droid_jni_show_keyboard(ANativeActivity* na, bool on, int flags) {
    // see: https://github.com/aosp-mirror/platform_frameworks_base/blob/master/core/java/android/app/NativeActivity.java
    JNIEnv* env = na->env;
    jobject activity = activity_this(na);
    bool supported = true;
    jobject ims = get_system_service(na, "INPUT_METHOD_SERVICE"); check(ims);
    jobject window = call_obj_method(activity, "getWindow", "()Landroid/view/Window;");
    check(window);
    jobject decor_view = call_obj_method(window, "getDecorView", "()Landroid/view/View;");
    check(decor_view);
    bool done = false;
    if (supported) {
        if (on) {
            done = call_bool_method(ims, "showSoftInput", "(Landroid/view/View;I)Z", decor_view, 0);
        } else {
            jobject binder = call_obj_method(decor_view, "getWindowToken", "()Landroid/os/IBinder;");
            check(binder);
            done = call_bool_method(ims, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z", binder, 0);
        }
        if (!done) { traceln("done=%d happens on back button", done); }
    }
    return !check_and_clear_exception(env) && supported;
}

void droid_jni_get_display_real_size(ANativeActivity* na, droid_display_metrics_t* m) {
    JNIEnv* env = na->env;
    bool supported = true;
    jobject ws = get_system_service(na, "WINDOW_SERVICE"); check(ws);
    jclass ws_class = getObjectClass(ws); check(ws_class);
    jobject display_metrics = new_object(env, "android/util/DisplayMetrics");
    check(display_metrics);
    jobject display = call_obj_method(ws, "getDefaultDisplay", "()Landroid/view/Display;");
    check(display);
    jobject dm = new_object(env, "android/util/DisplayMetrics"); check(dm);
    call_void_method(display, "getRealMetrics", "(Landroid/util/DisplayMetrics;)V", dm);
    if (supported) {
        jclass dm_class = getObjectClass(dm); check(dm_class);
        m->w = get_int_field(dm, "widthPixels");
        m->h = get_int_field(dm, "heightPixels");
        m->xdpi = get_float_field(dm, "xdpi");
        m->ydpi = get_float_field(dm, "ydpi");
        m->dpi = get_int_field(dm, "densityDpi");
        m->density = get_float_field(dm, "density");
        m->scaled_density = get_float_field(dm, "scaledDensity");
    } else {
        memset(m, 0, sizeof(*m));
    }
}

END_C
