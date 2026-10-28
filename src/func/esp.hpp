#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "../game/data.hpp"
#include "../ui/cfg.hpp"
#include "../ui/styles/styles.h"
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>
#include <algorithm>
#include <cmath>

namespace func::esp {

    // ═══════════════════════════════════════════════════════
    // HELPER COLOR
    // ═══════════════════════════════════════════════════════
    inline ImU32 toU32(const ImVec4& c, float alphaMul = 1.f) {
        return IM_COL32(
            (int)(c.x * 255.f),
            (int)(c.y * 255.f),
            (int)(c.z * 255.f),
            (int)(c.w * 255.f * alphaMul));
    }

    inline ImU32 withAlpha(ImU32 c, float mul) {
        int a = (int)(((c >> 24) & 0xFF) * mul);
        if (a < 0) a = 0; if (a > 255) a = 255;
        return (c & 0x00FFFFFF) | ((ImU32)a << 24);
    }

    inline ImU32 fadeDist(ImU32 base, float dist, float maxRange) {
        float t = ImClamp(1.f - dist / maxRange, 0.35f, 1.f);
        return withAlpha(base, t);
    }

    inline ImU32 hpColor(float t) {
        t = ImClamp(t, 0.f, 1.f);
        float r, g, b;
        if (t < 0.5f) { float k = t * 2.f; r = 1.f; g = 0.35f + 0.55f * k; b = 0.15f; }
        else          { float k = (t - 0.5f) * 2.f; r = 1.f - 0.9f * k; g = 0.9f; b = 0.15f + 0.30f * k; }
        return IM_COL32((int)(r*255), (int)(g*255), (int)(b*255), 255);
    }

    inline ImU32 armorColor(float t) {
        t = ImClamp(t, 0.f, 1.f);
        return IM_COL32((int)(80 + 100 * t), (int)(180 + 60 * t), 255, 255);
    }

    // ═══════════════════════════════════════════════════════
    // FORMAT
    // ═══════════════════════════════════════════════════════
    inline bool ok(const Vector3& v) { return v.x > 0 && v.y > 0; }

    inline void fmtDist(float d, char* out, size_t n) {
        if (d >= 1000.f) snprintf(out, n, "%.1fkm", d / 1000.f);
        else             snprintf(out, n, "%.0fm", d);
    }

    inline void truncName(const char* src, char* out, size_t n) {
        if (!src || !src[0]) { out[0] = 0; return; }
        size_t len = strlen(src);
        if (len <= 12) { snprintf(out, n, "%s", src); return; }
        snprintf(out, n, "%.11s.", src);
    }

    // ═══════════════════════════════════════════════════════
    // CONSTANTS
    // ═══════════════════════════════════════════════════════
    static constexpr ImU32 C_SHADOW = IM_COL32(0, 0, 0, 180);
    static constexpr ImU32 C_BORDER = IM_COL32(255, 255, 255, 40);

    // ── FONT SIZE (kecil, seimbang) ──
    static constexpr float ESP_FONT_SIZE = 12.f;
    static constexpr float PANEL_ROW_H   = ESP_FONT_SIZE + 5.f;   // 17px

    // ── PANEL OFFSET DARI KEPALA (jauh di atas) ──
    static constexpr float PANEL_TOP_OFFSET = 35.f;

    // ═══════════════════════════════════════════════════════
    // SKELETON BOX — hitung bounding box dari bone points
    // ═══════════════════════════════════════════════════════
    struct SkeletonBox {
        ImVec2 tl, br;
        bool   valid;
    };

