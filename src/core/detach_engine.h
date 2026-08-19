#ifndef ZYGISK_DETACH_ENGINE_H
#define ZYGISK_DETACH_ENGINE_H

#include "../../include/common.h"

class DetachEngine {
public:
    DetachEngine();
    ~DetachEngine();

    bool initialize();
    void loadConfig();
    void loadDetachList();
    // Dipakai saat daftar diterima via IPC companion (proses app
    // tidak punya akses baca langsung ke /data/adb)
    void loadDetachListFromString(const std::string &content);

    bool isDetached(const std::string &package_name) const;
    bool isValidPackageName(const std::string &name);

    void addPackage(const std::string &package_name);
    void removePackage(const std::string &package_name);
    std::vector<std::string> getDetachedPackages() const;

    bool shouldFilterPackage(const std::string &package_name,
                             const std::string &caller_package) const;
    bool shouldFilterFromList(const std::string &package_name) const;

    void setActiveLayer(HookLayer layer);
    bool isLayerActive(HookLayer layer) const;
    int getActiveLayerCount() const;

    DetachConfig config;

private:
    bool initialized;
    int active_layers;
    mutable std::mutex mutex;
    std::set<std::string> detached_packages;
    int android_sdk;
    OEMType oem_type;
};

#endif
