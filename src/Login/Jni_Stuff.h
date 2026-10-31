// src/Login/Jni_Stuff.h
#pragma once

#include <string>
#include <atomic>

inline std::string g_Token = "NOKEY_TOKEN";
inline std::string g_Auth = "NOKEY_AUTH";
inline bool bValid = true;
inline std::string EXP = "2099-12-31";
inline std::string mod_status = "Active";
inline std::string DeviceID = "ZYGISK_INTERNAL";
inline std::string DeviceModel = "Android";
inline char g_license_key_input[256] = "NOKEY";

inline bool CheckLicenseValidity() {
    return true;
}
