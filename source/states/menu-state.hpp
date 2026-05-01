#pragma once

#include <application.hpp>
#include <shader/shader.hpp>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <systems/audio-system.hpp>
#include <imgui.h>

#include <functional>
#include <array>
#include <algorithm>
#include <cfloat>
#include <cmath>

struct Button
{
    glm::vec2 position, size;
    std::function<void()> action;

    bool isInside(const glm::vec2 &v) const
    {
        return position.x <= v.x && position.y <= v.y &&
               v.x <= position.x + size.x &&
               v.y <= position.y + size.y;
    }

    glm::mat4 getLocalToWorld() const
    {
        return glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, 0.0f)) *
               glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
    }
};

class Menustate : public our::State
{
    static constexpr const char *GameTitle = "DUSTY AIR RUSH";
    static constexpr std::array<const char *, 2> ButtonLabels = {
        "PRESS SPACE TO PLAY",
        "PRESS ESC TO EXIT"};
    static constexpr const char *LeaderboardLabel = "LEADERBOARD";
    static constexpr const char *LeaderboardHint = "PRESS L";

    our::TexturedMaterial *menuMaterial;
    our::TexturedMaterial *cupMaterial;
    our::TintedMaterial *highlightMaterial;
    // Dark overlay drawn on top of the background to improve text legibility
    our::TintedMaterial *darkOverlay;
    our::Mesh *rectangle;
    float time = 0.0f;

    std::array<Button, 2> buttons;
    Button leaderboardButton{};
    std::array<glm::vec2, 2> buttonTextPositions{};
    std::array<glm::vec2, 2> buttonTextSizes{};
    glm::vec2 titlePosition{};
    glm::vec2 titleSize{};
    glm::vec2 leaderboardLabelPosition{};
    glm::vec2 leaderboardHintPosition{};

    float titleFontSize = 96.0f;
    float buttonFontSize = 44.0f;
    float leaderboardFontSize = 26.0f;
    ImFont *titleFont = nullptr;
    ImFont *buttonFont = nullptr;

    our::AudioSystem menuAudio;

    // ─── Layout ──────────────────────────────────────────────────────────────
    void updateLayout(const glm::ivec2 &fbSize)
    {
        ImFont *layoutTitleFont = titleFont ? titleFont : ImGui::GetFont();
        ImFont *layoutButtonFont = buttonFont ? buttonFont : layoutTitleFont;
        float W = (float)fbSize.x;
        float H = (float)fbSize.y;

                titleFontSize = std::clamp(H * 0.14f, 90.0f, 150.0f);
        buttonFontSize = std::clamp(H * 0.065f, 44.0f, 72.0f);

        ImVec2 ts = layoutTitleFont->CalcTextSizeA(titleFontSize, FLT_MAX, 0.0f, GameTitle);
        titleSize = {ts.x, ts.y};
        titlePosition = {
            (W - titleSize.x) * 0.5f,
            H * 0.52f //  middle of screen
        };

        float padX = std::clamp(W * 0.030f, 32.0f, 56.0f);
        float padY = std::clamp(H * 0.014f, 14.0f, 22.0f);
        float titleGap = std::clamp(H * 0.07f, 40.0f, 72.0f);
        float btnGap = std::clamp(H * 0.008f, 6.0f, 12.0f);
        float currentY = titlePosition.y + titleSize.y + titleGap;
        leaderboardFontSize = std::clamp(H * 0.030f, 22.0f, 34.0f);

        for (size_t i = 0; i < buttons.size(); ++i)
        {
            ImVec2 bs = layoutButtonFont->CalcTextSizeA(buttonFontSize, FLT_MAX, 0.0f, ButtonLabels[i]);
            buttonTextSizes[i] = {bs.x, bs.y};

            buttons[i].size = {bs.x + padX * 2.0f, bs.y + padY * 2.0f};
            buttons[i].position = {(W - buttons[i].size.x) * 0.5f, currentY};

            buttonTextPositions[i] = {
                buttons[i].position.x + (buttons[i].size.x - bs.x) * 0.5f,
                buttons[i].position.y + (buttons[i].size.y - bs.y) * 0.5f};

            currentY += buttons[i].size.y + btnGap;
        }

        float iconSize = std::clamp(H * 0.12f, 74.0f, 120.0f);
        float margin = std::clamp(W * 0.025f, 22.0f, 42.0f);
        leaderboardButton.size = {iconSize, iconSize};
        leaderboardButton.position = {W - margin - iconSize, margin};

        ImVec2 labelSize = layoutButtonFont->CalcTextSizeA(leaderboardFontSize, FLT_MAX, 0.0f, LeaderboardLabel);
        ImVec2 hintSize = layoutButtonFont->CalcTextSizeA(leaderboardFontSize * 0.82f, FLT_MAX, 0.0f, LeaderboardHint);
        leaderboardLabelPosition = {
            leaderboardButton.position.x + (leaderboardButton.size.x - labelSize.x) * 0.5f,
            leaderboardButton.position.y + leaderboardButton.size.y + H * 0.012f};
        leaderboardHintPosition = {
            leaderboardButton.position.x + (leaderboardButton.size.x - hintSize.x) * 0.5f,
            leaderboardLabelPosition.y + labelSize.y + H * 0.004f};
    }

