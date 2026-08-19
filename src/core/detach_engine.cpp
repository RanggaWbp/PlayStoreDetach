#include "detach_engine.h"
#include "../utils/file_utils.h"

#include <sstream>
#include <cctype>

DetachEngine::DetachEngine() : initialized(false), active_layers(0),
                               android_sdk(0),
                               oem_type(OEMType::UNKNOWN) {}

DetachEngine::~DetachEngine() = default;

bool DetachEngine::initialize() {
    if (initialized) return true;

    LOGI("Initializing Detach Engine...");
    loadConfig();

    android_sdk = AndroidVersion::get();
    oem_type = OEMDetector::get();
    LOGI("Android SDK: " + std::to_string(android_sdk));
    LOGI("OEM: " + OEMDetector::getOEMName());

    initialized = true;
    return true;
}

void DetachEngine::loadConfig() {
    std::string content = FileUtils::readFile("/data/adb/detach/config.json");
    if (!content.empty()) {
        if (content.find("\"debug\": true") != std::string::npos ||
            content.find("\"debug\":true") != std::string::npos) {
            config.debug = true;
            Logger::setDebug(true);
        }
        if (content.find("\"aggressive_mode\": true") != std::string::npos ||
            content.find("\"aggressive_mode\":true") != std::string::npos) {
            config.aggressive_mode = true;
        }
    }
}

void DetachEngine::loadDetachList() {
    std::string content = FileUtils::readFile("/data/adb/detach/detach.list");
    loadDetachListFromString(content);
}

void DetachEngine::loadDetachListFromString(const std::string &content) {
    std::lock_guard<std::mutex> lock(mutex);
    detached_packages.clear();

    if (content.empty()) {
        LOGI("Detach list kosong");
        return;
    }

    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        line = StringUtils::trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (isValidPackageName(line)) {
            detached_packages.insert(line);
        } else {
            LOGE("Package name tidak valid: " + line);
        }
    }
    LOGI("Loaded " + std::to_string(detached_packages.size()) + " packages");
}

bool DetachEngine::isDetached(const std::string &package_name) const {
    std::lock_guard<std::mutex> lock(mutex);
    return detached_packages.count(package_name) > 0;
}

bool DetachEngine::isValidPackageName(const std::string &name) {
    if (name.empty() || name.size() > 256) return false;
    if (name[0] == '.' || name.back() == '.') return false;
    for (char c : name) {
        if (!isalnum((unsigned char)c) && c != '.' && c != '_') return false;
    }
    return true;
}

void DetachEngine::addPackage(const std::string &package_name) {
    std::lock_guard<std::mutex> lock(mutex);
    detached_packages.insert(package_name);
}

void DetachEngine::removePackage(const std::string &package_name) {
    std::lock_guard<std::mutex> lock(mutex);
    detached_packages.erase(package_name);
}

std::vector<std::string> DetachEngine::getDetachedPackages() const {
    std::lock_guard<std::mutex> lock(mutex);
    return std::vector<std::string>(detached_packages.begin(),
                                    detached_packages.end());
}

bool DetachEngine::shouldFilterPackage(const std::string &package_name,
                                       const std::string &caller_package) const {
    if (caller_package != "com.android.vending" &&
        caller_package != "com.google.android.gms") {
        return false;
    }
    return isDetached(package_name);
}

bool DetachEngine::shouldFilterFromList(const std::string &package_name) const {
    return isDetached(package_name);
}

void DetachEngine::setActiveLayer(HookLayer layer) {
    std::lock_guard<std::mutex> lock(mutex);
    active_layers |= static_cast<int>(layer);
    LOGI("Hook layer aktif: " + std::to_string(static_cast<int>(layer)));
}

bool DetachEngine::isLayerActive(HookLayer layer) const {
    return (active_layers & static_cast<int>(layer)) != 0;
}

int DetachEngine::getActiveLayerCount() const {
    int count = 0, layers = active_layers;
    while (layers) { count += layers & 1; layers >>= 1; }
    return count;
}