    inline SkeletonBox computeSkeletonBox(const PlayerData& p) {
        SkeletonBox sb{ {0,0}, {0,0}, false };

        const Vector3* pts[] = {
            &p.Head, &p.Breast, &p.Hip,
            &p.LShoulder, &p.RShoulder,
            &p.LForeArm,  &p.RForeArm,
            &p.LHand,     &p.RHand,
            &p.LAnkle,    &p.RAnkle,
            &p.LToe,      &p.RToe
        };

        float minX =  1e9f, minY =  1e9f;
        float maxX = -1e9f, maxY = -1e9f;
        int   cnt  = 0;

        for (auto* v : pts) {
            if (!ok(*v)) continue;
            if (v->x < minX) minX = v->x;
            if (v->x > maxX) maxX = v->x;
            if (v->y < minY) minY = v->y;
            if (v->y > maxY) maxY = v->y;
            cnt++;
        }

        if (cnt < 4) {
            if (ok(p.Head) && ok(p.Toe)) {
                minX = ImMin(p.Head.x, p.Toe.x);
                maxX = ImMax(p.Head.x, p.Toe.x);
                minY = ImMin(p.Head.y, p.Toe.y);
                maxY = ImMax(p.Head.y, p.Toe.y);
                sb.valid = true;
            }
            if (!sb.valid) return sb;
        } else {
            sb.valid = true;
        }

        const float padX = 4.f;
        const float padY = 3.f;

        sb.tl = ImVec2(minX - padX, minY - padY);
        sb.br = ImVec2(maxX + padX, maxY + padY);
        return sb;
    }

    // ═══════════════════════════════════════════════════════
    // NAME PANEL
    // ═══════════════════════════════════════════════════════
    inline void drawNamePanel(ImDrawList* dl, const PlayerData& p,
                              float cx, float top_y) {
        if (!cfg::esp::name && !cfg::esp::distance &&
            !cfg::esp::health && !cfg::esp::armor) return;

        char nameBuf[24];
        truncName(p.name, nameBuf, sizeof(nameBuf));
        if (nameBuf[0] == 0)
            snprintf(nameBuf, sizeof(nameBuf), "%s", p.isBot ? "BOT" : "Player");

        char distBuf[16];
        fmtDist(p.distance, distBuf, sizeof(distBuf));

        ImU32 nameCol = toU32(cfg::esp::name_col);
        ImU32 distCol = toU32(cfg::esp::distance_col);
        ImU32 visTag  = IM_COL32(30, 100, 220, 240);
        ImU32 botTag  = IM_COL32(200, 40, 40, 240);

        const char* tag = cfg::esp::bot_tag ? (p.isBot ? "AI" : "PL") : nullptr;
        ImU32 tagBg = p.isBot ? botTag : visTag;

        ImU32 bgMain    = p.isVisible ? IM_COL32(12, 12, 16, 235) : IM_COL32(35, 35, 42, 215);
        ImU32 textColor = p.isVisible ? nameCol : withAlpha(nameCol, 0.65f);

        const float fs   = ESP_FONT_SIZE;
        const float padX = 5.f;
        const float padY = 2.f;
        const float gap  = 3.f;

        float tagW  = tag ? ImGui::CalcTextSize(tag).x + padX * 2.f : 0.f;
        float nameW = cfg::esp::name
                      ? ImGui::CalcTextSize(nameBuf).x + padX * 2.f : 0.f;
        float distW = cfg::esp::distance
                      ? ImGui::CalcTextSize(distBuf).x + padX * 2.f : 0.f;

        float rowH = PANEL_ROW_H;
        float totalW = tagW;
        if (nameW > 0.f) totalW += (totalW > 0.f ? gap : 0.f) + nameW;
        if (distW > 0.f) totalW += (totalW > 0.f ? gap : 0.f) + distW;
        if (totalW <= 0.f) return;

        float bx = cx - totalW * 0.5f;
        float by = top_y - rowH - PANEL_TOP_OFFSET;

        dl->AddRectFilled({bx + 1, by + 1}, {bx + totalW + 1, by + rowH + 1},
                          C_SHADOW, 4.f);
        dl->AddRectFilled({bx, by}, {bx + totalW, by + rowH}, bgMain, 4.f);
        dl->AddRect({bx, by}, {bx + totalW, by + rowH}, C_BORDER, 4.f, 0, 1.f);

        float cursorX = bx;

        if (tag) {
            dl->AddRectFilled({cursorX, by}, {cursorX + tagW, by + rowH},
                              withAlpha(tagBg, p.isVisible ? 1.f : 0.65f),
                              4.f, ImDrawFlags_RoundCornersLeft);
            ImVec2 ts = ImGui::CalcTextSize(tag);
            dl->AddText(ImGui::GetFont(), fs,
                {cursorX + (tagW - ts.x) * 0.5f, by + padY},
                IM_COL32(255, 255, 255, 255), tag);
            cursorX += tagW;
        }

        if (cfg::esp::name) {
            if (tag) cursorX += gap;
            ImVec2 ts = ImGui::CalcTextSize(nameBuf);
            dl->AddText(ImGui::GetFont(), fs,
                {cursorX + (nameW - ts.x) * 0.5f, by + padY},
                textColor, nameBuf);
            cursorX += nameW;
        }

        if (cfg::esp::distance) {
            if (cursorX > bx) cursorX += gap;
            ImVec2 ts = ImGui::CalcTextSize(distBuf);
            ImU32 dCol = p.isVisible ? distCol : withAlpha(distCol, 0.65f);
            dl->AddText(ImGui::GetFont(), fs,
                {cursorX + (distW - ts.x) * 0.5f, by + padY}, dCol, distBuf);
        }

        float barY = by + rowH + 2.f;
        float barH = 3.f;
        float barOffset = 0.f;

        if (cfg::esp::health) {
            float hp01 = p.maxHP > 0 ? (float)p.curHP / (float)p.maxHP : 0.f;
            hp01 = ImClamp(hp01, 0.f, 1.f);

            dl->AddRectFilled({bx, barY}, {bx + totalW, barY + barH},
                              IM_COL32(0, 0, 0, 210), barH * 0.5f);
            dl->AddRectFilled({bx, barY}, {bx + totalW * hp01, barY + barH},
                              hpColor(hp01), barH * 0.5f);
            dl->AddRect({bx, barY}, {bx + totalW, barY + barH},
                        C_BORDER, barH * 0.5f, 0, 1.f);
            barOffset = barH + 1.f;
        }

        if (cfg::esp::armor && p.curArmor > 0) {
            float arm01 = ImClamp((float)p.curArmor / 100.f, 0.f, 1.f);
            float armY  = barY + barOffset;
            float armH  = 2.5f;

            dl->AddRectFilled({bx, armY}, {bx + totalW, armY + armH},
                              IM_COL32(0, 0, 0, 210), armH * 0.5f);
            dl->AddRectFilled({bx, armY}, {bx + totalW * arm01, armY + armH},
                              armorColor(arm01), armH * 0.5f);
        }
    }

