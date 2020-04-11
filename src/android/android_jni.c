#include "android_jni.h"
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

jobject get_system_service(ANativeActivity* na, const char* label) {
    JNIEnv* env = na->env;
    jclass    context_class = (*env)->FindClass(env, "android/content/Context");
    jfieldID  service_name_fid = (*env)->GetStaticFieldID(env, context_class, label, "Ljava/lang/String;");
    jstring   service_name_string = (*env)->GetStaticObjectField(env, context_class, service_name_fid);
    jmethodID getSystemService_mid = (*env)->GetMethodID(env, context_class, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject   service = (*env)->CallObjectMethod(env, activity_this(na), getSystemService_mid, service_name_string);
    return check_and_clear_exception(env) ? null : service;
}

bool android_jni_hide_navigation_bar(ANativeActivity* na) {
    JNIEnv* env = na->env;
    jclass    activity_class   = (*env)->FindClass(env, "android/app/NativeActivity");
    jmethodID getWindow_mid    = (*env)->GetMethodID(env, activity_class, "getWindow", "()Landroid/view/Window;");
    jclass    window_class     = (*env)->FindClass(env, "android/view/Window");
    jmethodID getDecorView_mid = (*env)->GetMethodID(env, window_class, "getDecorView", "()Landroid/view/View;");
    jclass    viewClass = (*env)->FindClass(env, "android/view/View");
    jmethodID setSystemUiVisibility = (*env)->GetMethodID(env, viewClass, "setSystemUiVisibility", "(I)V");
    jobject   window = (*env)->CallObjectMethod(env, activity_this(na), getWindow_mid);
    jobject   decorView = (*env)->CallObjectMethod(env, window, getDecorView_mid);
    jfieldID  flagFullscreen_fid      = (*env)->GetStaticFieldID(env, viewClass, "SYSTEM_UI_FLAG_FULLSCREEN", "I");
    jfieldID  flagHideNavigation_fid  = (*env)->GetStaticFieldID(env, viewClass, "SYSTEM_UI_FLAG_HIDE_NAVIGATION", "I");
    jfieldID  flagImmersiveSticky_fid = (*env)->GetStaticFieldID(env, viewClass, "SYSTEM_UI_FLAG_IMMERSIVE_STICKY", "I");
    const int flagFullscreen = (*env)->GetStaticIntField(env, viewClass, flagFullscreen_fid);
    const int flagHideNavigation = (*env)->GetStaticIntField(env, viewClass, flagHideNavigation_fid);
    const int flagImmersiveSticky = (*env)->GetStaticIntField(env, viewClass, flagImmersiveSticky_fid);
    const int flag = flagFullscreen | flagHideNavigation | flagImmersiveSticky;
    (*env)->CallVoidMethod(env, decorView, setSystemUiVisibility, flag);
    return !check_and_clear_exception(env);
}

bool android_jni_vibrate_with_effect(ANativeActivity* na, const char* effect) { // API 26+
    JNIEnv* env = na->env;
    jobject vibrator_service = get_system_service(na, "VIBRATOR_SERVICE");
    bool supported = vibrator_service != null;
    jclass vibrator_service_class = supported ? (*env)->GetObjectClass(env, vibrator_service) : null;
    supported = supported && vibrator_service_class != null;
    jclass vibration_effect_class = supported ? (*env)->FindClass(env, "android/os/VibrationEffect") : null;
    supported = supported && vibration_effect_class != null;
    jfieldID effect_fid = supported ? (*env)->GetStaticFieldID(env, vibration_effect_class, effect, "I") : null;
    supported = supported && effect_fid != null;
    const int effect_int = supported ? (*env)->GetStaticIntField(env, vibration_effect_class, effect_fid) : 0;
    jmethodID vibrate_mid = supported ? (*env)->GetMethodID(env, vibrator_service_class, "vibrate", "(Landroid/os/VibrationEffect)V") : null;
    supported = supported && vibrate_mid != null;
    if (supported) {
        (*env)->CallVoidMethod(env, vibrator_service, vibrate_mid, effect_int);
    }
    return !check_and_clear_exception(env) && supported;
}

bool android_jni_vibrate_milliseconds(ANativeActivity* na, int milliseconds) {
    JNIEnv* env = na->env;
    jobject vibrator_service = get_system_service(na, "VIBRATOR_SERVICE");
    bool supported = vibrator_service != null;
    jclass vibrator_service_class = supported ? (*env)->GetObjectClass(env, vibrator_service) : null;
    supported = supported && vibrator_service_class != null;
    jmethodID vibrate_mid = supported ? (*env)->GetMethodID(env, vibrator_service_class, "vibrate", "(J)V") : null;
    supported = supported && vibrate_mid != null;
    jlong ms = milliseconds;
    if (supported) {
        (*env)->CallVoidMethod(env, vibrator_service, vibrate_mid, ms);
    }
    return !check_and_clear_exception(env) && supported;
}

END_C
