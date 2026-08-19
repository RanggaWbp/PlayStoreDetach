#ifndef ZYGISK_DETACH_ANDROID_COMPAT_H
#define ZYGISK_DETACH_ANDROID_COMPAT_H

#include "../../include/common.h"

class AndroidCompat {
public:
    static std::string getPackageManagerClassName();
    static std::string getIPackageManagerDescriptor();
    static std::string getTransactMethodName();
    static int getPackageManagerTransactionCode(const std::string &method);
    // Resolve transaction code via reflection (bukan hardcode) —
    // kunci future-proof untuk Android 16/17/18+
    static int resolveTransactionCodeRuntime(JNIEnv *env,
                                              const std::string &method_name);
    static bool isSupportedAndroidVersion();
    static std::string getFullCompatReport();
};

#endif
