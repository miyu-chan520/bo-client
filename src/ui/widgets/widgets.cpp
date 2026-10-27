#define IMGUI_DEFINE_MATH_OPERATORS
#include "widgets.hpp"
#include "Android_draw/draw.h"
#include <functional>

namespace ui {

namespace widgets {
    using namespace style;

    void separator(float a) {
        if (a < 0.01f) return;

        ImGuiWindow* w = ImGui::GetCurrentWindow();
        ImVec2 pos = w->DC.CursorPos;
        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;

        ImDrawList* dl = w->DrawList;
        float y = pos.y + 4.f * S;

        dl->AddRectFilledMultiColor(
            ImVec2(pos.x, y), ImVec2(pos.x + ww * 0.4f, y + 1),
            col(UITheme::border, a * 0.6f), col(UITheme::border, 0.f),
            col(UITheme::border, 0.f), col(UITheme::border, a * 0.6f)
        );
        dl->AddRectFilledMultiColor(
            ImVec2(pos.x + ww * 0.6f, y), ImVec2(pos.x + ww, y + 1),
            col(UITheme::border, 0.f), col(UITheme::border, a * 0.6f),
            col(UITheme::border, a * 0.6f), col(UITheme::border, 0.f)
        );

        ImGui::Dummy(ImVec2(0, 8.f * S));
    }

    bool button(const char* name, float a, std::function<void()> callback) {
        if (a < 0.01f) return false;

        ImGuiWindow* w = ImGui::GetCurrentWindow();
        if (w->SkipItems) return false;

        std::string ids = std::string("btn_") + name;
        ImGuiID id = w->GetID(ids.c_str());

        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;
        float h = 30.f * S;

        ImVec2 pos = w->DC.CursorPos;
        ImRect r(pos, ImVec2(pos.x + ww, pos.y + h));

        ImGui::ItemSize(r);
        if (!ImGui::ItemAdd(r, id)) return false;

        bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max) && !popup();
        bool press = hov && ImGui::IsMouseClicked(0);

        if (press) {
            if (callback) callback();
        }

        ImDrawList* dl = w->DrawList;
        ImVec4 bg = hov ? UITheme::widget : UITheme::panel;
        dl->AddRectFilled(r.Min, r.Max, col(bg, a));
        dl->AddRect(r.Min, r.Max, col(UITheme::border, a));

        ImGui::PushFont(fontMedium);
        ImVec2 ts = ImGui::CalcTextSize(name);
        dl->AddText(ImVec2(r.GetCenter().x - ts.x * 0.5f, r.GetCenter().y - ts.y * 0.5f), 
                    col(UITheme::text, a), name);
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, h + 4.f * S));
        
        return press;
    }
}
}