#include "file_utils.h"

#include <fstream>
#include <sys/stat.h>

namespace FileUtils {

std::string readFile(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());
}

bool writeFile(const std::string &path, const std::string &content) {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << content;
    return true;
}

bool fileExists(const std::string &path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

} // namespace FileUtils

namespace StringUtils {

std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

} // namespace StringUtils
