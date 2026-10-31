#include "scanner.hpp"
#include "Offsets.h"
#include <dlfcn.h>
#include <link.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <vector>
#include <android/log.h>

#define LOG_TAG "BoScanner"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace scanner {

RuntimeOffsets g_Offsets;

// IL2CPP Types & C API Prototypes
typedef void* (*il2cpp_domain_get_t)();
typedef void** (*il2cpp_domain_get_assemblies_t)(const void* domain, size_t* size);
typedef void* (*il2cpp_assembly_get_image_t)(const void* assembly);
typedef const char* (*il2cpp_image_get_name_t)(const void* image);
typedef void* (*il2cpp_class_from_name_t)(const void* image, const char* namespaze, const char* name);
typedef void* (*il2cpp_class_get_field_from_name_t)(void* klass, const char* name);
typedef size_t (*il2cpp_field_get_offset_t)(void* field);
typedef void* (*il2cpp_class_get_method_from_name_t)(void* klass, const char* name, int argsCount);
typedef void* (*il2cpp_class_get_methods_t)(void* klass, void** iter);
typedef const char* (*il2cpp_method_get_name_t)(void* method);
typedef void (*il2cpp_field_static_get_value_t)(void* field, void* value);

static il2cpp_domain_get_t                  il2cpp_domain_get = nullptr;
static il2cpp_domain_get_assemblies_t       il2cpp_domain_get_assemblies = nullptr;
static il2cpp_assembly_get_image_t          il2cpp_assembly_get_image = nullptr;
static il2cpp_image_get_name_t              il2cpp_image_get_name = nullptr;
static il2cpp_class_from_name_t             il2cpp_class_from_name = nullptr;
static il2cpp_class_get_field_from_name_t   il2cpp_class_get_field_from_name = nullptr;
static il2cpp_field_get_offset_t            il2cpp_field_get_offset = nullptr;
static il2cpp_class_get_method_from_name_t  il2cpp_class_get_method_from_name = nullptr;
static il2cpp_class_get_methods_t           il2cpp_class_get_methods = nullptr;
static il2cpp_method_get_name_t             il2cpp_method_get_name = nullptr;
static il2cpp_field_static_get_value_t      il2cpp_field_static_get_value = nullptr;

uintptr_t get_module_base(const char* module_name) {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[512];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, module_name)) {
            unsigned long start = 0;
            if (sscanf(line, "%lx-", &start) == 1) {
                base = (uintptr_t)start;
                break;
            }
        }
    }
    fclose(fp);
    return base;
}

uintptr_t pattern_scan(uintptr_t base, size_t size, const char* pattern, const char* mask) {
    size_t pattern_len = strlen(mask);
    const uint8_t* p_base = (const uint8_t*)base;
    for (size_t i = 0; i < size - pattern_len; ++i) {
        bool found = true;
        for (size_t j = 0; j < pattern_len; ++j) {
            if (mask[j] != '?' && pattern[j] != (char)p_base[i + j]) {
                found = false;
                break;
            }
        }
        if (found) {
            return base + i;
        }
    }
    return 0;
}

uintptr_t pattern_scan_module(const char* module_name, const char* pattern, const char* mask) {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[512];
    uintptr_t res = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, module_name) && strstr(line, "r-xp")) {
            unsigned long start = 0, end = 0;
            if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
                res = pattern_scan(start, end - start, pattern, mask);
                if (res) break;
            }
        }
    }
    fclose(fp);
    return res;
}

static bool resolve_il2cpp_exports(void* handle) {
    il2cpp_domain_get = (il2cpp_domain_get_t)dlsym(handle, "il2cpp_domain_get");
    il2cpp_domain_get_assemblies = (il2cpp_domain_get_assemblies_t)dlsym(handle, "il2cpp_domain_get_assemblies");
    il2cpp_assembly_get_image = (il2cpp_assembly_get_image_t)dlsym(handle, "il2cpp_assembly_get_image");
    il2cpp_image_get_name = (il2cpp_image_get_name_t)dlsym(handle, "il2cpp_image_get_name");
    il2cpp_class_from_name = (il2cpp_class_from_name_t)dlsym(handle, "il2cpp_class_from_name");
    il2cpp_class_get_field_from_name = (il2cpp_class_get_field_from_name_t)dlsym(handle, "il2cpp_class_get_field_from_name");
    il2cpp_field_get_offset = (il2cpp_field_get_offset_t)dlsym(handle, "il2cpp_field_get_offset");
    il2cpp_class_get_method_from_name = (il2cpp_class_get_method_from_name_t)dlsym(handle, "il2cpp_class_get_method_from_name");
    il2cpp_class_get_methods = (il2cpp_class_get_methods_t)dlsym(handle, "il2cpp_class_get_methods");
    il2cpp_method_get_name = (il2cpp_method_get_name_t)dlsym(handle, "il2cpp_method_get_name");
    il2cpp_field_static_get_value = (il2cpp_field_static_get_value_t)dlsym(handle, "il2cpp_field_static_get_value");

    return (il2cpp_domain_get && il2cpp_class_from_name);
}

