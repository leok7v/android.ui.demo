#include "rt.h"
#include "android_binder.h"

BEGIN_C

static uint32_t service_manager_lookup(binder_state* bs, uint32_t target, const char* name) {
    byte data[256] = {};
    binder_io msg = {};
    binder_io reply = {};
    bio_init(&msg, data, sizeof(data), 4);
    bio_put_uint32(&msg, 0);  // strict mode header
    bio_put_string16_x(&msg, SERVICE_MANAGER_INTERFACE);
    bio_put_string16_x(&msg, name);
    if (binder_call(bs, &msg, &reply, target, GET_SERVICE_TRANSACTION)) {
        return 0;
    }
    uint32_t handle = bio_get_ref(&reply);
    if (handle != 0) {
        binder_acquire(bs, handle);
    }
    binder_done(bs, &msg, &reply);
    return handle;
}

static int service_manager_publish(binder_state* bs, uint32_t target, const char* name, void* ptr) {
    byte data[256] = {};
    binder_io msg = {};
    binder_io reply = {};
    bio_init(&msg, data, sizeof(data), 4);
    bio_put_uint32(&msg, 0);  // strict mode header
    bio_put_string16_x(&msg, SERVICE_MANAGER_INTERFACE);
    bio_put_string16_x(&msg, name);
    bio_put_obj(&msg, ptr);
    if (binder_call(bs, &msg, &reply, target, ADD_SERVICE_TRANSACTION)) {
        return -1;
    }
    int status = bio_get_uint32(&reply);
    binder_done(bs, &msg, &reply);
    return status;
}

// https://github.com/pandalion98/SPay-inner-workings/blob/master/boot.oat_files/arm64/dex/out0/android/os/IVibratorService.java#L313

enum {
    VIBRATOR_HAS_VIBRATOR = 1, // boolean hasVibrator();
    VIBRATOR_VIBRATE = 2,
    VIBRATOR_VIBRATE_WITH_EFFECT = 2,
    VIBRATOR_CANCEL = 4
};

enum { // VibrationEffect
    DEFAULT_AMPLITUDE = -1,
    EFFECT_CLICK = 0,
    EFFECT_DOUBLE_CLICK = 1,
    EFFECT_TICK = 2,
    EFFECT_HEAVY_TICK = 5
};

enum { // AudioAttributes
    USAGE_UNKNOWN = 0,
    USAGE_MEDIA = 1,
    USAGE_VOICE_COMMUNICATION = 2,
    USAGE_VOICE_COMMUNICATION_SIGNALLING = 3,
    USAGE_ALARM = 4,
    USAGE_NOTIFICATION = 5,
    USAGE_NOTIFICATION_TELEPHONY_RINGTONE = 6,
    USAGE_NOTIFICATION_COMMUNICATION_REQUEST = 7,
    USAGE_NOTIFICATION_COMMUNICATION_INSTANT = 8,
    USAGE_NOTIFICATION_COMMUNICATION_DELAYED = 9,
    USAGE_NOTIFICATION_EVENT = 10,
    USAGE_ASSISTANCE_ACCESSIBILITY = 11,
    USAGE_ASSISTANCE_NAVIGATION_GUIDANCE = 12,
    USAGE_ASSISTANCE_SONIFICATION = 13,
    USAGE_GAME = 14
};

/* Alternaitive and newer but also does not work:

enum {
    VIBRATOR_HAS_VIBRATOR = 1, // boolean hasVibrator();
    VIBRATOR_HAS_AMPLITUDE_CONTROL = 2, // boolean hasAmplitudeControl();
    VIBRATOR_SET_ALWAYS_ON_EFFECT = 3, // boolean setAlwaysOnEffect(int uid, String opPkg, int alwaysOnId, in VibrationEffect effect, in AudioAttributes attributes);
    VIBRATOR_VIBRATE = 4, // void vibrate(int uid, String opPkg, in VibrationEffect effect, in AudioAttributes attributes, String reason, IBinder token);
    VIBRATOR_CANCEL = 5 // void cancelVibrate(IBinder token);
};
*/