    // ═══════════════════════════════════════════════════════
    // BOX
    // ═══════════════════════════════════════════════════════
    inline void drawBox(ImDrawList* dl, ImVec2 tl, ImVec2 br, ImU32 col, int type) {
        float thick = cfg::esp::box_thickness;
        float round = cfg::esp::box_rounding;

        if (type == 0) {
            float len = (br.x - tl.x) * cfg::esp::bracket_frac;
            ImVec2 pts[8][2] = {
                {tl,                 {tl.x + len, tl.y}},
                {tl,                 {tl.x, tl.y + len}},
                {{br.x - len, tl.y}, {br.x, tl.y}},
                {{br.x, tl.y},       {br.x, tl.y + len}},
                {{tl.x, br.y - len}, {tl.x, br.y}},
                {{tl.x, br.y},       {tl.x + len, br.y}},
                {{br.x - len, br.y}, {br.x, br.y}},
                {{br.x, br.y - len}, {br.x, br.y}},
            };
            for (auto& s : pts) {
                dl->AddLine(s[0], s[1], withAlpha(col, 0.25f), thick + 3.f);
                dl->AddLine({s[0].x + 1, s[0].y + 1},
                            {s[1].x + 1, s[1].y + 1}, C_SHADOW, thick + 1.f);
                dl->AddLine(s[0], s[1], col, thick);
            }
        } else if (type == 1) {
            dl->AddRect({tl.x + 2, tl.y + 2}, {br.x + 2, br.y + 2},
                        IM_COL32(0, 0, 0, 120), round, 0, thick + 3.f);
            dl->AddRect(tl, br, withAlpha(col, 0.35f), round, 0, thick + 2.f);
            dl->AddRect(tl, br, col, round, 0, thick);
        } else {
            dl->AddRect(tl, br, withAlpha(col, 0.18f), round, 0, thick + 5.f);
            dl->AddRect(tl, br, withAlpha(col, 0.45f), round, 0, thick + 2.f);
            dl->AddRect(tl, br, col, round, 0, thick);
            dl->AddRectFilled(tl, br, withAlpha(col, 0.06f), round);
        }
    }

