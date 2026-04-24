#pragma once

#include <imgui.h>
#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <algorithm>
#include <cfloat>

namespace our
{
    // A single floating text popup with configurable text, color, and animation
    struct TextPopup
    {
        std::string text;     // e.g. "+10", "-20 HP"
        glm::vec4 color;      // main text color (RGBA)
        glm::vec4 glowColor;  // outer glow color (RGBA, typically a darker/warmer variant)
        float timeRemaining;  // seconds left
        float totalTime;      // original lifetime
        float verticalOffset; // additional Y offset for stacking
    };

    // Usage:
    //   textPopupSystem.spawn("+10", {1,0.84,0,1});        // gold coin text
    //   textPopupSystem.spawn("-20 HP", {1,0.2,0.2,1});    // red damage text
    //   textPopupSystem.update(deltaTime);                   // in onDraw
    //   textPopupSystem.render();                            // in onImmediateGui
    class TextPopupSystem
    {
        std::vector<TextPopup> popups;

    public:
        // Spawn a new floating text popup at the center of the screen
        //  text     : the text to display
        //  color    : main color (RGBA)
        //  lifetime : how long it lives in seconds (default 1.2s)
        void spawn(const std::string &text, const glm::vec4 &color, float lifetime = 1.2f)
        {
            // Compute a glow color: same hue but darker and semi-transparent
            glm::vec4 glow = {color.r * 0.8f, color.g * 0.5f, color.b * 0.0f, 0.3f};

            // Stack offset: push newer popups below existing ones
            float stackOffset = 0.0f;
            for (const auto &p : popups)
                stackOffset += 50.0f;

            popups.push_back({text, color, glow, lifetime, lifetime, stackOffset});
        }

        // Tick all popup timers and remove expired ones
        void update(float deltaTime)
        {
            for (auto &p : popups)
                p.timeRemaining -= deltaTime;

            popups.erase(
                std::remove_if(popups.begin(), popups.end(),
                               [](const TextPopup &p)
                               { return p.timeRemaining <= 0.0f; }),
                popups.end());
        }

        // Render all active popups using ImGui foreground draw list
        // Call this from your state's onImmediateGui()
        void render()
        {
            if (popups.empty())
                return;

            ImGuiIO &io = ImGui::GetIO();
            ImDrawList *drawList = ImGui::GetForegroundDrawList();

            float screenW = io.DisplaySize.x;
            float screenH = io.DisplaySize.y;

            int index = 0;
            for (const auto &popup : popups)
            {
                float progress = 1.0f - (popup.timeRemaining / popup.totalTime); // 0→1

                // ── Position: float upward from ~40% screen height ──
                float baseY = screenH * 0.40f;
                float offsetY = progress * 120.0f + popup.verticalOffset;
                float posY = baseY - offsetY;

                // ── Fade: fully visible for first 30%, then linear fade out ──
                float alpha = 1.0f;
                if (progress > 0.3f)
                    alpha = 1.0f - ((progress - 0.3f) / 0.7f);
                alpha = alpha < 0.0f ? 0.0f : alpha;

                // ── Scale: start 30% larger, shrink to normal ──
                float scale = 1.0f + (1.0f - progress) * 0.3f;
                float fontSize = 48.0f * scale;

                // ── Colors with animated alpha ──
                ImU32 mainColor = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(popup.color.r, popup.color.g, popup.color.b, alpha));
                ImU32 shadowColor = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(0.0f, 0.0f, 0.0f, alpha * 0.8f));
                ImU32 glowColor = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(popup.glowColor.r, popup.glowColor.g, popup.glowColor.b, alpha * popup.glowColor.a));

                const char *text = popup.text.c_str();
                ImFont *font = ImGui::GetFont();
                ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
                float posX = (screenW - textSize.x) * 0.5f;

                // ── Draw glow layers ──
                for (int g = 0; g < 3; g++)
                {
                    float off = (float)(g + 1) * 2.0f;
                    drawList->AddText(font, fontSize, ImVec2(posX - off, posY - off), glowColor, text);
                    drawList->AddText(font, fontSize, ImVec2(posX + off, posY + off), glowColor, text);
                }

                // ── Draw dark shadow ──
                drawList->AddText(font, fontSize, ImVec2(posX + 2.0f, posY + 2.0f), shadowColor, text);
                drawList->AddText(font, fontSize, ImVec2(posX - 1.0f, posY + 2.0f), shadowColor, text);

                // ── Draw main text ──
                drawList->AddText(font, fontSize, ImVec2(posX, posY), mainColor, text);

                index++;
            }
        }

        // Clear all popups (e.g. on state destroy)
        void clear() { popups.clear(); }
    };

}