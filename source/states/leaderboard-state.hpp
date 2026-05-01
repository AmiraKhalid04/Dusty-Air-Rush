#pragma once

#include <application.hpp>
#include <shader/shader.hpp>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <systems/score-manager.hpp>
#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <functional>
#include <vector>

struct LeaderboardButton
{
    glm::vec2 position{}, size{};
    std::function<void()> action;

    bool isInside(const glm::vec2 &v) const
    {
        return position.x <= v.x && position.y <= v.y &&
               v.x <= position.x + size.x &&
               v.y <= position.y + size.y;
    }
};

class LeaderboardState : public our::State
{
    static constexpr const char *ScreenTitle = "LEADER BOARD";
    static constexpr const char *Subtitle = "LAST 5 WINS";
    static constexpr const char *HintBack = "PRESS ESC TO RETURN";

    our::TintedMaterial *backgroundMaterial = nullptr;
    our::TexturedMaterial *cupMaterial = nullptr;
    our::TintedMaterial *buttonMaterial = nullptr;
    our::Mesh *rectangle = nullptr;

    ImFont *titleFont = nullptr;
    ImFont *bodyFont = nullptr;

    float time = 0.0f;
    float titleFontSize = 72.0f;
    float subtitleFontSize = 32.0f;
    float rowFontSize = 38.0f;
    float hintFontSize = 24.0f;
    float cupSize = 96.0f;
    float cupY = 0.0f;
    float titleY = 0.0f;
    float tableX = 0.0f;
    float tableY = 0.0f;
    float tableW = 0.0f;
    float tableH = 0.0f;
    LeaderboardButton backButton{};

    std::vector<int> recentWins;

    void updateLayout(const glm::ivec2 &fbSize)
    {
        float W = (float)fbSize.x;
        float H = (float)fbSize.y;

        titleFontSize = std::clamp(H * 0.11f, 68.0f, 120.0f);
        subtitleFontSize = std::clamp(H * 0.045f, 28.0f, 46.0f);
        rowFontSize = std::clamp(H * 0.054f, 32.0f, 50.0f);
        hintFontSize = std::clamp(H * 0.036f, 22.0f, 34.0f);
        cupSize = std::clamp(H * 0.11f, 82.0f, 128.0f);

        cupY = H * 0.045f;
        titleY = cupY + cupSize + H * 0.030f;
        tableW = std::clamp(W * 0.62f, 560.0f, 860.0f);
        tableH = std::clamp(H * 0.58f, 360.0f, 540.0f);
        tableX = (W - tableW) * 0.5f;
        tableY = titleY + titleFontSize + subtitleFontSize + H * 0.050f;

        backButton.size = {tableW, hintFontSize * 2.3f};
        float buttonY = tableY + tableH + H * 0.028f;
        float bottomMargin = H * 0.035f;
        backButton.position = {tableX, std::min(buttonY, H - backButton.size.y - bottomMargin)};
    }