    // ═══════════════════════════════════════════════════════
    // SKELETON
    // ═══════════════════════════════════════════════════════
    inline void sLine(ImDrawList* dl, const Vector3& a, const Vector3& b,
                      ImU32 col, float t = 1.6f) {
        if (!ok(a) || !ok(b)) return;
        dl->AddLine({a.x, a.y}, {b.x, b.y}, withAlpha(col, 0.25f), t + 2.f);
        dl->AddLine({a.x, a.y}, {b.x, b.y}, col, t);
    }

    inline void sDot(ImDrawList* dl, const Vector3& v, ImU32 col, float r = 2.4f) {
        if (!ok(v)) return;
        dl->AddCircleFilled({v.x, v.y}, r + 2.f, withAlpha(col, 0.25f));
        dl->AddCircleFilled({v.x, v.y}, r, col);
    }

    inline void drawSkeleton(ImDrawList* dl, const PlayerData& p, ImU32 col) {
        sLine(dl, p.Head, p.Breast, col, 1.8f);
        sLine(dl, p.Breast, p.Hip, col, 1.8f);

        sLine(dl, p.Breast, p.LShoulder, col);
        sLine(dl, p.Breast, p.RShoulder, col);
        sLine(dl, p.LShoulder, p.LForeArm, col);
        sLine(dl, p.RShoulder, p.RForeArm, col);
        sLine(dl, p.LForeArm, p.LHand, col);
        sLine(dl, p.RForeArm, p.RHand, col);

        sLine(dl, p.Hip, p.LAnkle, col, 1.8f);
        sLine(dl, p.Hip, p.RAnkle, col, 1.8f);
        sLine(dl, p.LAnkle, p.LToe, col);
        sLine(dl, p.RAnkle, p.RToe, col);

        sDot(dl, p.Head, col, 2.8f);
        sDot(dl, p.Breast, col, 2.4f);
        sDot(dl, p.Hip, col, 2.4f);
        sDot(dl, p.LShoulder, col, 2.0f);
        sDot(dl, p.RShoulder, col, 2.0f);
        sDot(dl, p.LHand, col, 2.0f);
        sDot(dl, p.RHand, col, 2.0f);
        sDot(dl, p.LAnkle, col, 2.0f);
        sDot(dl, p.RAnkle, col, 2.0f);
    }

    // ═══════════════════════════════════════════════════════
    // FIRING
    // ═══════════════════════════════════════════════════════
    inline void drawFiring(ImDrawList* dl, const PlayerData& p, float cx, float ty) {
        if (!p.isFiring) return;
        float t = ImGui::GetTime();
        float pulse = 0.5f + 0.5f * sinf(t * 12.f);
        float r = 3.f + pulse * 1.2f;
        ImVec2 pos(cx, ty - 20.f);

        dl->AddCircleFilled(pos, r + 4.f,
            IM_COL32(255, 200, 40, (int)(70 * pulse)));
        dl->AddCircleFilled(pos, r, IM_COL32(255, 200, 40, 255));
        dl->AddCircle(pos, r + 1.f, IM_COL32(255, 255, 255, 210), 16, 1.2f);
    }

