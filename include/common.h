#ifndef ZYGISK_DETACH_COMMON_H
#define ZYGISK_DETACH_COMMON_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include <mutex>
#include <atomic>
#include <optional>

#include <jni.h>
#include <android/log.h>
#include <sys/system_properties.h>

// ============================================================
// Android 7 ~ 17 + FUTURE-PROOF
// ============================================================
#define ANDROID_VERSION_UNKNOWN  0
#define ANDROID_N               24  // Android 7.0
#define ANDROID_N_MR1           25  // Android 7.1
#define ANDROID_O               26  // Android 8.0
#define ANDROID_O_MR1           27  // Android 8.1
#define ANDROID_P               28  // Android 9
#define ANDROID_Q               29  // Android 10
#define ANDROID_R               30  // Android 11
#define ANDROID_S               31  // Android 12
#define ANDROID_S_V2            32  // Android 12L
#define ANDROID_TIRAMISU        33  // Android 13
#define ANDROID_UPSIDE_DOWN     34  // Android 14
#define ANDROID_VANILLA_ICE     35  // Android 15
#define ANDROID_BAKLAVA         36  // Android 16
#define ANDROID_17              37  // Android 17

// SDK di atas nilai ini → otomatis pakai behavior terbaru (future-proof)
#define SDK_MAX_KNOWN           37

// ============================================================
// Capability Flags — deteksi KEMAMPUAN, bukan angka versi
// ============================================================
struct AndroidCapabilities {
    bool has_aidl_package_manager;    // >= 12
    bool has_stable_binder_codes;     // >= 12
    bool uses_flag_permutations;      // >= 13
    bool has_background_install;      // >= 14
    bool uses_sdk_ext;                // >= 12
    bool has_archived_apps;           // >= 15
    int  art_layout_generation;       // 1..5
};

// ============================================================
// OEM — MIUI/HYPEROS DIGABUNG
// ============================================================
enum class OEMType {
    UNKNOWN,
    STOCK_AOSP,
    MIUI_HYPEROS,    // ← gabungan Xiaomi MIUI & HyperOS
    ONEUI,
    COLOROS,
    ORIGINOS,
    EMUI,
    FLYME,
    MAGICOS,
    REALME_UI,
    OXYGENOS,
    FUNTOUCHOS
};

enum class XiaomiSkin {
    NONE,
    MIUI,
    HYPEROS
};

enum class HookLayer {
    LAYER_NONE = 0,
    LAYER_1_JAVA = 1,
    LAYER_2_BINDER = 2,
    LAYER_3_PROVIDER = 3,
    LAYER_4_NATIVE = 4
};

// ============================================================
// Configuration
// ============================================================
struct DetachConfig {
    bool debug = false;
    bool force_detach = true;
    bool hide_from_play = true;
    bool block_update_check = true;
    bool aggressive_mode = false;
    std::vector<std::string> detached_packages;
};

// ============================================================
// Global JNIEnv (di-set saat onLoad di main.cpp)
// ============================================================
extern JNIEnv *g_env;

// ============================================================
// Logger
// ============================================================
class Logger {
private:
    static std::atomic<bool> debug_enabled;
    static std::string log_buffer;
    static std::mutex log_mutex;

public:
    static void setDebug(bool enable) { debug_enabled = enable; }
    static void log(const std::string &tag, const std::string &message);
    static void error(const std::string &tag, const std::string &message);
    static void debug(const std::string &tag, const std::string &message);
    static void flushToFile(const std::string &path);
};

#define LOG_TAG "ZygiskDetach"
#define LOGI(msg) Logger::log(LOG_TAG, msg)
#define LOGE(msg) Logger::error(LOG_TAG, msg)
#define LOGD(msg) Logger::debug(LOG_TAG, msg)

// ============================================================
// Android Version — future-proof
// ============================================================
class AndroidVersion {
private:
    static int cached_sdk_int;
    static bool cache_initialized;

public:
    static int get();                  // SDK actual (bisa > 37)
    static int getEffectiveSDK();      // dibatasi ke SDK_MAX_KNOWN
    static bool isAtLeast(int sdk);
    static bool isModernAndroid();     // >= 31
    static bool isAndroid16Plus();     // >= 36
    static bool isUnknownFuture();     // > 37
    static std::string getVersionString();
    static AndroidCapabilities getCapabilities();
};

// ============================================================
// OEM Detector
// ============================================================
class OEMDetector {
private:
    static OEMType detected_oem;
    static XiaomiSkin xiaomi_skin;
    static std::string skin_version_str;
    static bool detected;

public:
    static OEMType detect();
    static OEMType get();
    static std::string getOEMName();
    static XiaomiSkin getXiaomiSkin();
    static std::string getSkinVersion();

    static bool isMIUIHyperOS() {
        return get() == OEMType::MIUI_HYPEROS;
    }
    static bool isSamsung()  { return get() == OEMType::ONEUI; }
    static bool isOPPO()     { return get() == OEMType::COLOROS || get() == OEMType::REALME_UI; }
    static bool isHuawei()   { return get() == OEMType::EMUI; }
    static bool isVivo()     { return get() == OEMType::ORIGINOS || get() == OEMType::FUNTOUCHOS; }
};

struct PackageInfoInternal {
    std::string package_name;
    std::string installer;
    long version_code = 0;
    std::string version_name;
    bool is_detached = false;
};

#endif // ZYGISK_DETACH_COMMON_H
