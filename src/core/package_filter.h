#ifndef ZYGISK_DETACH_PACKAGE_FILTER_H
#define ZYGISK_DETACH_PACKAGE_FILTER_H

#include "../../include/common.h"
#include "detach_engine.h"
#include <jni.h>

class PackageFilter {
public:
    static void init(DetachEngine *engine);
    static jobject filterPackageList(JNIEnv *env, jobject package_list);
    static std::string getPackageNameFromInfo(JNIEnv *env, jobject package_info);
};

#endif