    // ═══════════════════════════════════════════════════════
    // WEAPON PILL
    // ═══════════════════════════════════════════════════════
    inline void drawWeapon(ImDrawList* dl, const PlayerData& p, ImVec2 foot) {
        if (p.weaponType < 0) return;
        static const char* kNames[] = {
            "AR","SNIPER","SMG","SHOTGUN","PISTOL","MELEE",
            "GRENADE","LMG","RIFLE","MARKSMAN","SPECIAL"
        };
        const char* wn = (p.weaponType < (int)(sizeof(kNames)/sizeof(*kNames)))
                         ? kNames[p.weaponType] : "WPN";

        const float fs   = ESP_FONT_SIZE - 2.f;
        const float padX = 6.f;
        const float padY = 3.f;

        ImVec2 ts = ImGui::CalcTextSize(wn);
        float pw = ts.x + padX * 2.f;
        float ph = fs + padY * 2.f;
        float px = foot.x - pw * 0.5f;
        float py = foot.y + 4.f;

        ImU32 wCol = toU32(cfg::esp::weapon_col);
        ImU32 bg   = p.isVisible ? IM_COL32(0, 0, 0, 210) : IM_COL32(0, 0, 0, 150);

        dl->AddRectFilled({px + 1, py + 1}, {px + pw + 1, py + ph + 1},
                          C_SHADOW, 5.f);
        dl->AddRectFilled({px, py}, {px + pw, py + ph}, bg, 5.f);
        dl->AddRect({px, py}, {px + pw, py + ph}, C_BORDER, 5.f, 0, 1.f);
        dl->AddText(ImGui::GetFont(), fs,
            {px + padX, py + padY}, wCol, wn);
    }

    // ═══════════════════════════════════════════════════════
    // LOOT
    // ═══════════════════════════════════════════════════════
    inline const char* lootCat(uint32_t resID) {
        uint32_t c = (resID >> 24) & 0xFF;
        switch (c) {
            case 0x01: return "GUN";
            case 0x02: return "AMMO";
            case 0x03: return "ARMOR";
            case 0x04: return "HEAL";
            case 0x05: return "NADE";
            case 0x06: return "BAG";
            default:   return "ITEM";
        }
    }

    inline ImU32 lootColor(const char* cat) {
        if (strcmp(cat, "GUN")   == 0) return IM_COL32(255, 100, 100, 255);
        if (strcmp(cat, "AMMO")  == 0) return IM_COL32(255, 200, 80, 255);
        if (strcmp(cat, "ARMOR") == 0) return IM_COL32(80, 180, 255, 255);
        if (strcmp(cat, "HEAL")  == 0) return IM_COL32(80, 255, 120, 255);
        if (strcmp(cat, "NADE")  == 0) return IM_COL32(200, 80, 255, 255);
        if (strcmp(cat, "BAG")   == 0) return IM_COL32(160, 120, 60, 255);
        return IM_COL32(200, 200, 200, 255);
    }

    inline void drawLootLabel(ImDrawList* dl, const LootItem& li) {
        const char* cat = lootCat(li.resID);
        ImU32 col = lootColor(cat);

        char buf[64];
        snprintf(buf, sizeof(buf), "%s  %.0fm", cat, li.distance);

        ImVec2 ts = ImGui::CalcTextSize(buf);
        float pad = 4.f;
        float px = li.screenPos.x - ts.x * 0.5f - pad;
        float py = li.screenPos.y;

        dl->AddRectFilled({px + 1, py + 1},
                          {px + ts.x + pad * 2 + 1, py + ts.y + pad * 2 + 1},
                          C_SHADOW, 4.f);
        dl->AddRectFilled({px, py},
                          {px + ts.x + pad * 2, py + ts.y + pad * 2},
                          IM_COL32(15, 15, 20, 220), 4.f);
        dl->AddRect({px, py},
                    {px + ts.x + pad * 2, py + ts.y + pad * 2},
                    col, 4.f, 0, 1.5f);

        dl->AddText(ImGui::GetFont(), ESP_FONT_SIZE - 2.f,
                    {px + pad, py + pad}, col, buf);
        dl->AddCircleFilled({li.screenPos.x, li.screenPos.y}, 3.f, col);
    }

