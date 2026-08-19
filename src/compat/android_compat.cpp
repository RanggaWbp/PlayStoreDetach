/**
 * Android 7.0 (API 24) ~ Android 17 (API 37) + future-proof.
 * Strategi: hardcode yang pasti diketahui (24-37),
 * SDK 38+ → fallback ke behavior SDK 37 (terbaru yang diketahui).
 */
#include "android_compat.h"

#include <sys/system_properties.h>
#include <cstring>
#include <cstdlib>

// ============================================================
// AndroidVersion
// ============================================================
int AndroidVersion::cached_sdk_int = -1;
bool AndroidVersion::cache_initialized = false;

int AndroidVersion::get() {
    if (cache_initialized) return cached_sdk_int;

    char sdk[PROP_VALUE_MAX] = {0};
    __system_property_get("ro.build.version.sdk", sdk);
    cached_sdk_int = atoi(sdk);

    if (cached_sdk_int < 24) {
        cached_sdk_int = 24;  // sanity clamp minimum
    }
    cache_initialized = true;
    return cached_sdk_int;
}

int AndroidVersion::getEffectiveSDK() {
    int actual = get();
    if (actual > SDK_MAX_KNOWN) {
        LOGI("SDK " + std::to_string(actual) + " > " +
             std::to_string(SDK_MAX_KNOWN) + ", future-proof mode (behavior SDK 37)");
        return SDK_MAX_KNOWN;
    }
    return actual;
}

bool AndroidVersion::isAtLeast(int sdk)      { return get() >= sdk; }
bool AndroidVersion::isModernAndroid()       { return isAtLeast(31); }
bool AndroidVersion::isAndroid16Plus()       { return isAtLeast(36); }
bool AndroidVersion::isUnknownFuture()       { return get() > SDK_MAX_KNOWN; }

std::string AndroidVersion::getVersionString() {
    switch (get()) {
        case 24: return "Android 7.0 (Nougat)";
        case 25: return "Android 7.1 (Nougat MR1)";
        case 26: return "Android 8.0 (Oreo)";
        case 27: return "Android 8.1 (Oreo MR1)";
        case 28: return "Android 9 (Pie)";
        case 29: return "Android 10 (Q)";
        case 30: return "Android 11 (R)";
        case 31: return "Android 12 (S)";
        case 32: return "Android 12L (Sv2)";
        case 33: return "Android 13 (Tiramisu)";
        case 34: return "Android 14 (UpsideDownCake)";
        case 35: return "Android 15 (VanillaIceCream)";
        case 36: return "Android 16 (Baklava)";
        case 37: return "Android 17";
        default:
            if (get() > SDK_MAX_KNOWN)
                return "Android (Future, SDK " + std::to_string(get()) + ")";
            return "Unknown (SDK " + std::to_string(get()) + ")";
    }
}

AndroidCapabilities AndroidVersion::getCapabilities() {
    static AndroidCapabilities caps = {};
    static bool cached = false;
    if (cached) return caps;

    int sdk = get();
    caps.has_aidl_package_manager = (sdk >= 31);
    caps.has_stable_binder_codes  = (sdk >= 31);
    caps.uses_flag_permutations   = (sdk >= 33);
    caps.has_background_install   = (sdk >= 34);
    caps.uses_sdk_ext             = (sdk >= 31);
    caps.has_archived_apps        = (sdk >= 35);

    if (sdk >= 34)      caps.art_layout_generation = 5;  // Android 14-17+
    else if (sdk >= 31) caps.art_layout_generation = 4;  // Android 12-13
    else if (sdk >= 28) caps.art_layout_generation = 3;  // Android 9-11
    else if (sdk >= 26) caps.art_layout_generation = 2;  // Android 8
    else                caps.art_layout_generation = 1;  // Android 7

    cached = true;
    return caps;
}

// ============================================================
// AndroidCompat
// ============================================================
std::string AndroidCompat::getPackageManagerClassName() {
    // Valid untuk Android 7-17 (binary name tetap sama)
    return "android/app/ApplicationPackageManager";
}

std::string AndroidCompat::getIPackageManagerDescriptor() {
    return "android.content.pm.IPackageManager";
}

std::string AndroidCompat::getTransactMethodName() {
    return (AndroidVersion::get() >= 31) ? "transactNative" : "transact";
}

int AndroidCompat::getPackageManagerTransactionCode(const std::string &method) {
    int sdk = AndroidVersion::get();

    if (sdk < 31) {
        // Android 7-11: hand-written binder codes
        if (method == "getInstalledPackages")     return 2;
        if (method == "getInstalledApplications") return 4;
        if (method == "getPackageInfo")           return 6;
        return -1;
    }

    // Android 12+: AIDL-generated — fallback kalibrasi.
    // Runtime resolution (resolveTransactionCodeRuntime) dipakai duluan.
    if (method == "getInstalledPackages")     return 0x3D;
    if (method == "getInstalledApplications") return 0x3F;
    if (method == "getPackageInfo")           return 0x1A;
    return -1;
}

int AndroidCompat::resolveTransactionCodeRuntime(JNIEnv *env,
                                                  const std::string &method_name) {
    if (!env) return -1;

    jclass stub = env->FindClass("android/content/pm/IPackageManager$Stub");
    if (!stub) {
        env->ExceptionClear();
        stub = env->FindClass("android/content/pm/IPackageManager");
        if (!stub) {
            env->ExceptionClear();
            return -1;
        }
    }

    std::string field_name = "TRANSACTION_" + method_name;
    jfieldID field = env->GetStaticFieldID(stub, field_name.c_str(), "I");
    if (!field) {
        env->ExceptionClear();
        env->DeleteLocalRef(stub);
        return -1;
    }

    jint code = env->GetStaticIntField(stub, field);
    env->DeleteLocalRef(stub);
    LOGD("TX code " + method_name + " = " + std::to_string(code));
    return code;
}

bool AndroidCompat::isSupportedAndroidVersion() {
    return AndroidVersion::get() >= 24;  // tanpa batas atas
}

std::string AndroidCompat::getFullCompatReport() {
    AndroidCapabilities caps = AndroidVersion::getCapabilities();
    std::string r;
    r += "Android: " + AndroidVersion::getVersionString() +
         " (SDK " + std::to_string(AndroidVersion::get()) + ")";
    if (AndroidVersion::isUnknownFuture())
        r += " [FUTURE-PROOF MODE: behavior SDK 37]";
    r += " | AIDL:" + std::string(caps.has_aidl_package_manager ? "Y" : "N");
    r += " FlagsPerm:" + std::string(caps.uses_flag_permutations ? "Y" : "N");
    r += " Archived:" + std::string(caps.has_archived_apps ? "Y" : "N");
    r += " ARTgen:" + std::to_string(caps.art_layout_generation);
    return r;
}
