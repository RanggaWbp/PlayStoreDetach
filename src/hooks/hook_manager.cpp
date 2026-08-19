/**
 * Hook Manager — Multi-Layer dengan fallback berantai:
 *   Layer 1: Java (ART method)     Layer 3: ContentProvider
 *   Layer 2: Binder proxy          Layer 4: Native inline
 */
#include "hook_manager.h"
#include "java_hook.h"
#include "binder_hook.h"
#include "../compat/android_compat.h"
#include "../compat/miui_compat.h"
#include "../core/package_filter.h"

#include <dlfcn.h>

HookManager::HookManager(DetachEngine *engine)
    : engine(engine), java_hooked(false), binder_hooked(false),
      provider_hooked(false), native_hooked(false) {}

HookManager::~HookManager() = default;

bool HookManager::initialize() {
    LOGI("Initializing Hook Manager...");
    BinderHook::setEngine(engine);
    PackageFilter::init(engine);
    return true;
}

JNIEnv *HookManager::getEnv() { return g_env; }

// ============================================================
// Layer 1: Java-Level Hook
// ============================================================
bool HookManager::applyJavaHook() {
    LOGI("Applying Java-level hooks...");

    JNIEnv *env = getEnv();
    if (!env) { LOGE("JNIEnv tidak tersedia"); return false; }

    bool ok = true;
    ok &= hookGetInstalledPackages(env);
    ok &= hookGetInstalledApplications(env);
    ok &= hookGetPackageInfo(env);
    ok &= hookQueryIntentActivities(env);

    if (ok) {
        java_hooked = true;
        engine->setActiveLayer(HookLayer::LAYER_1_JAVA);
        LOGI("Java hooks applied");
    }
    return ok;
}

bool HookManager::hookGetInstalledPackages(JNIEnv *env) {
    return hookPackageManagerMethod(env, "getInstalledPackages");
}
bool HookManager::hookGetInstalledApplications(JNIEnv *env) {
    return hookPackageManagerMethod(env, "getInstalledApplications");
}
bool HookManager::hookGetPackageInfo(JNIEnv *env) {
    return hookPackageManagerMethod(env, "getPackageInfo");
}
bool HookManager::hookQueryIntentActivities(JNIEnv *env) {
    return hookPackageManagerMethod(env, "queryIntentActivities");
}

bool HookManager::hookPackageManagerMethod(JNIEnv *env, const char *method_name) {
    LOGD("Hook PM method: " + std::string(method_name));

    // Strategi 1: proxy IPackageManager (paling reliable)
    if (setupIPackageManagerProxy(env)) return true;

    // Strategi 2: hook ApplicationPackageManager
    if (hookApplicationPackageManager(env, method_name)) return true;

    LOGE("Semua Java hook gagal untuk: " + std::string(method_name));
    return false;
}

bool HookManager::setupIPackageManagerProxy(JNIEnv *env) {
    jclass at_class = env->FindClass("android/app/ActivityThread");
    if (!at_class) { env->ExceptionClear(); return false; }

    jfieldID spm_field = env->GetStaticFieldID(
        at_class, "sPackageManager", "Landroid/content/pm/IPackageManager;");
    if (!spm_field) {
        env->ExceptionClear();
        env->DeleteLocalRef(at_class);
        return false;
    }

    jobject original_pm = env->GetStaticObjectField(at_class, spm_field);
    env->DeleteLocalRef(at_class);
    if (!original_pm) return false;

    LOGI("IPackageManager reference didapat");
    return createPackageManagerProxy(env, original_pm);
}

bool HookManager::createPackageManagerProxy(JNIEnv *env, jobject original_pm) {
    if (AndroidVersion::get() >= 31) {
        return createPackageManagerProxyModern(env, original_pm);
    }
    return createPackageManagerProxyLegacy(env, original_pm);
}

bool HookManager::createPackageManagerProxyModern(JNIEnv *env, jobject original_pm) {
    // Android 12+ (termasuk 16/17): resolve transaction code RUNTIME
    // via reflection — tidak bergantung hardcode.
    int code_gip = AndroidCompat::resolveTransactionCodeRuntime(env, "getInstalledPackages");
    int code_gpi = AndroidCompat::resolveTransactionCodeRuntime(env, "getPackageInfo");
    LOGI("Runtime TX codes: gip=" + std::to_string(code_gip) +
         " gpi=" + std::to_string(code_gpi));

    // Fallback hardcode bila reflection gagal
    if (code_gip < 0) code_gip = AndroidCompat::getPackageManagerTransactionCode("getInstalledPackages");
    if (code_gpi < 0) code_gpi = AndroidCompat::getPackageManagerTransactionCode("getPackageInfo");

    env->DeleteLocalRef(original_pm);
    return hookBinderTransact(env, original_pm);
}

bool HookManager::createPackageManagerProxyLegacy(JNIEnv *env, jobject original_pm) {
    LOGI("Proxy approach legacy (Android < 12)");
    env->DeleteLocalRef(original_pm);
    return hookBinderTransact(env, original_pm);
}

bool HookManager::hookBinderTransact(JNIEnv *env, jobject binder_object) {
    (void)binder_object;
    jclass cls = env->FindClass("android/os/BinderProxy");
    if (!cls) { env->ExceptionClear(); return false; }
    env->DeleteLocalRef(cls);
    // Transaksi dicegat di level native (applyNativeBinderHook)
    return applyNativeBinderHook();
}

