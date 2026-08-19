/**
 * Zygisk API v4 — mengikuti pola API resmi Magisk/ReZygisk.
 * Catatan: untuk build produksi, bisa juga swap dengan header
 * resmi dari repo Magisk (native/src/zygisk/include/zygisk.hpp) —
 * struktur dan makro identik.
 */
#ifndef ZYGISK_H
#define ZYGISK_H

#include <jni.h>
#include <stdint.h>

#define ZYGISK_API_VERSION 4

namespace zygisk {

struct Api;
struct AppSpecializeArgs;
struct ServerSpecializeArgs;

enum class Option : int {
    FORCE_DENYLIST_UNMOUNT = 0,
    DLCLOSE_MODULE_LIBRARY = 1,
};

struct AppSpecializeArgs {
    // Argumen wajib (pointer tidak null)
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jintArray &rlimits;
    jint &mount_external;
    jstring &se_info;
    jstring &nice_name;
    jstring &instruction_set;
    jstring &app_data_dir;

    // Argumen opsional (bisa null)
    jstring *app_dir = nullptr;
    jstring *split_se_info = nullptr;
    jboolean *is_top_app = nullptr;
    jlongArray *pkg_data_info_list = nullptr;
    jlongArray *isolated_mount_namespace = nullptr;
    jboolean *has_forked = nullptr;
    jboolean *is_child_zygote = nullptr;
    jboolean *usap_table_enabled = nullptr;

    AppSpecializeArgs() = delete;
};

struct ServerSpecializeArgs {
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jlongArray &rlimits;
    jint &mount_external;
    jstring &se_info;
    jstring &nice_name;
    jboolean &start_child_zygote;
    jboolean &usap_table_enabled;

    ServerSpecializeArgs() = delete;
};

class ModuleBase {
public:
    virtual void onLoad(Api *, JNIEnv *) {}
    virtual void preAppSpecialize(AppSpecializeArgs *) {}
    virtual void postAppSpecialize(const AppSpecializeArgs *) {}
    virtual void preServerSpecialize(ServerSpecializeArgs *) {}
    virtual void postServerSpecialize(const ServerSpecializeArgs *) {}
protected:
    virtual ~ModuleBase() = default;
};

namespace internal {

struct api_table {
    void *impl;
    bool (*registerModule)(api_table *, long *);
    void (*hookJniNativeMethods)(const char *, JNINativeMethod *, int);
    void (*pltHookRegister)(const char *, const char *, void *, void **);
    void (*pltHookExclude)(const char *);
    bool (*pltHookCommit)();
    void (*connectCompanion)(void *, int);
    void (*setOption)(void *, Option);
    int (*getModuleDir)(void *);
    uint32_t (*getFlags)(void *);
};

// Struktur ABI yang diserahkan ke framework Zygisk saat registrasi
struct module_abi {
    long api_version;
    ModuleBase *impl;
    void (*preAppSpecialize)(ModuleBase *, AppSpecializeArgs *);
    void (*postAppSpecialize)(ModuleBase *, const AppSpecializeArgs *);
    void (*preServerSpecialize)(ModuleBase *, ServerSpecializeArgs *);
    void (*postServerSpecialize)(ModuleBase *, const ServerSpecializeArgs *);
};

// Thunk: C function pointer → virtual method
template<class T>
void preAppThunk(ModuleBase *base, AppSpecializeArgs *args) {
    static_cast<T *>(base)->preAppSpecialize(args);
}
template<class T>
void postAppThunk(ModuleBase *base, const AppSpecializeArgs *args) {
    static_cast<T *>(base)->postAppSpecialize(args);
}
template<class T>
void preServerThunk(ModuleBase *base, ServerSpecializeArgs *args) {
    static_cast<T *>(base)->preServerSpecialize(args);
}
template<class T>
void postServerThunk(ModuleBase *base, const ServerSpecializeArgs *args) {
    static_cast<T *>(base)->postServerSpecialize(args);
}

} // namespace internal

struct Api {
    internal::api_table *impl;

    void connectCompanion(int fd)  { impl->connectCompanion(impl->impl, fd); }
    void setOption(Option opt)     { impl->setOption(impl->impl, opt); }
    int getModuleDir()             { return impl->getModuleDir(impl->impl); }
    uint32_t getFlags()            { return impl->getFlags(impl->impl); }

    void hookJniNativeMethods(const char *className,
                              JNINativeMethod *methods,
                              int numMethods) {
        impl->hookJniNativeMethods(className, methods, numMethods);
    }
    void pltHookRegister(const char *regex, const char *symbol,
                         void *fn, void **backup) {
        impl->pltHookRegister(regex, symbol, fn, backup);
    }
    void pltHookExclude(const char *regex) { impl->pltHookExclude(regex); }
    bool pltHookCommit() { return impl->pltHookCommit(); }
};

namespace internal {

template<class T>
void register_module(api_table *table, JNIEnv *env) {
    static T module;
    static module_abi abi {
        ZYGISK_API_VERSION,
        &module,
        &preAppThunk<T>,
        &postAppThunk<T>,
        &preServerThunk<T>,
        &postServerThunk<T>
    };
    Api api{table};
    module.onLoad(&api, env);
    table->registerModule(table, reinterpret_cast<long *>(&abi));
}

} // namespace internal

} // namespace zygisk

#define REGISTER_ZYGISK_MODULE(mod)                                    \
    extern "C" [[gnu::visibility("default")]]                          \
    void zygisk_module_entry(zygisk::internal::api_table *table,       \
                             JNIEnv *env) {                            \
        zygisk::internal::register_module<mod>(table, env);            \
    }

#define REGISTER_ZYGISK_COMPANION(func)                                \
    extern "C" [[gnu::visibility("default")]]                          \
    void zygisk_companion_entry(int fd) { func(fd); }

#endif // ZYGISK_H
