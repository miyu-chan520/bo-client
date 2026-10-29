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
#include "imgui.h"

#define TOUCH_MODE 2

namespace touch {
    static std::atomic<bool> g_touch_running{true};

    struct input {
        int fd;
        bool canBeUsed{false};
        input_absinfo absX{}, absY{};
        float absXMultiplier{0}, absYMultiplier{0};
#if TOUCH_MODE == 2
        bool trackingIDPresent{false};
#endif

        input(const char *_path, int32_t _screen_w, int32_t _screen_h) {
            fd = open(_path, O_RDWR);
            if (fd == -1)
                return;
            canBeUsed = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX) != -1 && ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY) != -1 && absX.maximum > 0 && absY.maximum > 0;
            if (!canBeUsed) {
                close();
                return;
            }

#if TOUCH_MODE == 2
            input_absinfo absTrackingID{};
            trackingIDPresent = ioctl(fd, EVIOCGABS(ABS_MT_TRACKING_ID), &absTrackingID) != -1 && absTrackingID.minimum <= 0 && absTrackingID.maximum > 0;
#endif

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

    uint8_t orientation;
    ImGuiIO *imgui_io;
    std::vector<input> inputs;

    void input_thread(const input &_input) {
        sleep(1);
        if (!imgui_io && ImGui::GetCurrentContext())
            imgui_io = &ImGui::GetIO();

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
#if TOUCH_MODE == 2
                    if (_input.trackingIDPresent && event.code == ABS_MT_TRACKING_ID) {
                        isDown = (event.value != -1);
                        continue;
                    }
#elif TOUCH_MODE == 0
                    if (event.code == ABS_MT_TRACKING_ID) {
                        isDown = (event.value != -1);
                        continue;
                    }
#endif
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
                    if (!imgui_io && ImGui::GetCurrentContext()) {
                        imgui_io = &ImGui::GetIO();
                    }

                    if (imgui_io) {
                        bool btnDown = isDown;
#if TOUCH_MODE == 2
                        if (!_input.trackingIDPresent && event_readed_count > 2) {
                            btnDown = true;
                        }
#elif TOUCH_MODE != 0
                        btnDown = (event_readed_count > 2);
#endif
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

                        imgui_io->AddMousePosEvent(mappedX, mappedY);
                        imgui_io->AddMouseButtonEvent(0, btnDown);
                        imgui_io->MousePos = ImVec2(mappedX, mappedY);
                        imgui_io->MouseDown[0] = btnDown;
                    }
                }
            }
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
        imgui_io = nullptr;
    }
}
