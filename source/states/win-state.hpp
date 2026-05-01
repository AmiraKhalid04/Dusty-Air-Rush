#pragma once

#include <application.hpp>
#include <shader/shader.hpp>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <systems/audio-system.hpp>
#include <systems/score-manager.hpp>
#include "play-state.hpp"
#include <imgui.h>

#include <functional>
#include <array>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <utility>

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
    // ── Labels ────────────────────────────────────────────────────────────────
    static constexpr const char *ScreenTitle = "YOU WON!!";
    static constexpr const char *ScoreLabel = "YOUR SCORE";
    static constexpr const char *TopLabel = "TOP SCORES";
    static constexpr const char *NewBestLabel = "NEW BEST!";
    static constexpr const char *HintPlay = "PRESS  SPACE  TO PLAY AGAIN";
    static constexpr const char *HintEsc = "PRESS  ESC  TO EXIT";

    // ── Scores ────────────────────────────────────────────────────────────────
    int currentScore = 0;
    bool isNewBest = false;
    std::vector<int> topScores;

    // ── GL resources ──────────────────────────────────────────────────────────
    our::TexturedMaterial *backgroundMaterial = nullptr;
    our::TintedMaterial *darkOverlay = nullptr;
    our::TintedMaterial *goldOverlay = nullptr;
    our::TintedMaterial *highlightMaterial = nullptr;
    our::Mesh *rectangle = nullptr;
    float time = 0.0f;

    // ── Buttons (ESC=0 left, PLAY=1 right) ───────────────────────────────────
    std::array<WinButton, 2> buttons{};

    // ── Cached layout values ──────────────────────────────────────────────────
    float titleFontSize = 72.0f;
    float labelFontSize = 28.0f;
    float heroFontSize = 96.0f;
    float tableFontSize = 32.0f;
    float hintFontSize = 24.0f;

    ImFont *titleFont = nullptr;
    ImFont *scoreFont = nullptr;
    ImFont *buttonFont = nullptr;

    // Zones
    float zoneTitle = 0.0f;
    float zoneScore = 0.0f;
    float zoneTable = 0.0f;
    float zoneHint = 0.0f;
    float panelX = 0.0f;
    float panelW = 0.0f;
    float tableX = 0.0f;
    float tableW = 0.0f;

    our::AudioSystem winAudio;

    // ── Star particles ────────────────────────────────────────────────────────
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

    // ── Layout ────────────────────────────────────────────────────────────────
    void updateLayout(const glm::ivec2 &fb)
    {
        float W = (float)fb.x;
        float H = (float)fb.y;

        titleFontSize = std::clamp(H * 0.112f, 66.0f, 122.0f);
        labelFontSize = std::clamp(H * 0.058f, 34.0f, 58.0f);
        heroFontSize = std::clamp(H * 0.138f, 84.0f, 144.0f);
        tableFontSize = std::clamp(H * 0.041f, 25.0f, 44.0f);
        hintFontSize = std::clamp(H * 0.045f, 26.0f, 50.0f);

        panelW = std::clamp(W * 0.90f, 560.0f, 1200.0f);
        panelX = (W - panelW) * 0.5f;
        tableW = std::clamp(W * 0.66f, 520.0f, 860.0f);
        tableX = (W - tableW) * 0.5f;

        float margin = std::clamp(H * 0.012f, 8.0f, 14.0f);
        float topInset = std::clamp(H * 0.020f, 14.0f, 22.0f);
        float bottomInset = std::clamp(H * 0.022f, 14.0f, 22.0f);
        float titleH = titleFontSize * 1.32f;
        float scoreH = heroFontSize * 1.82f;
        float rowH = tableFontSize * 1.78f;
        int numRows = static_cast<int>(our::ScoreManager::MaxScores);
        float tableH = rowH * (numRows + 2.05f);
        float hintH = hintFontSize * 2.65f;

        float contentH = titleH + scoreH + tableH + hintH + margin * 3.0f;
        float availableH = H - topInset - bottomInset;
        if (contentH > availableH)
        {
            float scale = availableH / contentH;
            titleFontSize *= scale;
            labelFontSize *= scale;
            heroFontSize *= scale;
            tableFontSize *= scale;
            hintFontSize *= scale;
            margin *= std::clamp(scale * 1.02f, 0.70f, 1.0f);

            titleH = titleFontSize * 1.32f;
            scoreH = heroFontSize * 1.82f;
            rowH = tableFontSize * 1.78f;
            tableH = rowH * (numRows + 2.05f);
            hintH = hintFontSize * 2.65f;
        }

        zoneTitle = topInset;
        zoneScore = zoneTitle + titleH + margin * 0.75f;
        zoneTable = zoneScore + scoreH + margin * 1.55f;
        zoneHint = zoneTable + tableH + margin;
        zoneHint = std::min(zoneHint, H - bottomInset - hintH);

        // Invisible buttons for keyboard/mouse hit-testing
        buttons[0].size = {panelW * 0.47f, hintH};
        buttons[1].size = {panelW * 0.47f, hintH};
        buttons[0].position = {panelX, zoneHint};
        buttons[1].position = {panelX + panelW * 0.53f, zoneHint};
    }

    // ── Helpers ───────────────────────────────────────────────────────────────
    static void drawShadowedText(ImDrawList *dl, ImFont *font, float fontSize,
                                 const glm::vec2 &pos, ImU32 color, const char *text,
                                 float shadowMul = 0.85f, float shadowOff = 4.0f)
    {
        float alpha = ((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
        ImU32 shadow = ImGui::ColorConvertFloat4ToU32(
            ImVec4(0.04f, 0.02f, 0.0f, alpha * shadowMul));
        dl->AddText(font, fontSize, ImVec2(pos.x + shadowOff, pos.y + shadowOff), shadow, text);
        dl->AddText(font, fontSize, ImVec2(pos.x + shadowOff * 0.5f, pos.y + shadowOff * 0.5f), shadow, text);
        dl->AddText(font, fontSize, ImVec2(pos.x, pos.y), color, text);
    }

    static void drawGoldDivider(ImDrawList *dl, float x1, float x2, float y, float alpha)
    {
        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.82f, 0.22f, 0.65f * alpha));
        dl->AddLine(ImVec2(x1, y), ImVec2(x2, y), col, 2.0f);
    }

    static float drawCenteredText(ImDrawList *dl, ImFont *font, float fontSize,
                                  float centerX, float y, ImU32 color, const char *text)
    {
        ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
        float x = centerX - ts.x * 0.5f;
        dl->AddText(font, fontSize, ImVec2(x, y), color, text);
        return x;
    }

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
            dl->AddTriangleFilled(ImVec2(cx, cy), pts[i], pts[(i + 2) % 8], col);
    }

    // ── onInitialize ──────────────────────────────────────────────────────────
    void onInitialize() override
    {
        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->Clear();
        titleFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Cinzel-Bold.ttf", 128.0f);
        scoreFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Rajdhani-Bold.ttf", 128.0f);
        buttonFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Rajdhani-SemiBold.ttf", 128.0f);
        if (!titleFont)
            titleFont = io.Fonts->AddFontDefault();
        if (!scoreFont)
            scoreFont = io.Fonts->AddFontDefault();
        if (!buttonFont)
            buttonFont = io.Fonts->AddFontDefault();
        getApp()->rebuildImGuiFonts();

        backgroundMaterial = new our::TexturedMaterial();
        backgroundMaterial->shader = new our::ShaderProgram();
        backgroundMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        backgroundMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        backgroundMaterial->shader->link();
        // backgroundMaterial->texture = our::texture_utils::loadImage("assets/textures/dusty-win.png");
        backgroundMaterial->tint = glm::vec4(0.0f);

        auto makeTinted = [&]() -> our::TintedMaterial *
        {
            auto *m = new our::TintedMaterial();
            m->shader = new our::ShaderProgram();
            m->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
            m->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
            m->shader->link();
            m->pipelineState.blending.enabled = true;
            m->pipelineState.blending.equation = GL_FUNC_ADD;
            m->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
            m->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;
            return m;
        };

        darkOverlay = makeTinted();

        goldOverlay = makeTinted();
        goldOverlay->pipelineState.blending.destinationFactor = GL_ONE;

        highlightMaterial = makeTinted();
        highlightMaterial->tint = glm::vec4(1.0f, 0.75f, 0.10f, 0.55f);
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

        currentScore = Playstate::lastScore;
        topScores = our::ScoreManager::getTopScores();
        isNewBest = !topScores.empty() && topScores[0] == currentScore;

        winAudio.initialize();
        winAudio.playSound("assets/sounds/finish-line.mp3");

        buttons[0].action = [this]()
        { getApp()->close(); };
        buttons[1].action = [this]()
        { getApp()->changeState("play"); };
    }

    // ── onImmediateGui ────────────────────────────────────────────────────────
    void onImmediateGui() override
    {
        glm::ivec2 fbSize = getApp()->getFrameBufferSize();
        updateLayout(fbSize);

        float W = (float)fbSize.x;
        float H = (float)fbSize.y;

        float titleFade = glm::smoothstep(0.10f, 0.70f, time);
        float scoreFade = glm::smoothstep(0.40f, 0.90f, time);
        float tableFade = glm::smoothstep(0.60f, 1.20f, time);
        float btnFade = glm::smoothstep(0.80f, 1.40f, time);
        float shimmer = 0.5f + 0.5f * std::sin(time * 6.5f);
        float pulse = 0.5f + 0.5f * std::sin(time * 3.8f);

        ImDrawList *dl = ImGui::GetForegroundDrawList();
        glm::vec2 mp = getApp()->getMouse().getMousePosition();

        ImFont *sf = scoreFont ? scoreFont : ImGui::GetFont();
        ImFont *tf = titleFont ? titleFont : ImGui::GetFont();
        ImFont *bf = buttonFont ? buttonFont : ImGui::GetFont();

        // ── Star particles ────────────────────────────────────────────────────
        for (auto &s : stars)
        {
            float twinkle = 0.5f + 0.5f * std::sin(time * s.speed * 3.0f + s.phase);
            float alpha = titleFade * twinkle * 0.70f;
            if (alpha < 0.02f)
                continue;
            ImU32 col = ImGui::ColorConvertFloat4ToU32(
                ImVec4(1.0f, 0.88f + twinkle * 0.12f, 0.35f + twinkle * 0.35f, alpha));
            drawStar4(dl, s.x, s.y, s.size, col);
        }

        // ═══════════════════════════════════════════════════════════════════════
        //  TITLE STRIP
        // ═══════════════════════════════════════════════════════════════════════
        if (titleFade > 0.01f)
        {
            float centerX = W * 0.5f;
            float titleY = zoneTitle;

            ImU32 titleShadow = ImGui::ColorConvertFloat4ToU32(ImVec4(0.18f, 0.09f, 0.0f, titleFade));
            ImU32 titleBase = ImGui::ColorConvertFloat4ToU32(
                ImVec4(1.0f, 0.76f + shimmer * 0.14f, 0.06f, titleFade));
            ImU32 titleHot = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.98f, 0.82f, titleFade * 0.50f));

            ImVec2 ts = tf->CalcTextSizeA(titleFontSize, FLT_MAX, 0.0f, ScreenTitle);
            float tx = centerX - ts.x * 0.5f;
            float so = titleFontSize * 0.048f;

            dl->AddText(tf, titleFontSize, ImVec2(tx + so, titleY + so), titleShadow, ScreenTitle);
            dl->AddText(tf, titleFontSize, ImVec2(tx, titleY), titleBase, ScreenTitle);
            dl->AddText(tf, titleFontSize, ImVec2(tx - 1.0f, titleY - 1.0f), titleHot, ScreenTitle);

            float divW = ts.x * 0.55f;
            float divY = titleY + ts.y + titleFontSize * 0.12f;
            drawGoldDivider(dl, centerX - divW, centerX + divW, divY, titleFade);
        }

        // ═══════════════════════════════════════════════════════════════════════
        //   HERO SCORE
        // ═══════════════════════════════════════════════════════════════════════
        if (scoreFade > 0.01f)
        {
            float centerX = W * 0.5f;
            float panelH = heroFontSize * 1.82f;

            // Subtle gold border
            ImU32 panelBorder = ImGui::ColorConvertFloat4ToU32(
                ImVec4(1.0f, 0.80f, 0.22f, scoreFade * 0.40f));
            dl->AddRect(
                ImVec2(panelX, zoneScore),
                ImVec2(panelX + panelW, zoneScore + panelH),
                panelBorder, 10.0f, 0, 1.0f);

            // "YOUR SCORE" label
            float lfs = labelFontSize;
            ImU32 lblCol = ImGui::ColorConvertFloat4ToU32(
                ImVec4(1.0f, 0.80f, 0.30f, scoreFade * 0.90f));
            float lblY = zoneScore + panelH * 0.17f;
            ImVec2 lts = sf->CalcTextSizeA(lfs, FLT_MAX, 0.0f, ScoreLabel);
            float lx = centerX - lts.x * 0.5f;
            dl->AddText(sf, lfs, ImVec2(lx, lblY), lblCol, ScoreLabel);
            float ulY = lblY + lts.y + 3.0f;
            ImU32 ulCol = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.78f, 0.22f, scoreFade * 0.65f));
            dl->AddLine(ImVec2(lx - 4.0f, ulY), ImVec2(lx + lts.x + 4.0f, ulY), ulCol, 1.2f);

            //  score number
            char scoreBuf[32];
            snprintf(scoreBuf, sizeof(scoreBuf), "%d", currentScore);
            ImU32 valCol = ImGui::ColorConvertFloat4ToU32(
                ImVec4(1.0f, 0.93f + shimmer * 0.07f, 0.55f, scoreFade));
            glm::vec2 vp;
            {
                ImVec2 vs = sf->CalcTextSizeA(heroFontSize, FLT_MAX, 0.0f, scoreBuf);
                vp = {centerX - vs.x * 0.5f, ulY + heroFontSize * 0.08f};
            }
            drawShadowedText(dl, sf, heroFontSize, vp, valCol, scoreBuf,
                             0.80f, heroFontSize * 0.045f);

            // "NEW BEST!" badge
            if (isNewBest && scoreFade > 0.3f)
            {
                float bfs = labelFontSize * 0.85f;
                ImVec2 bts = sf->CalcTextSizeA(bfs, FLT_MAX, 0.0f, NewBestLabel);
                float badgeX = panelX + panelW - bts.x - labelFontSize * 1.4f;
                float badgeY = zoneScore + panelH * 0.16f;
                float badgeW = bts.x + labelFontSize * 0.8f;
                float badgeH = bts.y + labelFontSize * 0.4f;

                float badgePulse = 0.70f + 0.30f * pulse;
                ImU32 badgeBg = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(0.12f, 0.34f, 0.10f, scoreFade * badgePulse * 0.80f));
                ImU32 badgeBdr = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(0.35f, 0.90f, 0.30f, scoreFade * badgePulse));
                ImU32 badgeTxt = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(0.45f, 1.00f, 0.40f, scoreFade));

                dl->AddRectFilled(ImVec2(badgeX, badgeY),
                                  ImVec2(badgeX + badgeW, badgeY + badgeH),
                                  badgeBg, badgeH * 0.5f);
                dl->AddRect(ImVec2(badgeX, badgeY),
                            ImVec2(badgeX + badgeW, badgeY + badgeH),
                            badgeBdr, badgeH * 0.5f, 0, 1.2f);
                dl->AddText(sf, bfs,
                            ImVec2(badgeX + (badgeW - bts.x) * 0.5f,
                                   badgeY + (badgeH - bts.y) * 0.5f),
                            badgeTxt, NewBestLabel);
            }
        }

        // ═══════════════════════════════════════════════════════════════════════
        // LEADERBOARD TABLE
        // ═══════════════════════════════════════════════════════════════════════
        if (tableFade > 0.01f)
        {
            int maxRows = our::ScoreManager::MaxScores;
            float rowH = tableFontSize * 1.82f;
            float headerH = rowH * 1.06f;
            float tableH = rowH * (maxRows + 2.05f);
            float tblRight = tableX + tableW;
            float centerX = tableX + tableW * 0.5f;

            float colRankX = centerX - tableW * 0.16f;
            float colScoreX = centerX + tableW * 0.16f;

            // Gold border
            ImU32 tblBdr = ImGui::ColorConvertFloat4ToU32(
                ImVec4(1.0f, 0.80f, 0.22f, tableFade * 0.35f));
            dl->AddRect(ImVec2(tableX, zoneTable),
                        ImVec2(tblRight, zoneTable + tableH),
                        tblBdr, 10.0f, 0, 1.0f);

            // "TOP SCORES" label
            float lfs = labelFontSize;
            ImU32 lblCol = ImGui::ColorConvertFloat4ToU32(
                ImVec4(1.0f, 0.80f, 0.30f, tableFade * 0.90f));
            float lblY = zoneTable + rowH * 0.20f;
            ImVec2 lts = sf->CalcTextSizeA(lfs, FLT_MAX, 0.0f, TopLabel);
            float lx = tableX + (tableW - lts.x) * 0.5f;
            dl->AddText(sf, lfs, ImVec2(lx, lblY), lblCol, TopLabel);
            float ulY = lblY + lts.y + 3.0f;
            ImU32 ulCol = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.78f, 0.22f, tableFade * 0.65f));
            dl->AddLine(ImVec2(lx - 4.0f, ulY), ImVec2(lx + lts.x + 4.0f, ulY), ulCol, 1.2f);

            // Header row
            float hdrY = zoneTable + rowH * 1.02f;
            float hdrBot = hdrY + headerH;
            ImU32 hdrBg = ImGui::ColorConvertFloat4ToU32(
                ImVec4(1.0f, 0.82f, 0.22f, tableFade * 0.12f));
            dl->AddRectFilled(ImVec2(tableX + 2.0f, hdrY),
                              ImVec2(tblRight - 2.0f, hdrBot), hdrBg);

            float hfs = tableFontSize * 0.92f;
            ImU32 hdrCol = ImGui::ColorConvertFloat4ToU32(
                ImVec4(1.0f, 0.80f, 0.30f, tableFade * 0.80f));
            float hdrTextY = hdrY + (headerH - hfs) * 0.5f;

            // RANK header
            {
                ImVec2 ts = sf->CalcTextSizeA(hfs, FLT_MAX, 0.0f, "RANK");
                dl->AddText(sf, hfs, ImVec2(colRankX - ts.x * 0.5f, hdrTextY), hdrCol, "RANK");
            }
            // SCORE header
            {
                ImVec2 ts = sf->CalcTextSizeA(hfs, FLT_MAX, 0.0f, "SCORE");
                dl->AddText(sf, hfs, ImVec2(colScoreX - ts.x * 0.5f, hdrTextY), hdrCol, "SCORE");
            }

            // Divider under header
            ImU32 divCol = ImGui::ColorConvertFloat4ToU32(
                ImVec4(1.0f, 0.82f, 0.22f, tableFade * 0.30f));
            dl->AddLine(ImVec2(tableX + 12.0f, hdrBot),
                        ImVec2(tblRight - 12.0f, hdrBot), divCol, 1.0f);

            // Data rows
            for (int i = 0; i < maxRows; ++i)
            {
                bool hasEntry = i < (int)topScores.size();
                bool isCurrentRow = hasEntry && topScores[i] == currentScore &&
                                    i == 0 && isNewBest;

                float rowTop = hdrBot + i * rowH;
                float rowMid = rowTop + rowH * 0.5f;

                if (isCurrentRow)
                {
                    float fadeInRow = std::min(tableFade * 1.5f, 1.0f);
                    ImU32 rowHlBg = ImGui::ColorConvertFloat4ToU32(
                        ImVec4(1.0f, 0.82f, 0.22f, fadeInRow * (0.08f + pulse * 0.04f)));
                    dl->AddRectFilled(
                        ImVec2(tableX + 2.0f, rowTop),
                        ImVec2(tblRight - 2.0f, rowTop + rowH), rowHlBg);
                    ImU32 accentCol = ImGui::ColorConvertFloat4ToU32(
                        ImVec4(1.0f, 0.82f, 0.22f, fadeInRow));
                    dl->AddRectFilled(
                        ImVec2(tableX + 2.0f, rowTop + 2.0f),
                        ImVec2(tableX + 6.0f, rowTop + rowH - 2.0f),
                        accentCol, 2.0f);
                }

                if (i < maxRows - 1)
                {
                    float sepY = rowTop + rowH;
                    ImU32 sepCol = ImGui::ColorConvertFloat4ToU32(
                        ImVec4(1.0f, 0.80f, 0.22f, tableFade * 0.15f));
                    dl->AddLine(ImVec2(tableX + 16.0f, sepY),
                                ImVec2(tblRight - 16.0f, sepY), sepCol, 0.5f);
                }

                float rfs = tableFontSize * 1.18f;
                float textY = rowMid - rfs * 0.5f;

                ImU32 rankCol, scoreCol;
                if (isCurrentRow)
                {
                    rankCol = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.92f + shimmer * 0.08f, 0.50f, tableFade));
                    scoreCol = rankCol;
                }
                else if (hasEntry)
                {
                    rankCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0.85f, 0.76f, 0.45f, tableFade * 0.90f));
                    scoreCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0.95f, 0.88f, 0.62f, tableFade * 0.90f));
                }
                else
                {
                    rankCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0.45f, 0.40f, 0.25f, tableFade * 0.55f));
                    scoreCol = rankCol;
                }

                // RANK column
                char rankBuf[16];
                snprintf(rankBuf, sizeof(rankBuf), "#%d", i + 1);
                {
                    ImVec2 ts = sf->CalcTextSizeA(rfs, FLT_MAX, 0.0f, rankBuf);
                    float rx = colRankX - ts.x * 0.5f;
                    if (isCurrentRow)
                        drawShadowedText(dl, sf, rfs, {rx, textY}, rankCol, rankBuf, 0.75f, rfs * 0.04f);
                    else
                        dl->AddText(sf, rfs, ImVec2(rx, textY), rankCol, rankBuf);
                }

                // SCORE column
                char scoreBuf[32];
                if (hasEntry)
                    snprintf(scoreBuf, sizeof(scoreBuf), "%d", topScores[i]);
                else
                    snprintf(scoreBuf, sizeof(scoreBuf), "---");
                {
                    ImVec2 ts = sf->CalcTextSizeA(rfs, FLT_MAX, 0.0f, scoreBuf);
                    float sx = colScoreX - ts.x * 0.5f;
                    if (isCurrentRow)
                        drawShadowedText(dl, sf, rfs, {sx, textY}, scoreCol, scoreBuf, 0.75f, rfs * 0.04f);
                    else
                        dl->AddText(sf, rfs, ImVec2(sx, textY), scoreCol, scoreBuf);
                }
            }
        }

        // ═══════════════════════════════════════════════════════════════════════
        //  HINT TEXT
        // ═══════════════════════════════════════════════════════════════════════
        if (btnFade > 0.01f)
        {
            float hfs = hintFontSize * 1.10f;
            float hoverPulse = 0.75f + 0.25f * pulse;
            float hintPadX = std::max(18.0f, buttons[0].size.x * 0.06f);

            auto fitHint = [&](const char *text) -> std::pair<float, ImVec2>
            {
                float fittedSize = hfs;
                ImVec2 size = bf->CalcTextSizeA(fittedSize, FLT_MAX, 0.0f, text);
                float maxWidth = buttons[0].size.x - hintPadX * 2.0f;
                if (size.x > maxWidth && size.x > 0.0f)
                {
                    fittedSize *= maxWidth / size.x;
                    size = bf->CalcTextSizeA(fittedSize, FLT_MAX, 0.0f, text);
                }
                return {fittedSize, size};
            };

            // ESC hint — left side
            {
                bool hov = buttons[0].isInside(mp);
                float alpha = btnFade * (hov ? hoverPulse : 0.70f);
                ImU32 col = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(1.0f, 0.88f + (hov ? shimmer * 0.12f : 0.0f), 0.40f, alpha));
                auto [fontSize, ts] = fitHint(HintEsc);
                float tx = buttons[0].position.x + (buttons[0].size.x - ts.x) * 0.5f;
                float ty = zoneHint + (buttons[0].size.y - ts.y) * 0.5f;
                if (hov)
                    drawShadowedText(dl, bf, fontSize, {tx, ty}, col, HintEsc, 0.70f, fontSize * 0.04f);
                else
                    dl->AddText(bf, fontSize, ImVec2(tx, ty), col, HintEsc);
            }

            // SPACE hint — right side
            {
                bool hov = buttons[1].isInside(mp);
                float alpha = btnFade * (hov ? hoverPulse : 0.70f);
                ImU32 col = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(1.0f, 0.88f + (hov ? shimmer * 0.12f : 0.0f), 0.40f, alpha));
                auto [fontSize, ts] = fitHint(HintPlay);
                float tx = buttons[1].position.x + (buttons[1].size.x - ts.x) * 0.5f;
                float ty = zoneHint + (buttons[1].size.y - ts.y) * 0.5f;
                if (hov)
                    drawShadowedText(dl, bf, fontSize, {tx, ty}, col, HintPlay, 0.70f, fontSize * 0.04f);
                else
                    dl->AddText(bf, fontSize, ImVec2(tx, ty), col, HintPlay);
            }

            // Subtle divider between the two hints
            float divX = panelX + panelW * 0.5f;
            float divY1 = zoneHint + buttons[0].size.y * 0.15f;
            float divY2 = zoneHint + buttons[0].size.y * 0.85f;
            ImU32 divC = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.78f, 0.22f, btnFade * 0.25f));
            dl->AddLine(ImVec2(divX, divY1), ImVec2(divX, divY2), divC, 1.0f);
        }
    }

    // ── onDraw ────────────────────────────────────────────────────────────────
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
        glm::vec2 mousePos = mouse.getMousePosition();
        if (mouse.justPressed(0))
            for (auto &btn : buttons)
                if (btn.isInside(mousePos))
                    btn.action();

        glViewport(0, 0, size.x, size.y);
        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        glm::mat4 fullScreen = glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

        time += (float)deltaTime;

        // Background fade-in
        backgroundMaterial->tint = glm::vec4(glm::smoothstep(0.0f, 1.2f, time));
        backgroundMaterial->setup();
        backgroundMaterial->shader->set("transform", VP * fullScreen);
        rectangle->draw();

        // Dark overlay
        float darkAlpha = glm::smoothstep(0.15f, 1.10f, time) * 0.40f;
        darkOverlay->tint = glm::vec4(0.00f, 0.01f, 0.04f, darkAlpha);
        darkOverlay->setup();
        darkOverlay->shader->set("transform", VP * fullScreen);
        rectangle->draw();

        // Pulsing gold glow overlay
        float pulse = 0.5f + 0.5f * std::sin(time * 2.8f);
        float goldAlpha = glm::smoothstep(0.50f, 1.60f, time) * (0.08f + pulse * 0.05f);
        goldOverlay->tint = glm::vec4(1.0f, 0.78f, 0.12f, goldAlpha);
        goldOverlay->setup();
        goldOverlay->shader->set("transform", VP * fullScreen);
        rectangle->draw();

        // Button hover highlight
        for (auto &btn : buttons)
        {
            if (btn.isInside(mousePos))
            {
                highlightMaterial->setup();
                highlightMaterial->shader->set("transform", VP * btn.getLocalToWorld());
                rectangle->draw();
            }
        }
    }

    // ── onDestroy ─────────────────────────────────────────────────────────────
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
        titleFont = scoreFont = buttonFont = nullptr;
    }
};
