#include "zygisk.hpp"
#include <jni.h>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <android/log.h>
#include <unistd.h>

#define LOG_TAG "BoZygisk"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Forward declarations of core subsystems
namespace bypass {
    void init_combined_bypass();
}

namespace scanner {
    bool run_auto_scan();
}

namespace egl_hook {
    bool install_hooks();
}

namespace features {
    void install_hooks();
}

namespace data {
    void thread_start();
}

class BoClientZygiskModule : public zygisk::ModuleBase {
private:
    zygisk::Api *api_{nullptr};
    JNIEnv *env_{nullptr};
    bool is_target_game_{false};
    std::string package_name_;

public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        api_ = api;
        env_ = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        if (!args || !args->nice_name) {
            api_->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        const char *nice_name = env_->GetStringUTFChars(args->nice_name, nullptr);
        if (nice_name) {
            package_name_ = nice_name;
            env_->ReleaseStringUTFChars(args->nice_name, nice_name);
        }

        // Target check: Free Fire Standard (TH), VN, & MAX
        if (package_name_.find("freefire") != std::string::npos ||
            package_name_ == "com.dts.freefireth" || 
            package_name_ == "com.dts.freefirevn" || 
            package_name_ == "com.dts.freefiremax") {
            is_target_game_ = true;
            LOGI("[+] Detected Free Fire target: %s (PID: %d)", package_name_.c_str(), getpid());
        } else {
            is_target_game_ = false;
            // Unload immediately for all non-target processes to preserve system resources
            api_->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
        }
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
        if (!is_target_game_) return;

        LOGI("[+] Post app specialize inside %s. Spawning internal payload thread...", package_name_.c_str());

        // Spawn independent worker thread inside game process
        std::thread([]() {
            LOGI("[*] Payload thread active. Waiting for game libraries to map...");
            
            // Wait for libil2cpp.so to load into memory
            bool il2cpp_ready = false;
            for (int i = 0; i < 120; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                FILE *maps = fopen("/proc/self/maps", "r");
                if (maps) {
                    char line[512];
                    while (fgets(line, sizeof(line), maps)) {
                        if (strstr(line, "libil2cpp.so")) {
                            il2cpp_ready = true;
                            break;
                        }
                    }
                    fclose(maps);
                }
                if (il2cpp_ready) break;
            }

            if (!il2cpp_ready) {
                LOGE("[-] Timeout waiting for libil2cpp.so!");
                return;
            }

            LOGI("[+] libil2cpp.so mapped! Initializing Triple-Layer Bypass...");
            bypass::init_combined_bypass();

            LOGI("[+] Initializing Dynamic IL2CPP & AOB Offset Scanner...");
            scanner::run_auto_scan();

            LOGI("[+] Installing Internal EGL & Touch Hooks for NEXORA ImGui Menu...");
            egl_hook::install_hooks();

            LOGI("[+] Installing Game Feature Hooks (Aimbot, AutoKill, Fly, Ghost, Speed, Chams)...");
            features::install_hooks();

            LOGI("[+] Starting Entity Data Worker Thread...");
            data::thread_start();

            LOGI("[✓] Bo-Client Zygisk VIP Fully Injected & Running In-Game!");
        }).detach();
    }
};

static BoClientZygiskModule g_zygisk_module;

extern "C" [[gnu::visibility("default")]] void zygisk_module_entry(zygisk::Api *api, JNIEnv *env) {
    g_zygisk_module.onLoad(api, env);
}

extern "C" [[gnu::visibility("default")]] void zygisk_pre_app_specialize(zygisk::AppSpecializeArgs *args) {
    g_zygisk_module.preAppSpecialize(args);
}

extern "C" [[gnu::visibility("default")]] void zygisk_post_app_specialize(const zygisk::AppSpecializeArgs *args) {
    g_zygisk_module.postAppSpecialize(args);
}
