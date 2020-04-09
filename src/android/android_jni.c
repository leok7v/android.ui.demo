#include "android_jni.h"
#include <jni.h>

BEGIN_C

static JavaVM* jvm;
static jobject vibrator_service;
static jclass  vibrator_service_class;

// IMPORTANT:
// http://web.archive.org/web/20130331010259/http://blog.tewdew.com/post/6852907694/using-jni-from-a-native-activity

static bool check_and_clear_exception(JNIEnv* env) {
    // jthrowable throwable = env->ExceptionOccurred(); may be used to trace exception details here
    bool exception_occured = (*env)->ExceptionCheck(env);
    if (exception_occured) { (*env)->ExceptionClear(env); }
    return exception_occured;
}

// ANativeActivity.clazz is NativeActivity object handle.
// IMPORTANT NOTE: This member is mis-named. It should really be named 'activity' instead of 'clazz',
// since it's a reference to the NativeActivity instance created by the system for you.
// We unfortunately cannot change this without breaking NDK source-compatibility.

static jobject activity_this(ANativeActivity* na) {
    jobject activity = na->clazz; // which is misnamed in NDK code with the note above
    return activity;              // exquisite attention to details was always paramount in Android code...
}

jobject get_system_service(ANativeActivity* na, const char* label) {
    JNIEnv* env = na->env;
/*
    jclass activity_class = (*env)->GetObjectClass(env, activity_this(na));
    jmethodID getClassLoader_mid = (*env)->GetMethodID(env, activity_class, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject class_loader = (*env)->CallObjectMethod(env, activity_class, getClassLoader_mid);
*/
    traceln("%s", label);
    jclass    context_class = (*env)->FindClass(env, "android/content/Context");
    traceln("context_class %p", context_class);
    jfieldID  service_name_fid = (*env)->GetStaticFieldID(env, context_class, label, "Ljava/lang/String;");
    traceln("service_name_fid %p", service_name_fid);
    jstring   service_name_string = (*env)->GetStaticObjectField(env, context_class, service_name_fid);
    traceln("service_name_string %p", service_name_string);
    jmethodID getSystemService_mid = (*env)->GetMethodID(env, context_class, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    traceln("getSystemService_mid %p", getSystemService_mid);
    jobject   service = (*env)->CallObjectMethod(env, activity_this(na), getSystemService_mid, service_name_string);
    traceln("service %p", service);
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
//  jobject vibrator_service = get_system_service(na, "VIBRATOR_SERVICE");
    bool supported = vibrator_service != null;
    traceln("vibrator_service %p", vibrator_service);
//  jclass vibrator_service_class = supported ? (*env)->GetObjectClass(env, vibrator_service) : null;
    traceln("vibrator_service_class %p", vibrator_service_class);
    supported = supported && vibrator_service_class != null;
    jclass vibration_effect_class = supported ? (*env)->FindClass(env, "android/os/VibrationEffect") : null;
    traceln("vibration_effect_class %p", vibration_effect_class);
    supported = supported && vibration_effect_class != null;
    jfieldID effect_fid = supported ? (*env)->GetStaticFieldID(env, vibration_effect_class, effect, "I") : null;
    traceln("effect_fid %p", effect_fid);
    supported = supported && effect_fid != null;
    const int effect_int = supported ? (*env)->GetStaticIntField(env, vibration_effect_class, effect_fid) : 0;
    jmethodID vibrate_mid = supported ? (*env)->GetMethodID(env, vibrator_service_class, "vibrate", "(Landroid/os/VibrationEffect)V") : null;
    traceln("vibrate_mid %p", vibrate_mid);
    supported = supported && vibrate_mid != null;
    if (supported) {
        (*env)->CallVoidMethod(env, vibrator_service, vibrate_mid, effect_int);
        traceln("Vibrator.vibrate(%s=%d) has been called", effect, effect_int);
    } else {
        traceln("unsupported");
    }
    return !check_and_clear_exception(env) && supported;
}

bool android_jni_vibrate_milliseconds(ANativeActivity* na, int milliseconds) {
    JNIEnv* env = na->env;
//  jobject vibrator_service = get_system_service(na, "VIBRATOR_SERVICE");
    bool supported = vibrator_service != null;
//  jclass vibrator_service_class = supported ? (*env)->GetObjectClass(env, vibrator_service) : null;
    traceln("vibrator_service_class %p", vibrator_service_class);
    supported = supported && vibrator_service_class != null;
    jmethodID vibrate_mid = supported ? (*env)->GetMethodID(env, vibrator_service_class, "vibrate", "(J)V") : null;
    traceln("vibrate_mid %p", vibrate_mid);
    supported = supported && vibrate_mid != null;
    jlong ms = milliseconds;
    if (supported) {
        (*env)->CallVoidMethod(env, vibrator_service, vibrate_mid, ms);
        traceln("Vibrator.vibrate(%d) has been called", milliseconds);
    } else {
        traceln("unsupported");
    }
    return !check_and_clear_exception(env) && supported;
}

void android_jni_attach_current_thread_as_daemon(JNIEnv* env) {
    (*jvm)->AttachCurrentThreadAsDaemon(jvm, &env, null);
}

void android_jni_dettach_current_thread(JNIEnv* env) {
    (*jvm)->DetachCurrentThread(jvm);
}

void android_jni_init(ANativeActivity* na) {
    JNIEnv* env = na->env;
    (*env)->GetJavaVM(env, &jvm);
    jobject vs = get_system_service(na, "VIBRATOR_SERVICE");
    if (vs != null) { vibrator_service = (*env)->NewGlobalRef(env, vs); }
    jclass vs_class = vibrator_service != null ? (*env)->GetObjectClass(env, vibrator_service) : null;
    if (vs_class != null) { vibrator_service_class = (*env)->NewGlobalRef(env, vs_class); }


}

void android_jni_done(ANativeActivity* na) {
    JNIEnv* env = na->env;
    if (vibrator_service != null) { (*env)->DeleteGlobalRef(env, vibrator_service); vibrator_service = null; }
    if (vibrator_service_class != null) { (*env)->DeleteGlobalRef(env, vibrator_service_class); vibrator_service_class = null; }
}

END_C