    inline void drawLoot() {
        if (!data::inMatch.load()) return;
        ImDrawList* dl = ImGui::GetForegroundDrawList();

        std::vector<LootItem> snap;
        {
            std::lock_guard<std::mutex> lk(data::loots_mutex);
            snap = data::loots;
        }

        for (const auto& li : snap) {
            if (li.screenPos.x < 0 || li.screenPos.y < 0) continue;
            if (li.screenPos.x > g_sw || li.screenPos.y > g_sh) continue;
            drawLootLabel(dl, li);
        }
    }

    // ═══════════════════════════════════════════════════════
    // CROSSHAIR
    // ═══════════════════════════════════════════════════════
    inline void drawCrosshair(ImDrawList* dl) {
        float cx = g_sw * 0.5f, cy = g_sh * 0.5f;
        float gap = 6.f, len = 10.f;
        ImU32 c = IM_COL32(220, 40, 40, 240);

        dl->AddLine({cx - gap - len, cy}, {cx - gap, cy}, c, 1.8f);
        dl->AddLine({cx + gap, cy}, {cx + gap + len, cy}, c, 1.8f);
        dl->AddLine({cx, cy - gap - len}, {cx, cy - gap}, c, 1.8f);
        dl->AddLine({cx, cy + gap}, {cx, cy + gap + len}, c, 1.8f);
        dl->AddCircleFilled({cx, cy}, 1.5f, c);
    }

    // ═══════════════════════════════════════════════════════
    // MAIN DRAW
    // ═══════════════════════════════════════════════════════
    inline void draw() {
        if (!data::inMatch.load()) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList();

        std::vector<PlayerData> snap;
        {
            std::lock_guard<std::mutex> lk(data::players_mutex);
            snap = data::players;
        }

        ImVec2 anchor(g_sw * 0.5f, 0.f);

        for (const auto& p : snap) {
            if (p.Head.x < 0 || p.Head.y < 0) continue;
            if (p.Toe.x  < 0 || p.Toe.y  < 0) continue;

            if (p.distance > cfg::esp::max_range) continue;

            SkeletonBox sb = computeSkeletonBox(p);
            if (!sb.valid) continue;

            ImVec2 tl = sb.tl;
            ImVec2 br = sb.br;

            if (br.x < -100 || tl.x > g_sw + 100) continue;
            if (br.y < -100 || tl.y > g_sh + 100) continue;

            float cx = (tl.x + br.x) * 0.5f;

            ImU32 colBase = p.isVisible ? toU32(cfg::esp::box_visible)
                                        : toU32(cfg::esp::box_col);
            colBase = fadeDist(colBase, p.distance, cfg::esp::max_range);

            if (cfg::esp::tracer) {
                ImVec2 head(p.Head.x, p.Head.y);
                float aa = ImClamp(1.f - p.distance / cfg::esp::max_range, 0.25f, 1.f);
                dl->AddLine(anchor, head,
                    withAlpha(toU32(cfg::esp::box_col), 0.35f * aa), 4.f);
                dl->AddLine(anchor, head,
                    withAlpha(toU32(cfg::esp::box_col), 0.95f * aa), 1.8f);
            }

            if (cfg::esp::box)
                drawBox(dl, tl, br, colBase, cfg::esp::box_type);

            if (cfg::esp::skeleton)
                drawSkeleton(dl, p, colBase);

            if (cfg::esp::firing_ind)
                drawFiring(dl, p, cx, tl.y);

            if (cfg::esp::name || cfg::esp::distance ||
                cfg::esp::health || cfg::esp::armor) {
                PlayerData tmp = p;
                if (tmp.name[0] == 0)
                    snprintf(tmp.name, sizeof(tmp.name), "%s",
                             p.isBot ? "BOT" : "Player");
                drawNamePanel(dl, tmp, cx, tl.y);
            }

            if (cfg::esp::weapon)
                drawWeapon(dl, p, {cx, br.y});
        }

        if (cfg::esp::loot)      drawLoot();
        if (cfg::esp::crosshair) drawCrosshair(dl);
        if (cfg::aim::draw_fov) {
            dl->AddCircle(ImVec2(g_sw * 0.5f, g_sh * 0.5f), cfg::aim::fov,
                          toU32(cfg::aim::fov_col), 64, 1.5f);
        }
    }
}