    // ─── Shadowed text helper ─────────────────────────────────────────────────
    static void drawShadowedText(ImDrawList *dl, ImFont *font, float fontSize,
                                 const glm::vec2 &pos, ImU32 color, const char *text,
                                 float shadowOpacityMul = 0.85f, float shadowOffset = 4.0f)
    {
        float alpha = ((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
        ImU32 shadow = ImGui::ColorConvertFloat4ToU32(
            ImVec4(0.02f, 0.01f, 0.01f, alpha * shadowOpacityMul));
        dl->AddText(font, fontSize, ImVec2(pos.x + shadowOffset, pos.y + shadowOffset), shadow, text);
        dl->AddText(font, fontSize, ImVec2(pos.x + shadowOffset * 0.5f, pos.y + shadowOffset * 0.5f), shadow, text);
        dl->AddText(font, fontSize, ImVec2(pos.x, pos.y), color, text);
    }

    // ─── Decorative horizontal rule ───────────────────────────────────────────
    static void drawDivider(ImDrawList *dl, float x1, float x2, float y, float alpha)
    {
        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(0.96f, 0.82f, 0.50f, 0.50f * alpha));
        dl->AddLine(ImVec2(x1, y), ImVec2(x2, y), col, 2.0f);
    }

    // ─── onInitialize ─────────────────────────────────────────────────────────
    void onInitialize() override
    {
        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->Clear();

        titleFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Cinzel-Bold.ttf", 128.0f);
        buttonFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Rajdhani-SemiBold.ttf", 128.0f);
        if (!buttonFont)
        {
            buttonFont = io.Fonts->AddFontDefault();
        }
        if (!titleFont)
        {
            titleFont = io.Fonts->AddFontDefault();
        }

        getApp()->rebuildImGuiFonts();

        // Background image material
        menuMaterial = new our::TexturedMaterial();
        menuMaterial->shader = new our::ShaderProgram();
        menuMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        menuMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        menuMaterial->shader->link();
        menuMaterial->texture = our::texture_utils::loadImage("assets/textures/dusty-menu.jpeg");
        menuMaterial->tint = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

        cupMaterial = new our::TexturedMaterial();
        cupMaterial->shader = new our::ShaderProgram();
        cupMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        cupMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        cupMaterial->shader->link();
        cupMaterial->texture = our::texture_utils::loadImage("assets/textures/cup.png");
        cupMaterial->tint = glm::vec4(1.0f);
        cupMaterial->pipelineState.blending.enabled = true;
        cupMaterial->pipelineState.blending.equation = GL_FUNC_ADD;
        cupMaterial->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
        cupMaterial->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;

        // Dark overlay — standard alpha blending, tint driven per-frame
        darkOverlay = new our::TintedMaterial();
        darkOverlay->shader = new our::ShaderProgram();
        darkOverlay->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        darkOverlay->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        darkOverlay->shader->link();
        darkOverlay->tint = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        darkOverlay->pipelineState.blending.enabled = true;
        darkOverlay->pipelineState.blending.equation = GL_FUNC_ADD;
        darkOverlay->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
        darkOverlay->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;

        // Highlight material (subtract blend for inversion effect on hover)
        highlightMaterial = new our::TintedMaterial();
        highlightMaterial->shader = new our::ShaderProgram();
        highlightMaterial->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        highlightMaterial->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        highlightMaterial->shader->link();
        highlightMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        highlightMaterial->pipelineState.blending.enabled = true;
        highlightMaterial->pipelineState.blending.equation = GL_FUNC_SUBTRACT;
        highlightMaterial->pipelineState.blending.sourceFactor = GL_ONE;
        highlightMaterial->pipelineState.blending.destinationFactor = GL_ONE;

        rectangle = new our::Mesh(
            {
                {{0, 0, 0}, {255, 255, 255, 255}, {0, 1}, {0, 0, 1}},
                {{1, 0, 0}, {255, 255, 255, 255}, {1, 1}, {0, 0, 1}},
                {{1, 1, 0}, {255, 255, 255, 255}, {1, 0}, {0, 0, 1}},
                {{0, 1, 0}, {255, 255, 255, 255}, {0, 0}, {0, 0, 1}},
            },
            {0, 1, 2, 2, 3, 0});

        time = 0.0f;
        menuAudio.initialize();
        menuAudio.playLooping("assets/sounds/menu-start.mp3", 0.5f);

        buttons[0].action = [this]()
        { getApp()->changeState("play"); };
        buttons[1].action = [this]()
        { getApp()->close(); };
        leaderboardButton.action = [this]()
        { getApp()->changeState("leaderboard"); };
    }

    // ─── onImmediateGui ───────────────────────────────────────────────────────
    void onImmediateGui() override
    {
        glm::ivec2 fbSize = getApp()->getFrameBufferSize();
        updateLayout(fbSize);

        float titleFade = glm::smoothstep(0.35f, 1.10f, time);
        float btnFade = glm::smoothstep(0.55f, 1.30f, time);
        float pulse = 0.5f + 0.5f * std::sin(time * 3.5f);

        ImDrawList *dl = ImGui::GetForegroundDrawList();
        glm::vec2 mp = getApp()->getMouse().getMousePosition();
        bool leaderboardHovered = leaderboardButton.isInside(mp);

        // ── Title ────────────────────────────────────────────────────────────
        float sOff = titleFontSize * 0.045f;
        ImU32 titleShadow = ImGui::ColorConvertFloat4ToU32(
            ImVec4(0.08f, 0.04f, 0.01f, titleFade * 0.95f));
        ImU32 titleBase = ImGui::ColorConvertFloat4ToU32(
            ImVec4(0.98f, 0.85f, 0.40f, titleFade));
        ImU32 titleShine = ImGui::ColorConvertFloat4ToU32(
            ImVec4(1.00f, 0.97f, 0.80f, titleFade * 0.55f));

        dl->AddText(titleFont, titleFontSize,
                    ImVec2(titlePosition.x + sOff, titlePosition.y + sOff), titleShadow, GameTitle);
        dl->AddText(titleFont, titleFontSize,
                    ImVec2(titlePosition.x + sOff * 0.4f, titlePosition.y + sOff * 0.4f), titleShadow, GameTitle);
        dl->AddText(titleFont, titleFontSize,
                    ImVec2(titlePosition.x, titlePosition.y), titleBase, GameTitle);
        dl->AddText(titleFont, titleFontSize,
                    ImVec2(titlePosition.x - 1.0f, titlePosition.y - 1.5f), titleShine, GameTitle);

        // Divider
        float divY = titlePosition.y + titleSize.y + std::clamp(titleSize.y * 0.28f, 8.0f, 20.0f);
        float divHalfW = titleSize.x * 0.42f;
        float centerX = titlePosition.x + titleSize.x * 0.5f;
        drawDivider(dl, centerX - divHalfW, centerX + divHalfW, divY, titleFade);

        // ── Buttons ──────────────────────────────────────────────────────────
        for (size_t i = 0; i < buttons.size(); ++i)
        {
            bool hovered = buttons[i].isInside(mp);

            if (hovered)
            {
                float gAlpha = 0.20f + 0.08f * pulse;
                ImU32 hoverBg = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(0.96f, 0.82f, 0.30f, gAlpha * btnFade));
                dl->AddRectFilled(
                    ImVec2(buttons[i].position.x, buttons[i].position.y),
                    ImVec2(buttons[i].position.x + buttons[i].size.x,
                           buttons[i].position.y + buttons[i].size.y),
                    hoverBg, 10.0f);

                ImU32 hoverBorder = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(1.0f, 0.92f, 0.55f, (0.55f + 0.25f * pulse) * btnFade));
                dl->AddRect(
                    ImVec2(buttons[i].position.x, buttons[i].position.y),
                    ImVec2(buttons[i].position.x + buttons[i].size.x,
                           buttons[i].position.y + buttons[i].size.y),
                    hoverBorder, 10.0f, 0, 1.5f);
            }

            ImVec4 c = hovered
                           ? ImVec4(1.00f, 0.97f, 0.72f, btnFade)
                           : ImVec4(0.90f, 0.85f, 0.74f, btnFade * 0.85f);

            drawShadowedText(dl, buttonFont, buttonFontSize,
                             buttonTextPositions[i],
                             ImGui::ColorConvertFloat4ToU32(c),
                             ButtonLabels[i],
                             0.75f, buttonFontSize * 0.06f);
        }

