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

struct WinButton
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

class WinState : public our::State
{
    static constexpr const char *ScreenTitle = "YOU WON!!";
    static constexpr const char *ScoreLabel = "YOUR SCORE";
    static constexpr const char *BestLabel = "BEST SCORE";
    static constexpr std::array<const char *, 2> ButtonLabels = {
        "PRESS SPACE TO PLAY AGAIN",
        "PRESS ESC TO EXIT"};

    // ── placeholder values – swap these out later ──────────────────────────
    int currentScore = 9420;
    int bestScore = 12750;
    // ───────────────────────────────────────────────────────────────────────

    our::TexturedMaterial *backgroundMaterial = nullptr;
    our::TintedMaterial *darkOverlay = nullptr;
    our::TintedMaterial *goldOverlay = nullptr;
    our::TintedMaterial *highlightMaterial = nullptr;
    our::Mesh *rectangle = nullptr;
    float time = 0.0f;

    std::array<WinButton, 2> buttons{};
    std::array<glm::vec2, 2> buttonTextPositions{};
    glm::vec2 titlePosition{};
    glm::vec2 titleSize{};
    glm::vec2 scorePanelPosition{};
    glm::vec2 scorePanelSize{};
    float dividerY = 0.0f;

    float titleFontSize = 96.0f;
    float scoreFontSize = 52.0f;
    float buttonFontSize = 44.0f;
    ImFont *titleFont = nullptr;
    ImFont *scoreFont = nullptr;
    ImFont *buttonFont = nullptr;

    our::AudioSystem winAudio;

    // ─── star particle state ─────────────────────────────────────────────────
    struct Star
    {
        float x, y, speed, size, phase;
    };
    static constexpr int StarCount = 64;
    std::array<Star, StarCount> stars{};

    void initStars(float W, float H)
    {
        for (auto &s : stars)
        {
            s.x = static_cast<float>(rand() % (int)W);
            s.y = static_cast<float>(rand() % (int)H);
            s.speed = 0.4f + static_cast<float>(rand() % 100) / 100.0f * 1.2f;
            s.size = 1.8f + static_cast<float>(rand() % 100) / 100.0f * 3.0f;
            s.phase = static_cast<float>(rand() % 628) / 100.0f;
        }
    }

