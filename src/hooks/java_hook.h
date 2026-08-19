#ifndef ZYGISK_DETACH_JAVA_HOOK_H
#define ZYGISK_DETACH_JAVA_HOOK_H

#include <jni.h>
#include <string>
#include <vector>
#include <cstddef>

class JavaHook {
public:
    static bool hookMethod(JNIEnv *env,
                           const std::string &class_name,
                           const std::string &method_name,
                           const std::string &signature,
                           void *hook_function,
                           void **original_function);

    static bool replaceArtMethodEntry(JNIEnv *env, jclass cls, jmethodID method,
                                      void *new_entry, void **original_entry);

    // Kandidat offset per ART generation + verifikasi runtime
    static std::vector<size_t> getOffsetCandidates(int generation);

    // Legacy getters (kompatibilitas)
    static size_t getArtMethodEntryPointOffset_SDK24();
    static size_t getArtMethodEntryPointOffset_SDK28();
    static size_t getArtMethodEntryPointOffset_SDK31();
    static size_t getArtMethodEntryPointOffset_SDK34();
    static size_t getArtMethodEntryPointOffset_SDK36();
    static size_t getArtMethodEntryPointOffset_SDK37();
};

#endif
