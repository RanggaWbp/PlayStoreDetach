#include "package_filter.h"

static DetachEngine *g_filter_engine = nullptr;

void PackageFilter::init(DetachEngine *engine) {
    g_filter_engine = engine;
}

std::string PackageFilter::getPackageNameFromInfo(JNIEnv *env, jobject package_info) {
    if (!package_info) return "";

    jclass pi_class = env->GetObjectClass(package_info);
    jfieldID field = env->GetFieldID(pi_class, "packageName", "Ljava/lang/String;");
    if (!field) {
        env->ExceptionClear();
        env->DeleteLocalRef(pi_class);
        return "";
    }

    jstring jname = (jstring)env->GetObjectField(package_info, field);
    if (!jname) {
        env->DeleteLocalRef(pi_class);
        return "";
    }

    const char *chars = env->GetStringUTFChars(jname, nullptr);
    std::string result(chars ? chars : "");
    env->ReleaseStringUTFChars(jname, chars);
    env->DeleteLocalRef(jname);
    env->DeleteLocalRef(pi_class);
    return result;
}

jobject PackageFilter::filterPackageList(JNIEnv *env, jobject package_list) {
    if (!package_list || !env) return package_list;

    jclass list_class = env->GetObjectClass(package_list);
    jmethodID size_m = env->GetMethodID(list_class, "size", "()I");
    jmethodID get_m  = env->GetMethodID(list_class, "get", "(I)Ljava/lang/Object;");
    env->DeleteLocalRef(list_class);
    if (!size_m || !get_m) {
        env->ExceptionClear();
        return package_list;
    }

    jclass al_class = env->FindClass("java/util/ArrayList");
    jmethodID ctor  = env->GetMethodID(al_class, "<init>", "()V");
    jmethodID add_m = env->GetMethodID(al_class, "add", "(Ljava/lang/Object;)Z");
    jobject filtered = env->NewObject(al_class, ctor);
    env->DeleteLocalRef(al_class);

    jint size = env->CallIntMethod(package_list, size_m);
    jint kept = 0;

    for (jint i = 0; i < size; i++) {
        jobject info = env->CallObjectMethod(package_list, get_m, i);
        if (env->ExceptionCheck()) { env->ExceptionClear(); continue; }
        if (!info) continue;

        std::string name = getPackageNameFromInfo(env, info);
        if (!name.empty() && g_filter_engine && g_filter_engine->isDetached(name)) {
            LOGD("Filtered: " + name);  // skip — jangan masukkan
        } else {
            env->CallBooleanMethod(filtered, add_m, info);
            kept++;
        }
        env->DeleteLocalRef(info);
    }

    LOGI("Filtered " + std::to_string(size - kept) + " packages dari list");
    return filtered;
}
