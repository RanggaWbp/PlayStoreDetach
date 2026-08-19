#ifndef ZYGISK_DETACH_BINDER_HOOK_H
#define ZYGISK_DETACH_BINDER_HOOK_H

#include "../../include/common.h"
#include "../core/detach_engine.h"
#include <jni.h>
#include <cstdint>
#include <cstddef>

class BinderHook {
public:
    static void setEngine(DetachEngine *engine);
    static bool filterPackageListParcel(void *parcel_data, size_t parcel_size);
    static bool filterPackageInfoParcel(void *parcel_data, size_t parcel_size,
                                        const std::string &package_name);
    static int onTransact(uint32_t code, const void *data,
                          void *reply, uint32_t flags);
    static bool hookJavaBinderTransact(JNIEnv *env);
};

#endif
