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

struct LossButton
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

class LossState : public our::State
{
    static constexpr const char *ScreenTitle = "GAME OVER";
    static constexpr std::array<const char *, 2> ButtonLabels = {
        "PRESS SPACE TO PLAY AGAIN",
        "PRESS ESC TO EXIT"};

    our::TexturedMaterial *backgroundMaterial = nullptr;
    our::TintedMaterial *darkOverlay = nullptr;
    our::TintedMaterial *accentOverlay = nullptr;
    our::TintedMaterial *highlightMaterial = nullptr;
    our::Mesh *rectangle = nullptr;
    float time = 0.0f;

    std::array<LossButton, 2> buttons{};
    std::array<glm::vec2, 2> buttonTextPositions{};
    glm::vec2 titlePosition{};
    glm::vec2 titleSize{};
    float dividerY = 0.0f; // ← computed in updateLayout, used in onImmediateGui

    float titleFontSize = 96.0f;
    float buttonFontSize = 44.0f;
    ImFont *titleFont = nullptr;
    ImFont *buttonFont = nullptr;

    our::AudioSystem lossAudio;

    // ─── Layout ──────────────────────────────────────────────────────────────
    void updateLayout(const glm::ivec2 &fbSize)
    {
        ImFont *lTitleFont = titleFont ? titleFont : ImGui::GetFont();
        ImFont *lButtonFont = buttonFont ? buttonFont : lTitleFont;
        float W = (float)fbSize.x;
        float H = (float)fbSize.y;

        //  title height
        titleFontSize = std::clamp(H * 0.19f, 120.0f, 200.0f);
        buttonFontSize = std::clamp(H * 0.060f, 42.0f, 70.0f);

        ImVec2 ts = lTitleFont->CalcTextSizeA(titleFontSize, FLT_MAX, 0.0f, ScreenTitle);
        titleSize = {ts.x, ts.y};

        titlePosition = {(W - titleSize.x) * 0.5f, H * 0.28f};

        // Divider sits below the title
        dividerY = titlePosition.y + titleSize.y + std::clamp(titleSize.y * 0.18f, 8.0f, 20.0f);

        float padX = std::clamp(W * 0.030f, 34.0f, 60.0f);
        float padY = std::clamp(H * 0.014f, 14.0f, 24.0f);
        float buttonStartY = H * 0.72f;
        float btnGap = std::clamp(H * 0.010f, 8.0f, 14.0f);
        float currentY = buttonStartY;

        for (size_t i = 0; i < buttons.size(); ++i)
        {
            ImVec2 bs = lButtonFont->CalcTextSizeA(buttonFontSize, FLT_MAX, 0.0f, ButtonLabels[i]);
            buttons[i].size = {bs.x + padX * 2.0f, bs.y + padY * 2.0f};
            buttons[i].position = {(W - buttons[i].size.x) * 0.5f, currentY};
            buttonTextPositions[i] = {
                buttons[i].position.x + (buttons[i].size.x - bs.x) * 0.5f,
                buttons[i].position.y + (buttons[i].size.y - bs.y) * 0.5f};
            currentY += buttons[i].size.y + btnGap;
        }
    }

    // ─── Helpers ─────────────────────────────────────────────────────────────
    static void drawShadowedText(ImDrawList *dl, ImFont *font, float fontSize,
                                 const glm::vec2 &pos, ImU32 color, const char *text,
                                 float shadowOpacityMul = 0.9f, float shadowOffset = 4.0f)
    {
        float alpha = ((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
        ImU32 shadow = ImGui::ColorConvertFloat4ToU32(
            ImVec4(0.03f, 0.0f, 0.0f, alpha * shadowOpacityMul));
        dl->AddText(font, fontSize, ImVec2(pos.x + shadowOffset, pos.y + shadowOffset), shadow, text);
        dl->AddText(font, fontSize, ImVec2(pos.x + shadowOffset * 0.5f, pos.y + shadowOffset * 0.5f), shadow, text);
        dl->AddText(font, fontSize, ImVec2(pos.x, pos.y), color, text);
    }

    static void drawDivider(ImDrawList *dl, float x1, float x2, float y, float alpha)
    {
        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.32f, 0.24f, 0.6f * alpha));
        dl->AddLine(ImVec2(x1, y), ImVec2(x2, y), col, 3.0f);
    }

