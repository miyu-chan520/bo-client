#include <cstdio>
#include <cstring>
#include <thread>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <cmath>
#include <unistd.h>
#include <vector>
#include <atomic>
#include <mutex>
#include "imgui.h"
#include "../../includes/internal/Android_touch/Touch.hpp"

namespace touch {
    static std::atomic<bool> g_touch_running{true};

    // Thread-safe buffered event state
    static std::mutex g_touch_mtx;
    static float g_buffered_x = 0.0f;
    static float g_buffered_y = 0.0f;
    static bool  g_buffered_down = false;
    static bool  g_has_new_event = false;

    struct input {
        int fd{-1};
        bool canBeUsed{false};
        input_absinfo absX{}, absY{};
        float absXMultiplier{0}, absYMultiplier{0};
        bool trackingIDPresent{false};

        input(const char *_path, int32_t _screen_w, int32_t _screen_h) {
            fd = open(_path, O_RDWR);
            if (fd == -1)
                return;
            canBeUsed = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX) != -1 &&
                        ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY) != -1 &&
                        absX.maximum > 0 && absY.maximum > 0;
            if (!canBeUsed) {
                close();
                return;
            }

            input_absinfo absTrackingID{};
            trackingIDPresent = ioctl(fd, EVIOCGABS(ABS_MT_TRACKING_ID), &absTrackingID) != -1 &&
                                absTrackingID.minimum <= 0 && absTrackingID.maximum > 0;

            calculateMultipliers(_screen_w, _screen_h);
        }

        void close() {
            if (fd != -1) {
                ::close(fd);
                fd = -1;
            }
        }

        void calculateMultipliers(int32_t _screen_w, int32_t _screen_h) {
            auto max = fmax((float) absX.maximum, (float) absY.maximum);
            auto min = fmin((float) absX.maximum, (float) absY.maximum);
            absXMultiplier = (float) _screen_w / (_screen_w > _screen_h ? max : min);
            absYMultiplier = (float) _screen_h / (_screen_w > _screen_h ? min : max);
        }
    };

    static uint8_t orientation = 0;
    static std::vector<input> inputs;

    void input_thread(const input &_input) {
        input_event events[64]{0};
        bool isDown = false;
        float x = 0.0f, y = 0.0f;

        while (_input.fd > 0 && g_touch_running.load(std::memory_order_relaxed)) {
            auto event_readed_count = read(_input.fd, events, sizeof(events));
            if (event_readed_count <= 0)
                continue;
            event_readed_count /= (ssize_t) sizeof(input_event);

            for (long j = 0; j < event_readed_count; j++) {
                auto &event = events[j];

                if (event.type == EV_KEY) {
                    if (event.code == BTN_TOUCH) {
                        isDown = (event.value != 0);
                        continue;
                    }
                }

                if (event.type == EV_ABS) {
                    if (event.code == ABS_MT_TRACKING_ID) {
                        isDown = (event.value != -1);
                        continue;
                    }
                    if (event.code == ABS_MT_POSITION_X) {
                        x = (float) event.value;
                        continue;
                    }
                    if (event.code == ABS_MT_POSITION_Y) {
                        y = (float) event.value;
                        continue;
                    }
                }

                if (event.code == SYN_REPORT) {
                    bool btnDown = isDown;
                    if (!_input.trackingIDPresent && event_readed_count > 2) {
                        btnDown = true;
                    }

                    float mappedX = x, mappedY = y;
                    switch (orientation) {
                        case 1:
                            mappedX = y;
                            mappedY = _input.absX.maximum - x;
                            break;
                        case 2:
                            mappedX = _input.absX.maximum - x;
                            mappedY = _input.absY.maximum - y;
                            break;
                        case 3:
                            mappedX = _input.absY.maximum - y;
                            mappedY = x;
                            break;
                        default:
                            mappedX = x;
                            mappedY = y;
                            break;
                    }
                    mappedX *= _input.absXMultiplier;
                    mappedY *= _input.absYMultiplier;

                    // Buffer event safely without touching ImGui from this thread
                    {
                        std::lock_guard<std::mutex> lk(g_touch_mtx);
                        g_buffered_x = mappedX;
                        g_buffered_y = mappedY;
                        g_buffered_down = btnDown;
                        g_has_new_event = true;
                    }
                }
            }
        }
    }

    // Dispatches buffered touch events directly on the main thread (thread-safe)
    void dispatch_events() {
        if (!ImGui::GetCurrentContext()) return;
        ImGuiIO& io = ImGui::GetIO();

        float x = 0.0f, y = 0.0f;
        bool down = false;
        bool hasEvent = false;

        {
            std::lock_guard<std::mutex> lk(g_touch_mtx);
            if (g_has_new_event) {
                x = g_buffered_x;
                y = g_buffered_y;
                down = g_buffered_down;
                hasEvent = true;
                g_has_new_event = false;
            }
        }

        if (hasEvent) {
            io.AddMousePosEvent(x, y);
            io.AddMouseButtonEvent(0, down);
        }
    }

    bool init(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation) {
        orientation = _orientation;
        g_touch_running.store(true);

        bool result = false;

        const char *path = "/dev/input/";
        auto dir = opendir(path);
        if (!dir) return false;

        dirent *ptr;
        while ((ptr = readdir(dir))) {
            if (strstr(ptr->d_name, "event")) {
                char event_path[128];
                snprintf(event_path, sizeof(event_path), "%s%s", path, ptr->d_name);
                input _input(event_path, _screen_w, _screen_h);
                if (_input.canBeUsed) {
                    result = true;
                    inputs.push_back(_input);
                    std::thread(input_thread, _input).detach();
                }
            }
        }
        closedir(dir);

        return result;
    }

    void update(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation) {
        orientation = _orientation;

        for (auto &_input: inputs) {
            _input.calculateMultipliers(_screen_w, _screen_h);
        }
    }

    void updateOrientation(uint8_t _orientation) {
        orientation = _orientation;
    }

    void shutdown() {
        g_touch_running.store(false);
        for (auto &_input: inputs)
            _input.close();
        inputs.clear();
    }
}
