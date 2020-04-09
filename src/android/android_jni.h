#pragma once
#include "rt.h"
#include <android/native_activity.h>

BEGIN_C // interfacce to unavoidable Android Java code via jni

void android_jni_attach_current_thread_as_daemon(JNIEnv* env);
void android_jni_dettach_current_thread(JNIEnv* env);

void android_jni_init(ANativeActivity* na);
void android_jni_done(ANativeActivity* na);

bool android_jni_hide_navigation_bar(ANativeActivity* na);

bool android_jni_vibrate_milliseconds(ANativeActivity* na, int milliseconds);

// "DEFAULT_AMPLITUDE", "EFFECT_CLICK", "EFFECT_DOUBLE_CLICK" "EFFECT_TICK", "EFFECT_HEAVY_TICK"
bool android_jni_vibrate_with_effect(ANativeActivity* na, const char* effect);


END_C
