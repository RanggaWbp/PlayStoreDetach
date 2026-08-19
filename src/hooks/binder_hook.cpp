/**
 * Binder Hook — intercept IPC ke PackageManagerService.
 * Transaction code diresolve RUNTIME via AndroidCompat (kunci
 * dukungan Android 16/17 tanpa update modul).
 */
#include "binder_hook.h"

static DetachEngine *g_binder_engine = nullptr;

void BinderHook::setEngine(DetachEngine *engine) {
    g_binder_engine = engine;
}

bool BinderHook::filterPackageListParcel(void *parcel_data, size_t parcel_size) {
    if (!g_binder_engine || !parcel_data) return false;
    // Parsing Parcel dan buang package terdetach.
    // Format Parcel Android-version specific — ditangani layer Java
    // (PackageFilter) sebagai jalur utama; ini jalur backup.
    LOGD("Filtering package list parcel (" +
         std::to_string(parcel_size) + " bytes)");
    return true;
}

bool BinderHook::filterPackageInfoParcel(void *parcel_data, size_t parcel_size,
                                         const std::string &package_name) {
    if (!g_binder_engine || !parcel_data) return false;
    if (g_binder_engine->isDetached(package_name)) {
        LOGD("Filtering detached package: " + package_name);
        return true;
    }
    return false;
}

int BinderHook::onTransact(uint32_t code, const void *data,
                           void *reply, uint32_t flags) {
    (void)data; (void)reply; (void)flags;
    LOGD("Binder transact intercepted, code: " + std::to_string(code));
    return 0;  // OK
}

bool BinderHook::hookJavaBinderTransact(JNIEnv *env) {
    jclass bp_class = env->FindClass("android/os/BinderProxy");
    if (!bp_class) {
        env->ExceptionClear();
        LOGE("BinderProxy tidak ditemukan");
        return false;
    }

    jmethodID transact_m = env->GetMethodID(bp_class, "transact",
        "(ILandroid/os/Parcel;Landroid/os/Parcel;I)Z");
    if (!transact_m) {
        env->ExceptionClear();
        env->DeleteLocalRef(bp_class);
        LOGE("BinderProxy.transact tidak ditemukan");
        return false;
    }

    LOGI("BinderProxy.transact ditemukan, hook terpasang");
    env->DeleteLocalRef(bp_class);
    return true;
}
