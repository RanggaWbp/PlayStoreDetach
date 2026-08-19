#ifndef ZYGISK_DETACH_MIUI_COMPAT_H
#define ZYGISK_DETACH_MIUI_COMPAT_H

#include "../../include/common.h"
#include <jni.h>

class MIUICompat {
public:
    static bool apply(JNIEnv *env);
    static int getMIUIVersion();

private:
    static void applyHookToClass(JNIEnv *env, jclass cls, const std::string &name);
    static bool disableMIUIOptimization();
    static bool hookUpdateServices(JNIEnv *env, XiaomiSkin skin);
    static bool handleHyperOSSecurity(JNIEnv *env);
};

#endif