    // ─── onInitialize ─────────────────────────────────────────────────────────
    void onInitialize() override
    {
        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->Clear();
        titleFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Cinzel-Bold.ttf", 128.0f);
        buttonFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Rajdhani-SemiBold.ttf", 128.0f);
        if (!buttonFont)
            buttonFont = io.Fonts->AddFontDefault();
        if (!titleFont)
            titleFont = io.Fonts->AddFontDefault();
        getApp()->rebuildImGuiFonts();

        backgroundMaterial = new our::TexturedMaterial();
        backgroundMaterial->shader = new our::ShaderProgram();
        backgroundMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        backgroundMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        backgroundMaterial->shader->link();
        backgroundMaterial->texture = our::texture_utils::loadImage("assets/textures/dusty-crash.png");
        backgroundMaterial->tint = glm::vec4(0.0f);

        darkOverlay = new our::TintedMaterial();
        darkOverlay->shader = new our::ShaderProgram();
        darkOverlay->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        darkOverlay->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        darkOverlay->shader->link();
        darkOverlay->pipelineState.blending.enabled = true;
        darkOverlay->pipelineState.blending.equation = GL_FUNC_ADD;
        darkOverlay->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
        darkOverlay->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;

        accentOverlay = new our::TintedMaterial();
        accentOverlay->shader = new our::ShaderProgram();
        accentOverlay->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        accentOverlay->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        accentOverlay->shader->link();
        accentOverlay->pipelineState.blending.enabled = true;
        accentOverlay->pipelineState.blending.equation = GL_FUNC_ADD;
        accentOverlay->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
        accentOverlay->pipelineState.blending.destinationFactor = GL_ONE;

        highlightMaterial = new our::TintedMaterial();
        highlightMaterial->shader = new our::ShaderProgram();
        highlightMaterial->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        highlightMaterial->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        highlightMaterial->shader->link();
        highlightMaterial->tint = glm::vec4(1.0f, 0.25f, 0.20f, 0.75f);
        highlightMaterial->pipelineState.blending.enabled = true;
        highlightMaterial->pipelineState.blending.equation = GL_FUNC_ADD;
        highlightMaterial->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
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
        lossAudio.initialize();
        lossAudio.playSound("assets/sounds/game-over.wav");

        buttons[0].action = [this]()
        { getApp()->changeState("play"); };
        buttons[1].action = [this]()
        { getApp()->close(); };
    }

