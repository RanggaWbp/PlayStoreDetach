#ifndef ZYGISK_DETACH_FILE_UTILS_H
#define ZYGISK_DETACH_FILE_UTILS_H

#include <string>

namespace FileUtils {
    std::string readFile(const std::string &path);
    bool writeFile(const std::string &path, const std::string &content);
    bool fileExists(const std::string &path);
}

namespace StringUtils {
    std::string trim(const std::string &s);
}

#endif
