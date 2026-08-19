/**
 * ============================================================
 * PlayStoreDetach v1.0 — Main Entry
 * Android 7 ~ 17+ | miui/hyperos, OneUI, ColorOS, dll.
 * ============================================================
 */
#include "../include/common.h"
#include "../include/zygisk.h"
#include "core/detach_engine.h"
#include "core/package_filter.h"
#include "hooks/hook_manager.h"
#include "compat/android_compat.h"
#include "compat/miui_compat.h"

#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

// ============================================================
// Globals
// ============================================================
JNIEnv *g_env = nullptr;

static DetachEngine *g_engine = nullptr;
static HookManager *g_hook_manager = nullptr;

static const char *PLAY_STORE_PKG = "com.android.vending";
static const char *GMS_PKG = "com.google.android.gms";

// ============================================================
// Module
// ============================================================
class DetachModule : public zygisk::ModuleBase {
    zygisk::Api *api = nullptr;

public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        this->api = api;
        g_env = env;
        LOGI("Module loaded");
        LOGI("Android: " + AndroidVersion::getVersionString());
        LOGI("OEM: " + OEMDetector::getOEMName());
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        JNIEnv *env = g_env;
        if (!env || !args) return;

        const char *nice = env->GetStringUTFChars(args->nice_name, nullptr);
        std::string proc = nice ? nice : "";
        if (nice) env->ReleaseStringUTFChars(args->nice_name, nice);

        std::string pkg;
        if (args->app_data_dir) {
            const char *dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
            if (dir) {
                std::string d(dir);
                env->ReleaseStringUTFChars(args->app_data_dir, dir);
                // /data/user/0/<pkg> → ekstrak
                size_t pos = d.find("/0/");
                if (pos != std::string::npos) {
                    pkg = d.substr(pos + 3);
                }
            }
        }

        if (pkg == PLAY_STORE_PKG || proc.find(PLAY_STORE_PKG) == 0) {
            LOGI("Play Store process: " + proc);
            handlePlayStore();
        } else if (pkg == GMS_PKG || proc.find(GMS_PKG) == 0) {
            LOGI("Play Services process: " + proc);
            initEngineOnly();
        }
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *) override {
        if (g_hook_manager) g_hook_manager->onPostFork();
    }

private:
    void initEngineOnly() {
        if (!g_engine) {
            g_engine = new DetachEngine();
            g_engine->initialize();
        }
    }

    void handlePlayStore() {
        initEngineOnly();

        // Ambil detach list via companion (root) — proses app
        // tidak bisa baca /data/adb/detach langsung
        fetchDetachListViaCompanion();

        if (!g_hook_manager) {
            g_hook_manager = new HookManager(g_engine);
            g_hook_manager->initialize();
        }

        applyHookLayers();
    }

    void fetchDetachListViaCompanion() {
        if (!api) return;

        int fds[2];
        if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds) != 0) return;

        api->connectCompanion(fds[1]);

        write(fds[0], "get_detach_list", 15);

        char buf[16384] = {0};
        ssize_t n = read(fds[0], buf, sizeof(buf) - 1);
        close(fds[0]);
        close(fds[1]);

        if (n > 0) {
            g_engine->loadDetachListFromString(std::string(buf, n));
        } else {
            LOGE("Gagal ambil detach list dari companion");
        }
    }

    // ========================================================
    // Layer selection — berbasis CAPABILITY (Android 7-17+)
    // ========================================================
    void applyHookLayers() {
        LOGI("Applying hook layers...");
        LOGI(AndroidCompat::getFullCompatReport());

        AndroidCapabilities caps = AndroidVersion::getCapabilities();

        if (caps.has_aidl_package_manager) {
            // Android 12 ~ 17+ : jalur AIDL
            LOGI("Jalur modern (AIDL)");
            if (!g_hook_manager->applyBinderHook()) {
                LOGE("Binder hook gagal → fallback Java hook");
                g_hook_manager->applyJavaHook();
            }
        } else {
            // Android 7-11 : jalur legacy
            LOGI("Jalur legacy (pre-AIDL)");
            if (!g_hook_manager->applyJavaHook()) {
                LOGE("Java hook gagal → fallback Binder hook");
                g_hook_manager->applyBinderHook();
            }
        }

        g_hook_manager->applyProviderHook();
        applyOEMSpecificHooks();

        if (caps.uses_flag_permutations) {
            g_hook_manager->applyFlagPermutationHook();
        }
        if (caps.has_archived_apps) {
            g_hook_manager->applyArchivedAppHook();
        }
    }

    void applyOEMSpecificHooks() {
        switch (OEMDetector::get()) {
            case OEMType::MIUI_HYPEROS:
                LOGI("Applying miui/hyperos compat (skin: " +
                     std::string(OEMDetector::getXiaomiSkin() ==
                                 XiaomiSkin::HYPEROS ? "HyperOS" : "MIUI") + ")");
                g_hook_manager->applyMIUIHyperOSCompat();
                break;
            case OEMType::ONEUI:
                g_hook_manager->applyOneUICompat();
                break;
            case OEMType::COLOROS:
            case OEMType::REALME_UI:
            case OEMType::OXYGENOS:
                g_hook_manager->applyOPPOCompat();
                break;
            case OEMType::ORIGINOS:
            case OEMType::FUNTOUCHOS:
                g_hook_manager->applyOPPOCompat();
                break;
            case OEMType::EMUI:
            case OEMType::MAGICOS:
                g_hook_manager->applyEMUICompat();
                break;
            default:
                LOGI("Tidak perlu hook OEM-specific");
                break;
        }
    }
};

REGISTER_ZYGISK_MODULE(DetachModule)

// ============================================================
// Companion — berjalan sebagai ROOT, melayani IPC
// ============================================================
static void companion_handler(int fd) {
    char command[256] = {0};
    read(fd, command, sizeof(command) - 1);
    std::string cmd(command);

    if (cmd == "get_detach_list") {
        FILE *f = fopen("/data/adb/detach/detach.list", "r");
        if (f) {
            char buffer[16384] = {0};
            size_t bytes = fread(buffer, 1, sizeof(buffer) - 1, f);
            fclose(f);
            if (bytes > 0) write(fd, buffer, bytes);
        }
    } else if (cmd == "get_config") {
        FILE *f = fopen("/data/adb/detach/config.json", "r");
        if (f) {
            char buffer[4096] = {0};
            size_t bytes = fread(buffer, 1, sizeof(buffer) - 1, f);
            fclose(f);
            if (bytes > 0) write(fd, buffer, bytes);
        }
    }
}

REGISTER_ZYGISK_COMPANION(companion_handler)
