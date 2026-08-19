/**
 * Java Hook — ART method entry replacement.
 * v2.1: offset TIDAK hardcoded murni. Dua strategi:
 *   1. Kandidat offset per generation (24-37+)
 *   2. RUNTIME VERIFICATION — pointer di offset harus mengarah ke
 *      executable libart.so; kalau tidak, geser / brute force.
 * Inilah yang membuat hook tetap jalan di Android 16/17/18+.
 */
#include "java_hook.h"
#include "../../include/common.h"
#include "../compat/android_compat.h"

#include <dlfcn.h>
#include <cstring>

// ============================================================
// Runtime ART helpers
// ============================================================
static void *g_art_text_start = nullptr;
static void *g_art_text_end = nullptr;

static void findArtTextRange() {
    if (g_art_text_start) return;

    void *handle = dlopen("libart.so", RTLD_NOLOAD);
    if (!handle) handle = dlopen("libart.so", RTLD_LAZY);
    if (!handle) return;

    void *fn = dlsym(handle, "_ZN3art3Dbg14GetJdwpAllowedEv");
    if (fn) {
        Dl_info info;
        if (dladdr(fn, &info)) {
            g_art_text_start = info.dli_fbase;
            // Perkiraan kasar: libart ~35MB
            g_art_text_end = (char *)g_art_text_start + (35 * 1024 * 1024);
        }
    }
    dlclose(handle);
}

static bool isValidArtPointer(void *ptr) {
    findArtTextRange();
    if (!g_art_text_start || !ptr) return false;
    return ptr >= g_art_text_start && ptr < g_art_text_end;
}

// ============================================================
// API utama
// ============================================================
bool JavaHook::hookMethod(JNIEnv *env,
                          const std::string &class_name,
                          const std::string &method_name,
                          const std::string &signature,
                          void *hook_function,
                          void **original_function) {
    LOGD("Hooking: " + class_name + "." + method_name);

    jclass cls = env->FindClass(class_name.c_str());
    if (!cls) {
        env->ExceptionClear();
        LOGE("Class tidak ditemukan: " + class_name);
        return false;
    }

    jmethodID method = env->GetMethodID(cls, method_name.c_str(),
                                        signature.c_str());
    if (!method) {
        env->ExceptionClear();
        method = env->GetStaticMethodID(cls, method_name.c_str(),
                                        signature.c_str());
    }
    if (!method) {
        env->ExceptionClear();
        LOGE("Method tidak ditemukan: " + class_name + "." + method_name);
        env->DeleteLocalRef(cls);
        return false;
    }

    bool ok = replaceArtMethodEntry(env, cls, method,
                                    hook_function, original_function);
    env->DeleteLocalRef(cls);
    return ok;
}

bool JavaHook::replaceArtMethodEntry(JNIEnv *env, jclass cls, jmethodID method,
                                     void *new_entry, void **original_entry) {
    (void)env; (void)cls;

    AndroidCapabilities caps = AndroidVersion::getCapabilities();
    int generation = caps.art_layout_generation;

    std::vector<size_t> candidates = getOffsetCandidates(generation);
    void *art_method = (void *)method;  // jmethodID == ArtMethod*

    LOGD("ART gen " + std::to_string(generation) + ", kandidat: " +
         std::to_string(candidates.size()));

    // --- Strategi 1: kandidat offset + verifikasi runtime ---
    for (size_t offset : candidates) {
        void *current = *(void **)((char *)art_method + offset);
        if (isValidArtPointer(current)) {
            if (original_entry) *original_entry = current;
            *(void **)((char *)art_method + offset) = new_entry;
            __builtin___clear_cache((char *)art_method,
                                    (char *)art_method + 64);
            LOGD("Entry diganti di offset " + std::to_string(offset) +
                 " (terverifikasi runtime)");
            return true;
        }
    }

    // --- Strategi 2: brute force scan (darurat) ---
    LOGD("Kandidat gagal, brute force scan...");
    for (size_t offset = 8; offset <= 64; offset += 4) {
        void *current = *(void **)((char *)art_method + offset);
        if (isValidArtPointer(current)) {
            if (original_entry) *original_entry = current;
            *(void **)((char *)art_method + offset) = new_entry;
            __builtin___clear_cache((char *)art_method,
                                    (char *)art_method + 64);
            LOGI("Brute force berhasil di offset " + std::to_string(offset));
            return true;
        }
    }

    LOGE("Semua strategi offset gagal untuk SDK " +
         std::to_string(AndroidVersion::get()));
    return false;
}

std::vector<size_t> JavaHook::getOffsetCandidates(int generation) {
    std::vector<size_t> candidates;

#if defined(__aarch64__) || defined(__x86_64__)
    // ============ 64-BIT ============
    switch (generation) {
        case 5:  candidates = { 24, 32, 28, 16, 40 }; break; // Android 14-17+
        case 4:  candidates = { 24, 32, 28, 16 };     break; // Android 12-13
        case 3:  candidates = { 24, 32, 16, 28 };     break; // Android 9-11
        case 2:  candidates = { 24, 32, 16, 28 };     break; // Android 8
        case 1:  candidates = { 28, 32, 24, 16 };     break; // Android 7
        default: candidates = { 24, 32, 28, 16, 40, 36, 20, 44 }; break;
    }
#else
    // ============ 32-BIT ============
    switch (generation) {
        case 5:  candidates = { 16, 20, 12, 24 };     break;
        case 4:  candidates = { 16, 20, 12, 24 };     break;
        case 3:  candidates = { 16, 20, 12 };         break;
        case 2:  candidates = { 16, 20, 12 };         break;
        case 1:  candidates = { 20, 16, 12 };         break;
        default: candidates = { 16, 20, 12, 24, 28, 8 }; break;
    }
#endif
    return candidates;
}

// Legacy getters — delegasi ke getOffsetCandidates
size_t JavaHook::getArtMethodEntryPointOffset_SDK24() { return getOffsetCandidates(1)[0]; }
size_t JavaHook::getArtMethodEntryPointOffset_SDK28() { return getOffsetCandidates(3)[0]; }
size_t JavaHook::getArtMethodEntryPointOffset_SDK31() { return getOffsetCandidates(4)[0]; }
size_t JavaHook::getArtMethodEntryPointOffset_SDK34() { return getOffsetCandidates(5)[0]; }
size_t JavaHook::getArtMethodEntryPointOffset_SDK36() { return getOffsetCandidates(5)[0]; }
size_t JavaHook::getArtMethodEntryPointOffset_SDK37() { return getOffsetCandidates(5)[0]; }
