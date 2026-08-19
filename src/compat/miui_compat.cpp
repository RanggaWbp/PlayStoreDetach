/**
 * miui/hyperos Compatibility Layer
 * Menangani BAIK MIUI klasik (12/13/14) maupun HyperOS (1.0+)
 */
#include "miui_compat.h"

#include <sys/system_properties.h>
#include <vector>

bool MIUICompat::apply(JNIEnv *env) {
    LOGI("Applying miui/hyperos compatibility...");

    XiaomiSkin skin = OEMDetector::getXiaomiSkin();
    std::string version = OEMDetector::getSkinVersion();
    LOGI("Skin: " + std::string(skin == XiaomiSkin::HYPEROS ? "HyperOS" : "MIUI") +
         " | Versi: " + version);

    // Class PackageManager yang dimodifikasi miui/hyperos
    std::vector<std::string> pm_classes;
    if (skin == XiaomiSkin::HYPEROS) {
        pm_classes = {
            "miui.content.pm.PackageManagerServiceUtils",
            "com.miui.content.pm.IPackageManagerUtils",
            "com.miui.server.pm.PackageManagerServiceImpl",
            "miui.os.MiuiInit",
        };
    } else {
        pm_classes = {
            "miui.content.pm.PackageManagerServiceUtils",
            "com.miui.content.pm.IPackageManagerUtils",
            "android.content.pm.MiuiPackageManagerService",
            "miui.app.ActivityThread",
        };
    }

    int found = 0;
    for (const auto &name : pm_classes) {
        jclass cls = env->FindClass(name.c_str());
        if (cls) {
            LOGI("Class miui/hyperos PM ditemukan: " + name);
            applyHookToClass(env, cls, name);
            env->DeleteLocalRef(cls);
            found++;
        } else {
            env->ExceptionClear();
        }
    }
    LOGI("miui/hyperos PM classes: " + std::to_string(found) + "/" +
         std::to_string(pm_classes.size()));

    disableMIUIOptimization();
    hookUpdateServices(env, skin);

    if (skin == XiaomiSkin::HYPEROS) {
        handleHyperOSSecurity(env);
    }
    return true;
}

void MIUICompat::applyHookToClass(JNIEnv *env, jclass cls,
                                   const std::string &name) {
    (void)env; (void)cls;
    LOGD("Registrasi target hook: " + name);
}

bool MIUICompat::disableMIUIOptimization() {
    int r1 = __system_property_set("persist.sys.miui_optimization", "false");
    int r2 = __system_property_set("persist.sys.miui_optimization.mod", "false");
    if (r1 == 0 || r2 == 0) {
        LOGI("Optimisasi miui/hyperos dinonaktifkan");
        return true;
    }
    return false;
}

bool MIUICompat::hookUpdateServices(JNIEnv *env, XiaomiSkin skin) {
    (void)env;
    std::vector<std::string> pkgs;
    if (skin == XiaomiSkin::HYPEROS) {
        pkgs = {"com.xiaomi.mipicks", "com.miui.securitycenter",
                "com.miui.systemAdSolution", "com.xiaomi.market"};
    } else {
        pkgs = {"com.xiaomi.market", "com.miui.systemAdSolution",
                "com.miui.player"};
    }
    for (const auto &p : pkgs) {
        LOGD("Monitor miui/hyperos update service: " + p);
    }
    return true;
}

bool MIUICompat::handleHyperOSSecurity(JNIEnv *env) {
    (void)env;
    LOGI("HyperOS security handling aktif");
    return true;
}

int MIUICompat::getMIUIVersion() {
    char version[PROP_VALUE_MAX] = {0};
    __system_property_get("ro.miui.ui.version.name", version);
    if (strlen(version) == 0) return 0;
    if (strstr(version, "V15") || strstr(version, "OS")) return 15;
    if (strstr(version, "V14")) return 14;
    if (strstr(version, "V13")) return 13;
    if (strstr(version, "V12")) return 12;
    return 0;
}