    static void drawShadowedText(ImDrawList *dl, ImFont *font, float fontSize,
                                 const glm::vec2 &pos, ImU32 color, const char *text,
                                 float shadowMul = 0.8f, float shadowOff = 3.0f)
    {
        float alpha = ((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
        ImU32 shadow = ImGui::ColorConvertFloat4ToU32(
            ImVec4(0.02f, 0.01f, 0.01f, alpha * shadowMul));
        dl->AddText(font, fontSize, ImVec2(pos.x + shadowOff, pos.y + shadowOff), shadow, text);
        dl->AddText(font, fontSize, ImVec2(pos.x, pos.y), color, text);
    }

    void onInitialize() override
    {
        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->Clear();
        titleFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Cinzel-Bold.ttf", 128.0f);
        bodyFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Rajdhani-Bold.ttf", 128.0f);
        if (!titleFont)
            titleFont = io.Fonts->AddFontDefault();
        if (!bodyFont)
            bodyFont = io.Fonts->AddFontDefault();
        getApp()->rebuildImGuiFonts();

        auto makeTinted = [&]() -> our::TintedMaterial *
        {
            auto *material = new our::TintedMaterial();
            material->shader = new our::ShaderProgram();
            material->shader->attach("assets/shaders/tinted.vert", GL_VERTEX_SHADER);
            material->shader->attach("assets/shaders/tinted.frag", GL_FRAGMENT_SHADER);
            material->shader->link();
            material->pipelineState.blending.enabled = true;
            material->pipelineState.blending.equation = GL_FUNC_ADD;
            material->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
            material->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;
            return material;
        };

        backgroundMaterial = makeTinted();
        backgroundMaterial->pipelineState.blending.enabled = false;
        buttonMaterial = makeTinted();

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

        rectangle = new our::Mesh(
            {
                {{0, 0, 0}, {255, 255, 255, 255}, {0, 1}, {0, 0, 1}},
                {{1, 0, 0}, {255, 255, 255, 255}, {1, 1}, {0, 0, 1}},
                {{1, 1, 0}, {255, 255, 255, 255}, {1, 0}, {0, 0, 1}},
                {{0, 1, 0}, {255, 255, 255, 255}, {0, 0}, {0, 0, 1}},
            },
            {0, 1, 2, 2, 3, 0});

        recentWins = our::ScoreManager::getRecentWins();
        backButton.action = [this]()
        { getApp()->changeState("menu"); };
    }

    void onImmediateGui() override
    {
        glm::ivec2 fbSize = getApp()->getFrameBufferSize();
        updateLayout(fbSize);

        float W = (float)fbSize.x;
        float titleFade = glm::smoothstep(0.15f, 0.75f, time);
        float bodyFade = glm::smoothstep(0.35f, 1.00f, time);
        float pulse = 0.5f + 0.5f * std::sin(time * 3.0f);
        bool backHovered = backButton.isInside(getApp()->getMouse().getMousePosition());

        ImDrawList *dl = ImGui::GetForegroundDrawList();
        ImFont *tf = titleFont ? titleFont : ImGui::GetFont();
        ImFont *bf = bodyFont ? bodyFont : ImGui::GetFont();

        ImVec2 titleSize = tf->CalcTextSizeA(titleFontSize, FLT_MAX, 0.0f, ScreenTitle);
        glm::vec2 titlePos = {(W - titleSize.x) * 0.5f, titleY};
        drawShadowedText(
            dl, tf, titleFontSize, titlePos,
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.99f, 0.85f, 0.34f, titleFade)),
            ScreenTitle, 0.9f, titleFontSize * 0.045f);

        ImVec2 subtitleSize = bf->CalcTextSizeA(subtitleFontSize, FLT_MAX, 0.0f, Subtitle);
        dl->AddText(
            bf, subtitleFontSize,
            ImVec2((W - subtitleSize.x) * 0.5f, titlePos.y + titleSize.y + fbSize.y * 0.012f),
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.93f, 0.86f, 0.70f, titleFade * 0.9f)),
            Subtitle);

        float rowH = tableH / (float)(our::ScoreManager::MaxScores + 1);
        float rankCenterX = tableX + tableW * 0.22f;
        float scoreCenterX = tableX + tableW * 0.68f;

        float headerFontSize = rowFontSize * 0.98f;
        ImU32 headerColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.14f, 0.08f, 0.03f, bodyFade));
        dl->AddText(bf, headerFontSize,
                    ImVec2(rankCenterX - bf->CalcTextSizeA(headerFontSize, FLT_MAX, 0.0f, "RANK").x * 0.5f,
                           tableY + rowH * 0.20f),
                    headerColor, "RANK");
        dl->AddText(bf, headerFontSize,
                    ImVec2(scoreCenterX - bf->CalcTextSizeA(headerFontSize, FLT_MAX, 0.0f, "SCORE").x * 0.5f,
                           tableY + rowH * 0.20f),
                    headerColor, "SCORE");
        ImU32 headerSeparator = ImGui::ColorConvertFloat4ToU32(ImVec4(0.30f, 0.17f, 0.07f, bodyFade * 0.55f));
        dl->AddLine(ImVec2(tableX + 8.0f, tableY + rowH),
                    ImVec2(tableX + tableW - 8.0f, tableY + rowH),
                    headerSeparator, 1.6f);

        for (int i = 0; i < our::ScoreManager::MaxScores; ++i)
        {
            float rowTop = tableY + rowH * (i + 1);
            float rowMid = rowTop + rowH * 0.5f;
            bool hasEntry = i < (int)recentWins.size();
            bool latestEntry = i == 0 && hasEntry;

            if (latestEntry)
            {
                ImU32 latestBorder = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(0.40f, 0.23f, 0.09f, (0.45f + pulse * 0.08f) * bodyFade));
                dl->AddRect(ImVec2(tableX + 8.0f, rowTop + 6.0f),
                            ImVec2(tableX + tableW - 8.0f, rowTop + rowH - 6.0f),
                            latestBorder, 8.0f, 0, 1.4f);
            }

            if (i < our::ScoreManager::MaxScores - 1)
            {
                ImU32 separator = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(0.30f, 0.17f, 0.07f, bodyFade * 0.42f));
                dl->AddLine(ImVec2(tableX + 8.0f, rowTop + rowH),
                            ImVec2(tableX + tableW - 8.0f, rowTop + rowH),
                            separator, 1.4f);
            }

            char rankBuf[16];
            snprintf(rankBuf, sizeof(rankBuf), "#%d", i + 1);
            const char *scoreText = "---";
            char scoreBuf[32];
            if (hasEntry)
            {
                snprintf(scoreBuf, sizeof(scoreBuf), "%d", recentWins[i]);
                scoreText = scoreBuf;
            }

            ImU32 rankColor = ImGui::ColorConvertFloat4ToU32(
                hasEntry ? ImVec4(0.12f, 0.07f, 0.03f, bodyFade) : ImVec4(0.24f, 0.18f, 0.12f, bodyFade * 0.7f));
            ImU32 scoreColor = ImGui::ColorConvertFloat4ToU32(
                hasEntry ? ImVec4(0.10f, 0.06f, 0.02f, bodyFade) : ImVec4(0.24f, 0.18f, 0.12f, bodyFade * 0.7f));

            ImVec2 rankSize = bf->CalcTextSizeA(rowFontSize, FLT_MAX, 0.0f, rankBuf);
            ImVec2 scoreSize = bf->CalcTextSizeA(rowFontSize, FLT_MAX, 0.0f, scoreText);
            float textY = rowMid - rowFontSize * 0.42f;
            dl->AddText(bf, rowFontSize, ImVec2(rankCenterX - rankSize.x * 0.5f, textY), rankColor, rankBuf);
            dl->AddText(bf, rowFontSize, ImVec2(scoreCenterX - scoreSize.x * 0.5f, textY), scoreColor, scoreText);
        }

        if (recentWins.empty())
        {
            const char *emptyText = "NO WINS RECORDED YET";
            ImVec2 emptySize = bf->CalcTextSizeA(rowFontSize * 0.92f, FLT_MAX, 0.0f, emptyText);
            dl->AddText(
                bf, rowFontSize * 0.92f,
                ImVec2((W - emptySize.x) * 0.5f, tableY + tableH * 0.42f),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.30f, 0.18f, 0.09f, bodyFade * 0.85f)),
                emptyText);
        }

        ImVec2 hintSize = bf->CalcTextSizeA(hintFontSize, FLT_MAX, 0.0f, HintBack);
        dl->AddText(
            bf, hintFontSize,
            ImVec2(backButton.position.x + (backButton.size.x - hintSize.x) * 0.5f,
                   backButton.position.y + (backButton.size.y - hintSize.y) * 0.5f),
            ImGui::ColorConvertFloat4ToU32(
                backHovered ? ImVec4(1.0f, 0.96f, 0.84f, bodyFade)
                            : ImVec4(0.95f, 0.90f, 0.78f, bodyFade)),
            HintBack);
    }

    void onDraw(double deltaTime) override
    {
        glm::ivec2 size = getApp()->getFrameBufferSize();
        updateLayout(size);

        auto &keyboard = getApp()->getKeyboard();
        if (keyboard.justPressed(GLFW_KEY_ESCAPE) || keyboard.justPressed(GLFW_KEY_L))
        {
            getApp()->changeState("menu");
            return;
        }

        auto &mouse = getApp()->getMouse();
        if (mouse.justPressed(0) && backButton.isInside(mouse.getMousePosition()))
        {
            backButton.action();
            return;
        }

        glViewport(0, 0, size.x, size.y);
        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        glm::mat4 fullScreen = glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
        bool backHovered = backButton.isInside(mouse.getMousePosition());

        time += (float)deltaTime;

        backgroundMaterial->tint = glm::vec4(0.36f, 0.24f, 0.10f, 1.0f);
        backgroundMaterial->setup();
        backgroundMaterial->shader->set("transform", VP * fullScreen);
        rectangle->draw();

        buttonMaterial->tint = backHovered
                                   ? glm::vec4(0.49f, 0.30f, 0.10f, 0.92f)
                                   : glm::vec4(0.42f, 0.25f, 0.09f, 0.86f);
        buttonMaterial->setup();
        buttonMaterial->shader->set("transform", VP * (glm::translate(glm::mat4(1.0f), glm::vec3(backButton.position.x, backButton.position.y, 0.0f)) *
                                                       glm::scale(glm::mat4(1.0f), glm::vec3(backButton.size.x, backButton.size.y, 1.0f))));
        rectangle->draw();

        glm::mat4 cupTransform = glm::translate(glm::mat4(1.0f), glm::vec3((size.x - cupSize) * 0.5f, cupY, 0.0f)) *
                                 glm::scale(glm::mat4(1.0f), glm::vec3(cupSize, cupSize, 1.0f));
        cupMaterial->setup();
        cupMaterial->shader->set("transform", VP * cupTransform);
        rectangle->draw();
    }

    void onDestroy() override
    {
        delete rectangle;
        delete backgroundMaterial->shader;
        delete backgroundMaterial;
        delete cupMaterial->texture;
        delete cupMaterial->shader;
        delete cupMaterial;
        delete buttonMaterial->shader;
        delete buttonMaterial;

        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->AddFontDefault();
        getApp()->rebuildImGuiFonts();
        titleFont = nullptr;
        bodyFont = nullptr;
    }
};