    // ─── onImmediateGui ───────────────────────────────────────────────────────
    void onImmediateGui() override
    {
        glm::ivec2 fbSize = getApp()->getFrameBufferSize();
        updateLayout(fbSize);

        float titleFade = glm::smoothstep(0.10f, 0.85f, time);
        float btnFade = glm::smoothstep(0.55f, 1.20f, time);
        float pulse = 0.5f + 0.5f * std::sin(time * 4.2f);

        ImDrawList *dl = ImGui::GetForegroundDrawList();
        glm::vec2 mp = getApp()->getMouse().getMousePosition();

        // ── Title ─────────────────────────────────────────────────────────────
        float sOff = titleFontSize * 0.055f;
        ImU32 titleShadow = ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.00f, 0.00f, titleFade));
        ImU32 titleBase = ImGui::ColorConvertFloat4ToU32(ImVec4(1.00f, 0.35f, 0.22f, titleFade));
        ImU32 titleHot = ImGui::ColorConvertFloat4ToU32(ImVec4(1.00f, 0.82f, 0.73f, titleFade * 0.60f));

        dl->AddText(titleFont, titleFontSize,
                    ImVec2(titlePosition.x + sOff, titlePosition.y + sOff), titleShadow, ScreenTitle);
        dl->AddText(titleFont, titleFontSize,
                    ImVec2(titlePosition.x, titlePosition.y), titleBase, ScreenTitle);
        dl->AddText(titleFont, titleFontSize,
                    ImVec2(titlePosition.x - 1.0f, titlePosition.y - 1.0f), titleHot, ScreenTitle);

        // ── Divider ───────────────────────────────────────────────────────────
        float divHalfW = titleSize.x * 0.38f;
        float centerX = titlePosition.x + titleSize.x * 0.5f;
        drawDivider(dl, centerX - divHalfW, centerX + divHalfW, dividerY, titleFade);

        // ── Buttons ───────────────────────────────────────────────────────────
        for (size_t i = 0; i < buttons.size(); ++i)
        {
            bool hovered = buttons[i].isInside(mp);

            ImVec2 bMin(buttons[i].position.x, buttons[i].position.y);
            ImVec2 bMax(buttons[i].position.x + buttons[i].size.x,
                        buttons[i].position.y + buttons[i].size.y);

            // Base background
            ImU32 baseBg = ImGui::ColorConvertFloat4ToU32(
                ImVec4(0.16f, 0.03f, 0.03f, btnFade * 0.68f));
            dl->AddRectFilled(bMin, bMax, baseBg, 12.0f);

            // Border — brighter when hovered
            ImU32 border = ImGui::ColorConvertFloat4ToU32(
                hovered ? ImVec4(1.0f, 0.55f, 0.36f, btnFade)
                        : ImVec4(0.78f, 0.24f, 0.18f, btnFade * 0.85f));
            dl->AddRect(bMin, bMax, border, 12.0f, 0, hovered ? 2.2f : 1.6f);

            // Pulsing hover fill
            if (hovered)
            {
                ImU32 hoverFill = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(0.95f, 0.22f, 0.16f, (0.16f + pulse * 0.10f) * btnFade));
                dl->AddRectFilled(bMin, bMax, hoverFill, 12.0f);
            }

            ImVec4 textColor = hovered
                                   ? ImVec4(1.00f, 0.95f, 0.92f, btnFade)
                                   : ImVec4(0.95f, 0.78f, 0.74f, btnFade * 0.92f);

            drawShadowedText(dl, buttonFont, buttonFontSize,
                             buttonTextPositions[i],
                             ImGui::ColorConvertFloat4ToU32(textColor),
                             ButtonLabels[i],
                             0.85f, buttonFontSize * 0.06f);
        }
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

        auto &mouse = getApp()->getMouse();
        glm::vec2 mousePosition = mouse.getMousePosition();
        if (mouse.justPressed(0))
            for (auto &button : buttons)
                if (button.isInside(mousePosition))
                    button.action();

        glViewport(0, 0, size.x, size.y);
        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        glm::mat4 fullScreen = glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        time += (float)deltaTime;

        backgroundMaterial->tint = glm::vec4(glm::smoothstep(0.0f, 1.2f, time));
        backgroundMaterial->setup();
        backgroundMaterial->shader->set("transform", VP * fullScreen);
        rectangle->draw();

        float darkAlpha = glm::smoothstep(0.15f, 1.10f, time) * 0.74f;
        darkOverlay->tint = glm::vec4(0.02f, 0.00f, 0.00f, darkAlpha);
        darkOverlay->setup();
        darkOverlay->shader->set("transform", VP * fullScreen);
        rectangle->draw();

        float accentAlpha = glm::smoothstep(0.45f, 1.40f, time) * 0.18f;
        accentOverlay->tint = glm::vec4(0.90f, 0.10f, 0.06f, accentAlpha);
        accentOverlay->setup();
        accentOverlay->shader->set("transform", VP * fullScreen);
        rectangle->draw();

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
        lossAudio.destroy();
        delete rectangle;
        delete backgroundMaterial->texture;
        delete backgroundMaterial->shader;
        delete backgroundMaterial;
        delete darkOverlay->shader;
        delete darkOverlay;
        delete accentOverlay->shader;
        delete accentOverlay;
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