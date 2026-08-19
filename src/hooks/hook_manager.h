#ifndef ZYGISK_DETACH_HOOK_MANAGER_H
#define ZYGISK_DETACH_HOOK_MANAGER_H

#include "../../include/common.h"
#include "../core/detach_engine.h"
#include <jni.h>

class HookManager {
public:
    HookManager(DetachEngine *engine);
    ~HookManager();

    bool initialize();

    // Layer 1-4 (dengan fallback berantai)
    bool applyJavaHook();
    bool applyBinderHook();
    bool applyProviderHook();
    bool applyNativeHook();

    // OEM compat
    bool applyMIUIHyperOSCompat();
    bool applyOneUICompat();
    bool applyOPPOCompat();
    bool applyEMUICompat();

    // Hook Android modern
    bool applyFlagPermutationHook();   // Android 13+ (PackageInfoFlags)
    bool applyArchivedAppHook();       // Android 15+ (archived apps)

    void onPostFork();

private:
    DetachEngine *engine;
    bool java_hooked;
    bool binder_hooked;
    bool provider_hooked;
    bool native_hooked;

    bool hookGetInstalledPackages(JNIEnv *env);
    bool hookGetInstalledApplications(JNIEnv *env);
    bool hookGetPackageInfo(JNIEnv *env);
    bool hookQueryIntentActivities(JNIEnv *env);
    bool hookPackageManagerMethod(JNIEnv *env, const char *method_name);
    bool setupIPackageManagerProxy(JNIEnv *env);
    bool createPackageManagerProxy(JNIEnv *env, jobject original_pm);
    bool createPackageManagerProxyModern(JNIEnv *env, jobject original_pm);
    bool createPackageManagerProxyLegacy(JNIEnv *env, jobject original_pm);
    bool hookBinderTransact(JNIEnv *env, jobject binder_object);
    bool hookApplicationPackageManager(JNIEnv *env, const char *method_name);
    bool applyNativeBinderHook();

    JNIEnv *getEnv();
};

#endif
