#pragma once

#include <jni.h>
#include <stdint.h>

#define ZYGISK_API_VERSION 4

namespace zygisk {

enum Option : int {
    FORCE_DENYLIST_UNMOUNT = 0,
    DLCLOSE_MODULE_LIBRARY = 1,
};

enum StateFlag : uint32_t {
    PROCESS_ON_DENYLIST = 1 << 0,
    PROCESS_GRANTED_ROOT = 1 << 1,
};

struct AppSpecializeArgs {
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jobjectArray &rlimits;
    jint &mount_external;
    jstring &se_info;
    jstring &nice_name;
    jboolean &is_child_zygote;
    jstring &instruction_set;
    jstring &app_data_dir;
    jboolean &is_top_app;
    jobjectArray &pkg_data_info_list;
    jobjectArray &whitelisted_data_info_list;
    jboolean &mount_data_dirs;
    jboolean &mount_storage_dirs;
};

struct ServerSpecializeArgs {
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jlong &permitted_capabilities;
    jlong &effective_capabilities;
};

class Api {
public:
    virtual long getFlags() = 0;
    virtual void setOption(Option opt) = 0;
    virtual int getModuleDir() = 0;
    virtual void *pltHookRegister(const char *regex, const char *symbol, void *new_func, void **old_func) = 0;
    virtual void *pltHookCommit() = 0;
    virtual int connectCompanion() = 0;
    virtual void *getRemote(int id) = 0;
    virtual void setRemote(int id, void *remote) = 0;
};

class ModuleBase {
public:
    virtual void onLoad(Api *api, JNIEnv *env) {}
    virtual void preAppSpecialize(AppSpecializeArgs *args) {}
    virtual void postAppSpecialize(const AppSpecializeArgs *args) {}
    virtual void preServerSpecialize(ServerSpecializeArgs *args) {}
    virtual void postServerSpecialize(const ServerSpecializeArgs *args) {}
};

typedef void (*ModuleRegisterFunc)(Api *, JNIEnv *);
typedef void (*CompanionHandlerFunc)(int);

} // namespace zygisk

#define REGISTER_ZYGISK_MODULE(clazz)                                                               \
extern "C" [[gnu::visibility("default")]] void zygisk_module_entry(zygisk::Api *api, JNIEnv *env) { \
    static clazz module;                                                                             \
    api->setRemote(0, &module);                                                                      \
    module.onLoad(api, env);                                                                         \
}                                                                                                    \
extern "C" [[gnu::visibility("default")]] void zygisk_pre_app_specialize(zygisk::AppSpecializeArgs *args) { \
    auto *m = (clazz *)zygisk::Api::getRemote(0);                                                    \
    if (m) m->preAppSpecialize(args);                                                                \
}                                                                                                    \
extern "C" [[gnu::visibility("default")]] void zygisk_post_app_specialize(const zygisk::AppSpecializeArgs *args) { \
    auto *m = (clazz *)zygisk::Api::getRemote(0);                                                    \
    if (m) m->postAppSpecialize(args);                                                               \
}