    // ─── Layout ──────────────────────────────────────────────────────────────
    void updateLayout(const glm::ivec2 &fbSize)
    {
        ImFont *lTitleFont = titleFont ? titleFont : ImGui::GetFont();
        ImFont *lButtonFont = buttonFont ? buttonFont : lTitleFont;
        float W = (float)fbSize.x;
        float H = (float)fbSize.y;

        titleFontSize = std::clamp(H * 0.19f, 120.0f, 200.0f);
        scoreFontSize = std::clamp(H * 0.10f, 72.0f, 108.0f);
        buttonFontSize = std::clamp(H * 0.060f, 42.0f, 70.0f);

        ImVec2 ts = lTitleFont->CalcTextSizeA(titleFontSize, FLT_MAX, 0.0f, ScreenTitle);
        titleSize = {ts.x, ts.y};
        titlePosition = {(W - titleSize.x) * 0.5f, H * 0.22f};

        dividerY = titlePosition.y + titleSize.y + std::clamp(titleSize.y * 0.18f, 8.0f, 20.0f);

        // Score panel centered between divider and buttons
        float panelH = scoreFontSize * 2.6f;
        float panelW = std::clamp(W * 0.56f, 480.0f, 820.0f);
        scorePanelSize = {panelW, panelH};
        scorePanelPosition = {(W - panelW) * 0.5f, dividerY + std::clamp(H * 0.01f, 6.0f, 12.0f)};

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
            ImVec4(0.05f, 0.03f, 0.0f, alpha * shadowOpacityMul));
        dl->AddText(font, fontSize, ImVec2(pos.x + shadowOffset, pos.y + shadowOffset), shadow, text);
        dl->AddText(font, fontSize, ImVec2(pos.x + shadowOffset * 0.5f, pos.y + shadowOffset * 0.5f), shadow, text);
        dl->AddText(font, fontSize, ImVec2(pos.x, pos.y), color, text);
    }

    static void drawDivider(ImDrawList *dl, float x1, float x2, float y, float alpha)
    {
        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.82f, 0.22f, 0.70f * alpha));
        dl->AddLine(ImVec2(x1, y), ImVec2(x2, y), col, 3.0f);
    }

    // Draw a 4-pointed star (✦) via filled triangles
    static void drawStar4(ImDrawList *dl, float cx, float cy, float r, ImU32 col)
    {
        float inner = r * 0.38f;
        ImVec2 pts[8] = {
            {cx, cy - r},
            {cx + inner, cy - inner},
            {cx + r, cy},
            {cx + inner, cy + inner},
            {cx, cy + r},
            {cx - inner, cy + inner},
            {cx - r, cy},
            {cx - inner, cy - inner},
        };
        for (int i = 0; i < 8; i += 2)
        {
            int j = (i + 2) % 8;
            dl->AddTriangleFilled(ImVec2(cx, cy), pts[i], pts[j], col);
        }
    }

    // ─── onInitialize ─────────────────────────────────────────────────────────
    void onInitialize() override
    {
        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->Clear();
        titleFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Cinzel-Bold.ttf", 128.0f);
        scoreFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Rajdhani-Bold.ttf", 128.0f);
        buttonFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Rajdhani-SemiBold.ttf", 128.0f);
        if (!scoreFont)
            scoreFont = io.Fonts->AddFontDefault();
        if (!buttonFont)
            buttonFont = io.Fonts->AddFontDefault();
        if (!titleFont)
            titleFont = io.Fonts->AddFontDefault();
        getApp()->rebuildImGuiFonts();

        // Background – swap texture path to a suitable victory/celebration image
        backgroundMaterial = new our::TexturedMaterial();
        backgroundMaterial->shader = new our::ShaderProgram();
        backgroundMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        backgroundMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        backgroundMaterial->shader->link();
        backgroundMaterial->texture = our::texture_utils::loadImage("assets/textures/dusty-win.png");
        backgroundMaterial->tint = glm::vec4(0.0f);

        // Dark navy/deep-blue overlay
        darkOverlay = new our::TintedMaterial();
        darkOverlay->shader = new our::ShaderProgram();
        darkOverlay->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        darkOverlay->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        darkOverlay->shader->link();
        darkOverlay->pipelineState.blending.enabled = true;
        darkOverlay->pipelineState.blending.equation = GL_FUNC_ADD;
        darkOverlay->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
        darkOverlay->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;

        // Gold/amber additive glow overlay
        goldOverlay = new our::TintedMaterial();
        goldOverlay->shader = new our::ShaderProgram();
        goldOverlay->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        goldOverlay->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        goldOverlay->shader->link();
        goldOverlay->pipelineState.blending.enabled = true;
        goldOverlay->pipelineState.blending.equation = GL_FUNC_ADD;
        goldOverlay->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
        goldOverlay->pipelineState.blending.destinationFactor = GL_ONE;

        // Button hover highlight (gold additive)
        highlightMaterial = new our::TintedMaterial();
        highlightMaterial->shader = new our::ShaderProgram();
        highlightMaterial->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
        highlightMaterial->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
        highlightMaterial->shader->link();
        highlightMaterial->tint = glm::vec4(1.0f, 0.75f, 0.10f, 0.65f);
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
        srand(42);
        glm::ivec2 fb = getApp()->getFrameBufferSize();
        initStars((float)fb.x, (float)fb.y);

        winAudio.initialize();
        winAudio.playLooping("assets/sounds/win-fanfare.mp3", 0.40f);

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

        float W = (float)fbSize.x;
        float H = (float)fbSize.y;

        float titleFade = glm::smoothstep(0.10f, 0.85f, time);
        float scoreFade = glm::smoothstep(0.50f, 1.10f, time);
        float btnFade = glm::smoothstep(0.70f, 1.40f, time);
        float pulse = 0.5f + 0.5f * std::sin(time * 3.8f);
        float shimmer = 0.5f + 0.5f * std::sin(time * 6.5f); // faster gold shimmer

        ImDrawList *dl = ImGui::GetForegroundDrawList();
        glm::vec2 mp = getApp()->getMouse().getMousePosition();

        // ── Floating star particles ────────────────────────────────────────────
        for (auto &s : stars)
        {
            float twinkle = 0.5f + 0.5f * std::sin(time * s.speed * 3.0f + s.phase);
            float alpha = titleFade * twinkle * 0.75f;
            if (alpha <= 0.01f)
                continue;
            ImU32 col = ImGui::ColorConvertFloat4ToU32(
                ImVec4(1.0f, 0.90f + twinkle * 0.10f, 0.40f + twinkle * 0.30f, alpha));
            drawStar4(dl, s.x, s.y, s.size, col);
        }

        // ── Title ─────────────────────────────────────────────────────────────
        float sOff = titleFontSize * 0.055f;

        // Deep amber shadow
        ImU32 titleShadow = ImGui::ColorConvertFloat4ToU32(
            ImVec4(0.20f, 0.10f, 0.00f, titleFade));
        // Primary gold
        ImU32 titleBase = ImGui::ColorConvertFloat4ToU32(
            ImVec4(1.00f, 0.78f + shimmer * 0.12f, 0.08f, titleFade));
        // Bright highlight layer
        ImU32 titleHot = ImGui::ColorConvertFloat4ToU32(
            ImVec4(1.00f, 0.98f, 0.80f, titleFade * 0.55f));

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

        // ── Score Panel ───────────────────────────────────────────────────────
        if (scoreFade > 0.01f)
        {
            ImFont *sf = scoreFont ? scoreFont : ImGui::GetFont();

            // Vertical separator only — no background box, no border
            float midX = scorePanelPosition.x + scorePanelSize.x * 0.5f;
            float sepTop = scorePanelPosition.y + scorePanelSize.y * 0.05f;
            float sepBot = scorePanelPosition.y + scorePanelSize.y * 0.95f;
            ImU32 sepCol = ImGui::ColorConvertFloat4ToU32(
                ImVec4(1.0f, 0.80f, 0.20f, scoreFade * 0.55f));
            dl->AddLine(ImVec2(midX, sepTop), ImVec2(midX, sepBot), sepCol, 2.0f);

            // Helper: draw label + value in a half-panel cell
            auto drawScoreCell = [&](float cellCenterX, const char *label, int value)
            {
                // Label
                float labelFontSize = scoreFontSize * 0.52f;
                ImVec2 ls = sf->CalcTextSizeA(labelFontSize, FLT_MAX, 0.0f, label);
                float lx = cellCenterX - ls.x * 0.5f;
                float ly = scorePanelPosition.y + scorePanelSize.y * 0.10f;
                ImU32 labelCol = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(1.0f, 0.80f, 0.30f, scoreFade * 0.92f));
                dl->AddText(sf, labelFontSize, ImVec2(lx, ly), labelCol, label);

                // Add underline right below the label text
                float underlineY = ly + ls.y + 3.0f;
                float underlinePad = ls.x * 0.08f;
                ImU32 underlineCol = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(1.0f, 0.78f, 0.22f, scoreFade * 0.70f));
                dl->AddLine(
                    ImVec2(lx - underlinePad, underlineY),
                    ImVec2(lx + ls.x + underlinePad, underlineY),
                    underlineCol, 1.5f);

                // Value
                char buf[32];
                snprintf(buf, sizeof(buf), "%d", value);
                ImVec2 vs = sf->CalcTextSizeA(scoreFontSize, FLT_MAX, 0.0f, buf);
                glm::vec2 vp = {cellCenterX - vs.x * 0.5f,
                                scorePanelPosition.y + scorePanelSize.y * 0.50f};
                ImU32 valCol = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(1.00f, 0.95f + shimmer * 0.05f, 0.60f, scoreFade));
                drawShadowedText(dl, sf, scoreFontSize, vp, valCol, buf,
                                 0.80f, scoreFontSize * 0.05f);
            };

            float leftCell = scorePanelPosition.x + scorePanelSize.x * 0.25f;
            float rightCell = scorePanelPosition.x + scorePanelSize.x * 0.75f;
            drawScoreCell(leftCell, ScoreLabel, currentScore);
            drawScoreCell(rightCell, BestLabel, bestScore);
        }

        // ── Buttons ───────────────────────────────────────────────────────────
        for (size_t i = 0; i < buttons.size(); ++i)
        {
            bool hovered = buttons[i].isInside(mp);

            ImVec2 bMin(buttons[i].position.x, buttons[i].position.y);
            ImVec2 bMax(buttons[i].position.x + buttons[i].size.x,
                        buttons[i].position.y + buttons[i].size.y);

            // Base background
            // ImU32 baseBg = ImGui::ColorConvertFloat4ToU32(
            //     ImVec4(0.08f, 0.06f, 0.01f, btnFade * 0.72f));
            // dl->AddRectFilled(bMin, bMax, baseBg, 12.0f);

            // // Border
            // ImU32 border = ImGui::ColorConvertFloat4ToU32(
            //     hovered ? ImVec4(1.0f, 0.90f, 0.30f, btnFade)
            //             : ImVec4(0.80f, 0.62f, 0.10f, btnFade * 0.85f));
            // dl->AddRect(bMin, bMax, border, 12.0f, 0, hovered ? 2.2f : 1.6f);

            // Pulsing hover fill
            if (hovered)
            {
                ImU32 hoverFill = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(0.95f, 0.72f, 0.06f, (0.14f + pulse * 0.10f) * btnFade));
                dl->AddRectFilled(bMin, bMax, hoverFill, 12.0f);
            }

            ImVec4 textColor = hovered
                                   ? ImVec4(1.00f, 0.98f, 0.88f, btnFade)
                                   : ImVec4(0.95f, 0.88f, 0.68f, btnFade * 0.92f);

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

        // Background fade-in
        backgroundMaterial->tint = glm::vec4(glm::smoothstep(0.0f, 1.2f, time));
        backgroundMaterial->setup();
        backgroundMaterial->shader->set("transform", VP * fullScreen);
        rectangle->draw();

        // Dark deep-blue overlay
        float darkAlpha = glm::smoothstep(0.15f, 1.10f, time) * 0.68f;
        darkOverlay->tint = glm::vec4(0.00f, 0.01f, 0.04f, darkAlpha);
        darkOverlay->setup();
        darkOverlay->shader->set("transform", VP * fullScreen);
        rectangle->draw();

        // Pulsing gold glow overlay
        float pulse = 0.5f + 0.5f * std::sin(time * 2.8f);
        float goldAlpha = glm::smoothstep(0.50f, 1.60f, time) * (0.10f + pulse * 0.06f);
        goldOverlay->tint = glm::vec4(1.0f, 0.78f, 0.12f, goldAlpha);
        goldOverlay->setup();
        goldOverlay->shader->set("transform", VP * fullScreen);
        rectangle->draw();

        // Button hover highlight
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
        winAudio.destroy();
        delete rectangle;
        delete backgroundMaterial->texture;
        delete backgroundMaterial->shader;
        delete backgroundMaterial;
        delete darkOverlay->shader;
        delete darkOverlay;
        delete goldOverlay->shader;
        delete goldOverlay;
        delete highlightMaterial->shader;
        delete highlightMaterial;

        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->AddFontDefault();
        getApp()->rebuildImGuiFonts();
        titleFont = nullptr;
        scoreFont = nullptr;
        buttonFont = nullptr;
    }
};