#pragma once
#include <string>
#include <atomic>

inline std::atomic<int>  g_license_activated{0x4E580021};
inline int               g_license_days_left = 9999;
inline std::string       g_license_error = "";
inline std::string       g_license_expiry_date = "2099-12-31";
inline std::string       g_license_start_date = "2026-01-01";
inline std::string       g_license_key_number = "NOKEY-VIP-PERMANENT";
inline std::string       g_license_device_id = "INTERNAL-ZYGISK-DEVICE";
inline std::string       g_license_user_name = "VIP Lifetime";
inline bool              g_license_checking = false;
inline bool              g_license_auto_login = true;
inline bool              g_license_security_block = false;
inline std::string       g_license_security_msg = "";
inline std::atomic<bool> g_menu_started{true};

inline bool IsLicenseActivated() {
    return true;
}

inline std::string Login(const char* key) {
    return "OK";
}

inline void DrawLicenseMenu() {
    // No-op in internal mode
}