static int vibrator_vibrate(binder_state* bs, uint32_t target, int64_t milliseconds, int32_t usage, void* token) {
    unsigned iodata[512/4];
    binder_io msg = {};
    binder_io reply = {};
    bio_init(&msg, iodata, sizeof(iodata), 4);
    bio_put_uint32(&msg, 0); // strict mode header
    static const char* interface_token = "android.os.IVibratorService";
    static const char* package = "android.os";
    // void vibrate(int uid, String opPkg, long milliseconds, int usageHint, IBinder token)
    bio_put_string16_x(&msg, interface_token);
    bio_put_uint32(&msg, getuid());
    bio_put_string16_x(&msg, package);
    bio_put_uint64(&msg, milliseconds);
    bio_put_uint32(&msg, usage);
    bio_put_obj(&msg, token);
    if (binder_call(bs, &msg, &reply, target, VIBRATOR_VIBRATE)) {
        return -1;
    }
    binder_done(bs, &msg, &reply);
    return 0;
}

static int vibrator_vibrate_effect(binder_state* bs, uint32_t target, int32_t effect, int32_t usage, void* token) {
    unsigned iodata[512/4];
    binder_io msg = {};
    binder_io reply = {};
    bio_init(&msg, iodata, sizeof(iodata), 4);
    bio_put_uint32(&msg, 0); // strict mode header
    static const char* interface_token = "android.os.IVibratorService";
    static const char* package = "android.os";
    // void vibrate(int uid, String opPkg, in VibrationEffect effect, in AudioAttributes attributes, String reason, IBinder token);
    bio_put_string16_x(&msg, interface_token);
    bio_put_uint32(&msg, getuid());
    bio_put_string16_x(&msg, package);
    bio_put_uint32(&msg, effect);
    bio_put_uint32(&msg, usage);
    bio_put_string16_x(&msg, "");
    bio_put_obj(&msg, token);
    if (binder_call(bs, &msg, &reply, target, 4)) { // VIBRATOR_VIBRATE = 4 from aidl?
        return -1;
    }
    binder_done(bs, &msg, &reply);
    return 0;
}

static int vibrator_cancel(binder_state* bs, uint32_t target, void* token) {
    unsigned iodata[512/4];
    binder_io msg = {};
    binder_io reply = {};
    bio_init(&msg, iodata, sizeof(iodata), 4);
    bio_put_uint32(&msg, 0); // strict mode header
    static const char* interface_token = "android.os.IVibratorService";
    // cancelVibrate(IBinder token)
    bio_put_string16_x(&msg, interface_token);
    bio_put_obj(&msg, token);
    if (binder_call(bs, &msg, &reply, target, VIBRATOR_CANCEL)) {
        return -1;
    }
    binder_done(bs, &msg, &reply);
    return 0;
}

static void haptic_test() {
    binder_state* bs = binder_open(128*1024);
    if (bs == null) {
        traceln( "failed to open binder driver");
        return;
    }
    // https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/os/IVibratorService.aidl
    uint32_t handle = service_manager_lookup(bs, BINDER_SERVICE_MANAGER, "vibrator");
    traceln("android.os.IVibratorService %p", handle);
    if (handle != 0) {
        static uint32_t token;
        // TODO: remains to be seen why it is not working at all on any of my 4 different Androids:
        vibrator_vibrate(bs, handle, 100, USAGE_ASSISTANCE_ACCESSIBILITY, &token);
        vibrator_vibrate_effect(bs, handle, EFFECT_CLICK, USAGE_ASSISTANCE_ACCESSIBILITY, &token);
        vibrator_cancel(bs, handle, &token);
    }
}

static_init(haptic) {
    (void)service_manager_publish; // could be used to publish local service
    (void)haptic_test; // unused because it simply does not work
}

END_C
