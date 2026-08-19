#include "../../include/common.h"

#include <ctime>
#include <fstream>

std::atomic<bool> Logger::debug_enabled{false};
std::string Logger::log_buffer;
std::mutex Logger::log_mutex;

static std::string timestamp() {
    std::time_t now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&now));
    return buf;
}

void Logger::log(const std::string &tag, const std::string &message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    __android_log_print(ANDROID_LOG_INFO, tag.c_str(), "%s", message.c_str());
    log_buffer += "[" + timestamp() + "] [" + tag + "] " + message + "\n";
    if (log_buffer.size() > 100 * 1024) {
        log_buffer = log_buffer.substr(log_buffer.size() - 50 * 1024);
    }
}

void Logger::error(const std::string &tag, const std::string &message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    __android_log_print(ANDROID_LOG_ERROR, tag.c_str(), "%s", message.c_str());
    log_buffer += "[" + timestamp() + "] [" + tag + "] ERROR: " + message + "\n";
}

void Logger::debug(const std::string &tag, const std::string &message) {
    if (!debug_enabled.load()) return;
    std::lock_guard<std::mutex> lock(log_mutex);
    __android_log_print(ANDROID_LOG_DEBUG, tag.c_str(), "%s", message.c_str());
    log_buffer += "[" + timestamp() + "] [" + tag + "] DEBUG: " + message + "\n";
}

void Logger::flushToFile(const std::string &path) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::ofstream file(path, std::ios::app);
    if (file.is_open()) {
        file << log_buffer;
        file.close();
        log_buffer.clear();
    }
}
