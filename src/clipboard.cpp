// src/Login/clipboard.cpp
// Baca clipboard Android via root (service call clipboard)

#include "clipboard.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <android/log.h>

#define LOG_TAG "NXODA_CLIP"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// ─── Metode 1: service call clipboard ───
static std::string tryServiceCall() {
    FILE* f = popen(
        "service call clipboard 2 s16 com.android.shell i32 0 2>/dev/null",
        "r");
    if (!f) return "";

    char buf[4096];
    std::string raw;
    while (fgets(buf, sizeof(buf), f)) {
        raw += buf;
    }
    pclose(f);

    if (raw.empty()) return "";

    // Extract all text enclosed within quotes '...'
    std::string result;
    size_t pos = 0;
    while (true) {
        size_t start = raw.find('\'', pos);
        if (start == std::string::npos) break;
        size_t end = raw.find('\'', start + 1);
        if (end == std::string::npos) break;

        std::string chunk = raw.substr(start + 1, end - start - 1);
        for (char c : chunk) {
            if (c != '.' && c >= 32 && c < 127) {
                result += c;
            }
        }
        pos = end + 1;
    }

    return result;
}

// ─── Metode 2: cmd clipboard get (Android 10+) ───
static std::string tryCmdClipboard() {
    FILE* f = popen("cmd clipboard get 2>/dev/null", "r");
    if (!f) return "";

    char buf[4096];
    std::string raw;
    while (fgets(buf, sizeof(buf), f)) {
        raw += buf;
    }
    pclose(f);

    while (!raw.empty() && (raw.back() == '\n' || raw.back() == '\r')) {
        raw.pop_back();
    }
    return raw;
}

// ─── Main: getClipboard ───
std::string getClipboard() {
    std::string result = tryServiceCall();
    if (!result.empty()) {
        LOGI("Clipboard (service call): %s", result.c_str());
        return result;
    }

    result = tryCmdClipboard();
    if (!result.empty()) {
        LOGI("Clipboard (cmd): %s", result.c_str());
        return result;
    }

    LOGI("Clipboard empty / error");
    return "";
}