        dl->AddText(buttonFont, leaderboardFontSize,
                    ImVec2(leaderboardLabelPosition.x, leaderboardLabelPosition.y),
                    ImGui::ColorConvertFloat4ToU32(
                        leaderboardHovered ? ImVec4(1.0f, 0.95f, 0.76f, btnFade)
                                           : ImVec4(0.93f, 0.88f, 0.74f, btnFade * 0.9f)),
                    LeaderboardLabel);
        dl->AddText(buttonFont, leaderboardFontSize * 0.82f,
                    ImVec2(leaderboardHintPosition.x, leaderboardHintPosition.y),
                    ImGui::ColorConvertFloat4ToU32(
                        leaderboardHovered ? ImVec4(1.0f, 0.88f, 0.42f, btnFade)
                                           : ImVec4(0.90f, 0.78f, 0.42f, btnFade * 0.85f)),
                    LeaderboardHint);
    }

    // ─── onDraw ───────────────────────────────────────────────────────────────
    void onDraw(double deltaTime) override
    {
        glm::ivec2 size = getApp()->getFrameBufferSize();
        updateLayout(size);

        auto &keyboard = getApp()->getKeyboard();
        if (keyboard.justPressed(GLFW_KEY_SPACE))
        {
            getApp()->changeState("play");
            return;
        }
        if (keyboard.justPressed(GLFW_KEY_ESCAPE))
        {
            getApp()->close();
            return;
        }
        if (keyboard.justPressed(GLFW_KEY_L))
        {
            getApp()->changeState("leaderboard");
            return;
        }

        auto &mouse = getApp()->getMouse();
        glm::vec2 mousePosition = mouse.getMousePosition();

        if (mouse.justPressed(0))
        {
            for (auto &b : buttons)
                if (b.isInside(mousePosition))
                    b.action();
            if (leaderboardButton.isInside(mousePosition))
                leaderboardButton.action();
        }

        glViewport(0, 0, size.x, size.y);
        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        glm::mat4 M = glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        time += (float)deltaTime;

        // 1. Background image fades in over 2 seconds
        menuMaterial->tint = glm::vec4(glm::smoothstep(0.00f, 2.00f, time));
        menuMaterial->setup();
        menuMaterial->shader->set("transform", VP * M);
        rectangle->draw();

        // 2. Dark overlay fades in from t=1 to t=2.5, settling at 72% opacity
        //    This dims the photo so the text pops without hiding the scene entirely
        float overlayAlpha = glm::smoothstep(1.0f, 2.5f, time) * 0.72f;
        darkOverlay->tint = glm::vec4(0.0f, 0.0f, 0.0f, overlayAlpha);
        darkOverlay->setup();
        darkOverlay->shader->set("transform", VP * M);
        rectangle->draw();

        float cupPulse = 0.94f + 0.06f * (0.5f + 0.5f * std::sin(time * 3.0f));
        Button cupDrawButton = leaderboardButton;
        glm::vec2 extra = leaderboardButton.size * (cupPulse - 1.0f) * 0.5f;
        cupDrawButton.position -= extra;
        cupDrawButton.size *= cupPulse;
        cupMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, glm::smoothstep(0.45f, 1.30f, time));
        cupMaterial->setup();
        cupMaterial->shader->set("transform", VP * cupDrawButton.getLocalToWorld());
        rectangle->draw();

        // 3. Hover highlight (subtract blend) on top of everything
        for (auto &button : buttons)
        {
            if (button.isInside(mousePosition))
            {
                highlightMaterial->setup();
                highlightMaterial->shader->set("transform", VP * button.getLocalToWorld());
                rectangle->draw();
            }
        }
    }

    // ─── onDestroy ────────────────────────────────────────────────────────────
    void onDestroy() override
    {
        menuAudio.destroy();
        delete rectangle;
        delete menuMaterial->texture;
        delete menuMaterial->shader;
        delete menuMaterial;
        delete cupMaterial->texture;
        delete cupMaterial->shader;
        delete cupMaterial;
        delete darkOverlay->shader;
        delete darkOverlay;
        delete highlightMaterial->shader;
        delete highlightMaterial;
        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->AddFontDefault();
        getApp()->rebuildImGuiFonts();
        titleFont = nullptr;
        buttonFont = nullptr;
    }
};
