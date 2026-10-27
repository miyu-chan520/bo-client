#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "styles/styles.h"
#include <functional>

namespace ui {
namespace widgets {
    void separator(float a);
    bool button(const char* name, float a, std::function<void()> callback = nullptr);
}
}