static void* find_image(const char* target_image_name) {
    if (!il2cpp_domain_get || !il2cpp_domain_get_assemblies) return nullptr;
    void* domain = il2cpp_domain_get();
    if (!domain) return nullptr;
    size_t count = 0;
    void** assemblies = il2cpp_domain_get_assemblies(domain, &count);
    if (!assemblies || count == 0) return nullptr;
    for (size_t i = 0; i < count; ++i) {
        void* img = il2cpp_assembly_get_image(assemblies[i]);
        if (!img) continue;
        const char* name = il2cpp_image_get_name(img);
        if (name && strstr(name, target_image_name)) {
            return img;
        }
    }
    return nullptr;
}

bool run_auto_scan() {
    LOGI("[*] Starting Automatic Offset & Symbol Scan...");

    // Initialize defaults from Offsets.h
    g_Offsets.il2cpp_base = get_module_base("libil2cpp.so");
    if (!g_Offsets.il2cpp_base) {
        LOGE("[-] Failed to locate libil2cpp.so base!");
        return false;
    }
    LOGI("[+] libil2cpp.so base: 0x%lx", (unsigned long)g_Offsets.il2cpp_base);

    // Apply baseline fallbacks
    g_Offsets.GameFacade = Offsets::GameFacade;
    g_Offsets.Player_HP = Offsets::Player_HP;
    g_Offsets.Player_Armor = Offsets::Player_Armor;
    g_Offsets.Player_IsDead = Offsets::Player_IsDead;
    g_Offsets.Player_Avatar = Offsets::Player_Avatar;
    g_Offsets.Player_HeadTF = Offsets::Player_HeadTF;
    g_Offsets.Player_FootTF = Offsets::Player_FootTF;
    g_Offsets.Player_Camera = Offsets::Player_Camera;
    g_Offsets.Player_Weapon = Offsets::Player_Weapon;
    g_Offsets.Player_AimInfo = Offsets::Player_AimInfo;
    g_Offsets.Player_Velocity = Offsets::Player_Velocity;
    g_Offsets.Player_IsGrounded = Offsets::Player_IsGrounded;
    g_Offsets.Player_Rotation = Offsets::Player_Rotation;
    g_Offsets.Player_Name = Offsets::Player_Name;

    g_Offsets.fn_TakeDamage_9 = (void*)(g_Offsets.il2cpp_base + Offsets::Player_TakeDamage_9);
    g_Offsets.fn_TakeDamage_4 = (void*)(g_Offsets.il2cpp_base + Offsets::Player_TakeDamage_4);
    g_Offsets.fn_get_HeadCollider = (void*)(g_Offsets.il2cpp_base + Offsets::Player_get_HeadCollider);
    g_Offsets.fn_GetHeadTF = (void*)(g_Offsets.il2cpp_base + Offsets::Player_GetHeadTF);
    g_Offsets.fn_GetHipTF = (void*)(g_Offsets.il2cpp_base + Offsets::Player_GetHipTF);
    g_Offsets.fn_UpdateAiming = (void*)(g_Offsets.il2cpp_base + Offsets::Player_UpdateAiming);
    g_Offsets.fn_StartFiring = (void*)(g_Offsets.il2cpp_base + Offsets::Player_StartFiring);

    // Dynamic IL2CPP Walk
    void* handle = dlopen("libil2cpp.so", RTLD_NOLOAD);
    if (!handle) handle = dlopen("libil2cpp.so", RTLD_NOW);

    if (handle && resolve_il2cpp_exports(handle)) {
        LOGI("[+] IL2CPP C API exports resolved successfully! Resolving runtime classes...");

        void* img_csharp = find_image("Assembly-CSharp");
        void* img_core = find_image("UnityEngine.CoreModule");
        if (!img_core) img_core = find_image("UnityEngine");

        if (img_csharp) {
            LOGI("[+] Assembly-CSharp image located: %p", img_csharp);
            
            // Player class
            void* playerClass = il2cpp_class_from_name(img_csharp, "", "Player");
            if (!playerClass) playerClass = il2cpp_class_from_name(img_csharp, "Garena", "Player");
            if (playerClass) {
                LOGI("[+] Located Player class: %p", playerClass);

                // Methods
                void* m_td9 = il2cpp_class_get_method_from_name(playerClass, "TakeDamage", 9);
                if (m_td9) g_Offsets.fn_TakeDamage_9 = *(void**)m_td9;

                void* m_td4 = il2cpp_class_get_method_from_name(playerClass, "TakeDamage", 4);
                if (m_td4) g_Offsets.fn_TakeDamage_4 = *(void**)m_td4;

                void* m_hc = il2cpp_class_get_method_from_name(playerClass, "get_HeadCollider", 0);
                if (m_hc) g_Offsets.fn_get_HeadCollider = *(void**)m_hc;

                void* m_htf = il2cpp_class_get_method_from_name(playerClass, "GetHeadTF", 0);
                if (m_htf) g_Offsets.fn_GetHeadTF = *(void**)m_htf;

                void* m_aim = il2cpp_class_get_method_from_name(playerClass, "UpdateAiming", 1);
                if (m_aim) g_Offsets.fn_UpdateAiming = *(void**)m_aim;

                void* m_fire = il2cpp_class_get_method_from_name(playerClass, "StartFiring", 0);
                if (m_fire) g_Offsets.fn_StartFiring = *(void**)m_fire;

                LOGI("[+] Dynamic Player Methods -> TakeDamage: %p, HeadCollider: %p, HeadTF: %p",
                     g_Offsets.fn_TakeDamage_9, g_Offsets.fn_get_HeadCollider, g_Offsets.fn_GetHeadTF);
            }

            // GameFacade class
            void* facadeClass = il2cpp_class_from_name(img_csharp, "", "GameFacade");
            if (facadeClass) {
                void* f_inst = il2cpp_class_get_field_from_name(facadeClass, "s_instance");
                if (!f_inst) f_inst = il2cpp_class_get_field_from_name(facadeClass, "Instance");
                if (f_inst) {
                    LOGI("[+] Dynamic GameFacade static instance field found: %p", f_inst);
                }
            }
        }

        if (img_core) {
            LOGI("[+] UnityEngine.CoreModule located: %p", img_core);
            void* transformClass = il2cpp_class_from_name(img_core, "UnityEngine", "Transform");
            if (transformClass) {
                void* m_get_pos = il2cpp_class_get_method_from_name(transformClass, "get_position", 0);
                if (m_get_pos) g_Offsets.fn_Transform_get_position = *(void**)m_get_pos;

                void* m_set_pos = il2cpp_class_get_method_from_name(transformClass, "set_position", 1);
                if (m_set_pos) g_Offsets.fn_Transform_set_position = *(void**)m_set_pos;
            }

            void* cameraClass = il2cpp_class_from_name(img_core, "UnityEngine", "Camera");
            if (cameraClass) {
                void* m_main = il2cpp_class_get_method_from_name(cameraClass, "get_main", 0);
                if (m_main) g_Offsets.fn_Camera_get_main = *(void**)m_main;

                void* m_w2s = il2cpp_class_get_method_from_name(cameraClass, "WorldToScreenPoint", 1);
                if (m_w2s) g_Offsets.fn_Camera_WorldToScreenPoint = *(void**)m_w2s;
            }

            void* timeClass = il2cpp_class_from_name(img_core, "UnityEngine", "Time");
            if (timeClass) {
                void* m_ts = il2cpp_class_get_method_from_name(timeClass, "set_timeScale", 1);
                if (m_ts) g_Offsets.fn_Time_set_timeScale = *(void**)m_ts;
            }
        }
    } else {
        LOGI("[~] IL2CPP API not fully exported, switching to AOB pattern scan fallback...");
        // AOB Signature scans for TakeDamage and HeadCollider
        // TakeDamage prologue signature
        const char pat_td[] = "\xFD\x7B\xBF\xA9\xFD\x03\x00\x91\xF8\x5F\x01\xA9";
        const char msk_td[] = "xxxxxxxxxxxx";
        uintptr_t scan_td = pattern_scan_module("libil2cpp.so", pat_td, msk_td);
        if (scan_td) {
            g_Offsets.fn_TakeDamage_9 = (void*)scan_td;
            LOGI("[+] AOB Scan matched TakeDamage_9 @ %p", g_Offsets.fn_TakeDamage_9);
        }
    }

    LOGI("[✓] Auto-scan completed. All dynamic hooks and offsets ready.");
    return true;
}

} // namespace scanner