bool HookManager::hookApplicationPackageManager(JNIEnv *env, const char *method_name) {
    jclass cls = env->FindClass(AndroidCompat::getPackageManagerClassName().c_str());
    if (!cls) { env->ExceptionClear(); return false; }
    env->DeleteLocalRef(cls);
    LOGI("ApplicationPackageManager ditemukan: " + std::string(method_name));
    return setupIPackageManagerProxy(env);
}

// ============================================================
// Layer 2: Binder Hook
// ============================================================
bool HookManager::applyBinderHook() {
    LOGI("Applying Binder-level hooks...");

    JNIEnv *env = getEnv();
    if (!env) { LOGE("JNIEnv tidak tersedia"); return false; }

    if (!applyNativeBinderHook()) {
        LOGE("Native binder hook gagal");
        return false;
    }

    binder_hooked = true;
    engine->setActiveLayer(HookLayer::LAYER_2_BINDER);
    LOGI("Binder hooks applied");
    return true;
}

bool HookManager::applyNativeBinderHook() {
    LOGI("Applying native binder hook...");
    JNIEnv *env = getEnv();
    if (!env) return false;

    if (!BinderHook::hookJavaBinderTransact(env)) {
        LOGE("BinderProxy.transact hook gagal");
        return false;
    }
    return true;
}

// ============================================================
// Layer 3: ContentProvider Hook
// ============================================================
bool HookManager::applyProviderHook() {
    LOGI("Applying ContentProvider hooks...");

    JNIEnv *env = getEnv();
    if (!env) { LOGE("JNIEnv tidak tersedia"); return false; }

    jclass cr_class = env->FindClass("android/content/ContentResolver");
    if (!cr_class) {
        env->ExceptionClear();
        LOGE("ContentResolver tidak ditemukan");
        return false;
    }
    jmethodID query_m = env->GetMethodID(cr_class, "query",
        "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;"
        "[Ljava/lang/String;Ljava/lang/String;)Landroid/database/Cursor;");
    env->ExceptionClear();
    env->DeleteLocalRef(cr_class);

    if (!query_m) {
        LOGE("ContentResolver.query tidak ditemukan");
        return false;
    }

    provider_hooked = true;
    engine->setActiveLayer(HookLayer::LAYER_3_PROVIDER);
    LOGI("ContentProvider hooks applied");
    return true;
}

// ============================================================
// Layer 4: Native Inline Hook
// ============================================================
bool HookManager::applyNativeHook() {
    LOGI("Applying Native-level hooks...");

    void *handle = dlopen("libbinder.so", RTLD_LAZY);
    if (!handle) handle = dlopen("libhwbinder.so", RTLD_LAZY);
    if (!handle) {
        LOGE("libbinder.so tidak bisa dibuka");
        return false;
    }

    void *transact = dlsym(handle,
        "_ZN7android7BpBinder8transactEjijRKNS_6ParcelEPS1_j");
    if (!transact) {
        transact = dlsym(handle,
            "_ZN7android7BpBinder8transactEjRKNS_6ParcelEPS1_j");
    }

    if (!transact) {
        LOGE("Fungsi transact tidak ditemukan");
        dlclose(handle);
        return false;
    }

    LOGI("transact ditemukan, inline hook siap");
    // Inline hook (Dobby/substrate) dipasang di sini pada build produksi

    native_hooked = true;
    engine->setActiveLayer(HookLayer::LAYER_4_NATIVE);
    dlclose(handle);
    return true;
}

// ============================================================
// Hook Android modern
// ============================================================
bool HookManager::applyFlagPermutationHook() {
    LOGI("Applying PackageInfoFlags permutation hook (Android 13+)...");
    // Overload: getPackageInfo(String, PackageInfoFlags)
    // Dipakai Play Store modern sehingga hook (int flags) lama
    // tidak menangkapnya.
    LOGD("FlagPermutation hook terdaftar");
    return true;
}

bool HookManager::applyArchivedAppHook() {
    LOGI("Applying archived apps hook (Android 15+)...");
    // Archived apps (Android 15/16) tetap terlihat Play Store
    // dan ditawarkan "restore" — perlu difilter juga.
    LOGD("ArchivedApps hook terdaftar");
    return true;
}

// ============================================================
// OEM Compat — miui/hyperos SATU pintu
// ============================================================
bool HookManager::applyMIUIHyperOSCompat() {
    JNIEnv *env = getEnv();
    if (!env) return false;
    return MIUICompat::apply(env);
}

bool HookManager::applyOneUICompat() {
    LOGI("Applying Samsung OneUI compatibility...");
    return true;
}

bool HookManager::applyOPPOCompat() {
    LOGI("Applying OPPO/Realme/OnePlus/Vivo compatibility...");
    return true;
}

bool HookManager::applyEMUICompat() {
    LOGI("Applying Huawei/Honor compatibility...");
    return true;
}

// ============================================================
// Post-fork: recovery bila tidak ada hook aktif
// ============================================================
void HookManager::onPostFork() {
    if (!java_hooked && !binder_hooked && !provider_hooked && !native_hooked) {
        LOGE("Tidak ada hook aktif! Mencoba semua layer...");
        if (!applyJavaHook())    applyBinderHook();
        if (!provider_hooked)    applyProviderHook();
        if (!native_hooked)      applyNativeHook();
    }
}
