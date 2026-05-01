#pragma once

#include "../ecs/world.hpp"
#include "../components/camera.hpp"
#include "../components/free-camera-controller.hpp"
#include "../components/dusty.hpp"
#include "../components/collider.hpp"
#include "../mesh/mesh.hpp"
#include "../material/material.hpp"
#include "../asset-loader.hpp"
#include "../application.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <string>
#include <cfloat>

namespace our
{
    // UIRenderSystem is responsible for rendering all UI elements
    // It manages the creation and rendering of UI components independently from the game state
    class UIRenderSystem
    {
    private:
        Mesh *healthBarQuad = nullptr;
        TintedMaterial *healthBarMaterial = nullptr;

        // Find the player's Dusty component by looking for an entity with both
        // CameraComponent and FreeCameraControllerComponent
        DustyComponent *findPlayerDusty(World *world)
        {
            for (auto entity : world->getEntities())
            {
                if (entity->getComponent<CameraComponent>() &&
                    entity->getComponent<FreeCameraControllerComponent>())
                {
                    return entity->getComponent<DustyComponent>();
                }
            }
            return nullptr;
        }

        // Draw a single quad with the given transform and color
        void drawHealthBarQuad(const glm::mat4 &transform, const glm::vec4 &color)
        {
            healthBarMaterial->tint = color;
            healthBarMaterial->setup();
            healthBarMaterial->shader->set("transform", transform);
            healthBarQuad->draw();
        }

    public:
        // Initialize UI rendering resources
        void initialize(Application *app)
        {
            // Create the health bar quad mesh
            healthBarQuad = new Mesh({
                                         {{0.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                                         {{1.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                                         {{1.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
                                         {{0.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
                                     },
                                     {
                                         0,
                                         1,
                                         2,
                                         2,
                                         3,
                                         0,
                                     });

            healthBarMaterial = new TintedMaterial();
            healthBarMaterial->shader = AssetLoader<ShaderProgram>::get("tinted");
            healthBarMaterial->pipelineState.faceCulling.enabled = false;
            healthBarMaterial->pipelineState.depthTesting.enabled = false;
            healthBarMaterial->pipelineState.depthMask = false;
            healthBarMaterial->pipelineState.blending.enabled = true;
        }

        // Render all UI elements for the given world
        void render(World *world, Application *app)
        {
            auto *dusty = findPlayerDusty(world);
            if (!dusty || !healthBarMaterial || !healthBarMaterial->shader || !healthBarQuad)
                return;

            auto size = app->getFrameBufferSize();
            glViewport(0, 0, size.x, size.y);

            float healthRatio = 0.0f;
            if (dusty->maxHealth > 0.0f)
            {
                healthRatio = glm::clamp(dusty->currentHealth / dusty->maxHealth, 0.0f, 1.0f);
            }

            constexpr float left = -0.95f;
            constexpr float bottom = 0.85f;
            constexpr float maxWidth = 0.3f;
            constexpr float barHeight = 0.05f;
            constexpr float borderThickness = 0.005f;

            // 5-tier color matching the provided image palette
            glm::vec4 fillColor;
            if (healthRatio > 0.8f)
            {
                fillColor = {0.23f, 0.80f, 0.15f, 1.0f}; // Green
            }
            else if (healthRatio > 0.6f)
            {
                fillColor = {0.71f, 0.87f, 0.27f, 1.0f}; // Light Green / Lime
            }
            else if (healthRatio > 0.4f)
            {
                fillColor = {0.97f, 0.87f, 0.40f, 1.0f}; // Yellow
            }
            else if (healthRatio > 0.2f)
            {
                fillColor = {0.94f, 0.61f, 0.20f, 1.0f}; // Orange
            }
            else
            {
                fillColor = {0.89f, 0.14f, 0.15f, 1.0f}; // Red
            }

            // ── Fixed-width frame (always renders at maxWidth) ──
            glm::vec4 frameColor = {0.15f, 0.15f, 0.15f, 0.9f}; // Dark grey frame

            // Top border
            drawHealthBarQuad(
                glm::translate(glm::mat4(1.0f), glm::vec3(left - borderThickness, bottom + barHeight, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(maxWidth + 2.0f * borderThickness, borderThickness, 1.0f)),
                frameColor);
            // Bottom border
            drawHealthBarQuad(
                glm::translate(glm::mat4(1.0f), glm::vec3(left - borderThickness, bottom - borderThickness, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(maxWidth + 2.0f * borderThickness, borderThickness, 1.0f)),
                frameColor);
            // Left border
            drawHealthBarQuad(
                glm::translate(glm::mat4(1.0f), glm::vec3(left - borderThickness, bottom - borderThickness, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(borderThickness, barHeight + 2.0f * borderThickness, 1.0f)),
                frameColor);
            // Right border
            drawHealthBarQuad(
                glm::translate(glm::mat4(1.0f), glm::vec3(left + maxWidth, bottom - borderThickness, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(borderThickness, barHeight + 2.0f * borderThickness, 1.0f)),
                frameColor);

            // ── Background (empty portion) ──
            drawHealthBarQuad(
                glm::translate(glm::mat4(1.0f), glm::vec3(left, bottom, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(maxWidth, barHeight, 1.0f)),
                {0.1f, 0.1f, 0.1f, 0.6f}); // Dark semi-transparent background

            // ── Fill (health portion) ──
            const float fillWidth = maxWidth * healthRatio;
            if (fillWidth > 0.0f)
            {
                glm::mat4 foregroundTransform = glm::translate(
                                                    glm::mat4(1.0f),
                                                    glm::vec3(left, bottom, 0.0f)) *
                                                glm::scale(glm::mat4(1.0f), glm::vec3(fillWidth, barHeight, 1.0f));
                drawHealthBarQuad(foregroundTransform, fillColor);
            }
        }

        // Render persistent HUD text using ImGui draw lists.
        // Call this from the state's onImmediateGui().
        void renderScore(World *world, Application *app, float playTime, int currentSong = 0)
        {
            auto *dusty = findPlayerDusty(world);
            if (!dusty)
                return;

            ImDrawList *drawList = ImGui::GetForegroundDrawList();
            if (!drawList)
                return;

            glm::ivec2 screenSize = app->getFrameBufferSize();

            char timeBuf[64];
            int mins = (int)playTime / 60;
            float secs = playTime - (mins * 60);
            snprintf(timeBuf, sizeof(timeBuf), "%02d:%05.2f", mins, secs);

            std::string line1 = "Score: " + std::to_string(dusty->score);
            std::string line2 = "   " + std::string(timeBuf);
            std::string line3 = "   " + std::to_string(dusty->ringsPassed) + "      " + std::to_string(dusty->coins);

            ImFont *font = ImGui::GetFont();
            const float fontSize = 34.0f;
            const float emojiFontSize = fontSize * 0.4f; // 0.4 of size
            constexpr float margin = 60.0f;

            ImVec2 size1 = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, line1.c_str());
            ImVec2 size2 = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, line2.c_str());
            ImVec2 size3 = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, line3.c_str());

            float maxTextWidth = std::max({size1.x, size2.x, size3.x});
            float totalHeight = size1.y + size2.y + size3.y + 10.0f; // 5.0f spacing between lines

            ImVec2 basePos(screenSize.x - maxTextWidth - margin, margin);

            ImU32 panelColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.05f, 0.07f, 0.10f, 0.72f));
            ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.84f, 0.22f, 0.85f));
            ImU32 shadowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 0.65f));
            ImU32 textColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.95f, 0.78f, 1.0f));

            ImVec2 padding(18.0f, 12.0f);
            ImVec2 panelMin(basePos.x - padding.x, basePos.y - padding.y);
            ImVec2 panelMax(basePos.x + maxTextWidth + padding.x, basePos.y + totalHeight + padding.y);

            drawList->AddRect(panelMin, panelMax, borderColor, 10.0f, 0, 2.0f);

            ImVec2 pos1 = basePos;
            ImVec2 pos2 = ImVec2(basePos.x, basePos.y + size1.y + 5.0f);
            ImVec2 pos3 = ImVec2(basePos.x, basePos.y + size1.y + size2.y + 10.0f);

            drawList->AddText(font, fontSize, ImVec2(pos1.x + 2.0f, pos1.y + 2.0f), shadowColor, line1.c_str());
            drawList->AddText(font, fontSize, pos1, textColor, line1.c_str());

            drawList->AddText(font, fontSize, ImVec2(pos2.x + 2.0f, pos2.y + 2.0f), shadowColor, line2.c_str());
            drawList->AddText(font, fontSize, pos2, textColor, line2.c_str());

            // Draw timer emoji explicitly with smaller size at the start of line2
            const char *timerEmoji = u8"⏱";
            ImVec2 emojiSize = font->CalcTextSizeA(emojiFontSize, FLT_MAX, 0.0f, timerEmoji);
            ImVec2 emojiPos = ImVec2(pos2.x, pos2.y + (fontSize - emojiFontSize) * 0.5f); // vertically center
            drawList->AddText(font, emojiFontSize, ImVec2(emojiPos.x + 2.0f, emojiPos.y + 2.0f), shadowColor, timerEmoji);
            drawList->AddText(font, emojiFontSize, emojiPos, textColor, timerEmoji);

            drawList->AddText(font, fontSize, ImVec2(pos3.x + 2.0f, pos3.y + 2.0f), shadowColor, line3.c_str());
            drawList->AddText(font, fontSize, pos3, textColor, line3.c_str());

            // Draw ring emoji
            const char *ringEmoji = u8"🛟";
            ImVec2 ringEmojiPos = ImVec2(pos3.x, pos3.y + (fontSize - emojiFontSize) * 0.5f);
            drawList->AddText(font, emojiFontSize, ImVec2(ringEmojiPos.x + 2.0f, ringEmojiPos.y + 2.0f), shadowColor, ringEmoji);
            drawList->AddText(font, emojiFontSize, ringEmojiPos, textColor, ringEmoji);

            // Draw coin emoji
            ImVec2 ringsPartSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, ("   " + std::to_string(dusty->ringsPassed) + "   ").c_str());
            const char *coinEmoji = u8"🪙";
            ImVec2 coinEmojiPos = ImVec2(pos3.x + ringsPartSize.x, pos3.y + (fontSize - emojiFontSize) * 0.5f);
            drawList->AddText(font, emojiFontSize, ImVec2(coinEmojiPos.x + 2.0f, coinEmojiPos.y + 2.0f), shadowColor, coinEmoji);
            drawList->AddText(font, emojiFontSize, coinEmojiPos, textColor, coinEmoji);

            if (currentSong == 1 || currentSong == 2)
            {
                // Reversed presentation forms to bypass LTR rendering
                const char *songText = (currentSong == 1)
                                           ? u8"\uFEF2\uFEE8\uFEA3\uFEAE\uFEA0\uFE97 \uFEF1\uFE8D\uFEAF\uFE8D \uFE96\uFEE7\uFE8D"
                                           : u8"\uFEA1\uFE8D\uFEAE\uFE9F \uFE90\uFEF4\uFE92\uFEC3";

                // Add speaker emoji beside it. Visual direction is RTL, so adding at the end of LTR string (visual left)
                std::string songString = std::string(songText) + u8" 🔊";

                ImVec2 songSize = font->CalcTextSizeA(36.0f, FLT_MAX, 0.0f, songString.c_str());
                // Top middle position, with a bit of top margin
                ImVec2 songPos(screenSize.x * 0.5f - songSize.x * 0.5f, 40.0f);

                ImU32 songColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.84f, 0.22f, 1.0f)); // Gold fancy
                if (currentSong == 2)
                    songColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.4f, 0.9f, 1.0f, 1.0f)); // Cyan fancy

                // Add fancy background box for the song text
                ImVec2 boxMin(songPos.x - 20.0f, songPos.y - 10.0f);
                ImVec2 boxMax(songPos.x + songSize.x + 20.0f, songPos.y + songSize.y + 10.0f);
                drawList->AddRectFilled(boxMin, boxMax, ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 0.6f)), 15.0f);
                drawList->AddRect(boxMin, boxMax, songColor, 15.0f, 0, 3.0f);

                drawList->AddText(font, 36.0f, ImVec2(songPos.x + 3.0f, songPos.y + 3.0f), shadowColor, songString.c_str());
                drawList->AddText(font, 36.0f, songPos, songColor, songString.c_str());
            }
        }

        void renderMiniMap(World *world, Application *app)
        {
            auto *dusty = findPlayerDusty(world);
            if (!dusty)
                return;

            Entity *playerEntity = nullptr;
            for (auto entity : world->getEntities())
            {
                if (entity->getComponent<CameraComponent>() &&
                    entity->getComponent<FreeCameraControllerComponent>())
                {
                    playerEntity = entity;
                    break;
                }
            }
            if (!playerEntity)
                return;

            glm::ivec2 screenSize = app->getFrameBufferSize();
            ImDrawList *drawList = ImGui::GetForegroundDrawList();
            if (!drawList)
                return;

            glm::mat4 playerMatrix = playerEntity->getLocalToWorldMatrix();
            glm::vec3 playerPos = glm::vec3(playerMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

            float mapRadius = 80.0f;
            ImVec2 center((float)screenSize.x - mapRadius - 30.0f, (float)screenSize.y - mapRadius - 30.0f);

            ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.04f, 0.12f, 0.08f, 0.85f));
            ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.8f, 0.4f, 0.95f));
            ImU32 gridColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.1f, 0.4f, 0.2f, 0.4f));

            drawList->AddCircleFilled(center, mapRadius, bgColor, 64);

            // Draw a subtle radar grid
            drawList->AddCircle(center, mapRadius * 0.33f, gridColor, 32, 1.5f);
            drawList->AddCircle(center, mapRadius * 0.66f, gridColor, 48, 1.5f);
            drawList->AddLine(ImVec2(center.x - mapRadius, center.y), ImVec2(center.x + mapRadius, center.y), gridColor, 1.5f);
            drawList->AddLine(ImVec2(center.x, center.y - mapRadius), ImVec2(center.x, center.y + mapRadius), gridColor, 1.5f);

            drawList->AddCircle(center, mapRadius, borderColor, 64, 3.0f);

            std::vector<glm::vec3> unpassedRings;
            Entity *finishLineEntity = nullptr;
            for (auto entity : world->getEntities())
            {
                auto col = entity->getComponent<ColliderComponent>();
                if (col && col->objectType == "ring_score")
                {
                    glm::mat4 ringMatrix = entity->getLocalToWorldMatrix();
                    glm::vec3 ringPos = glm::vec3(ringMatrix * glm::vec4(col->center, 1.0f));
                    unpassedRings.push_back(ringPos);
                }
                else if (col && col->objectType == "finish_line")
                {
                    finishLineEntity = entity;
                }
            }

            float worldRadius = 82.0f;
            float scale = mapRadius / worldRadius;

            glm::vec3 pForward = glm::vec3(playerMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
            glm::vec3 pRight = glm::vec3(playerMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
            pForward.y = 0.0f;
            pRight.y = 0.0f;
            if (glm::length(pForward) > 0.001f)
                pForward = glm::normalize(pForward);
            else
                pForward = glm::vec3(0.0f, 0.0f, -1.0f);
            if (glm::length(pRight) > 0.001f)
                pRight = glm::normalize(pRight);
            else
                pRight = glm::vec3(1.0f, 0.0f, 0.0f);

            float minDistance = FLT_MAX;
            glm::vec3 closestTargetPos(0.0f);
            bool hasClosest = false;

            ImU32 ringColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.8f, 1.0f, 1.0f));

            if (!unpassedRings.empty())
            {
                for (const auto &ringPos : unpassedRings)
                {
                    float dx = ringPos.x - playerPos.x;
                    float dz = ringPos.z - playerPos.z;
                    float dist = std::sqrt(dx * dx + dz * dz);

                    if (dist < minDistance)
                    {
                        minDistance = dist;
                        closestTargetPos = ringPos;
                        hasClosest = true;
                    }

                    if (dist <= worldRadius)
                    {
                        float localX = dx * pRight.x + dz * pRight.z;
                        float localY = -(dx * pForward.x + dz * pForward.z);

                        float sx = localX * scale;
                        float sy = localY * scale;
                        drawList->AddCircleFilled(ImVec2(center.x + sx, center.y + sy), 5.0f, ringColor, 12);
                    }
                }
            }
            else if (finishLineEntity)
            {
                auto col = finishLineEntity->getComponent<ColliderComponent>();
                glm::mat4 finishMatrix = finishLineEntity->getLocalToWorldMatrix();
                glm::vec3 finishPos = glm::vec3(finishMatrix * glm::vec4(col->center, 1.0f));

                float dx = finishPos.x - playerPos.x;
                float dz = finishPos.z - playerPos.z;
                float dist = std::sqrt(dx * dx + dz * dz);

                minDistance = dist;
                closestTargetPos = finishPos;
                hasClosest = true;

                if (dist <= worldRadius)
                {
                    float localX = dx * pRight.x + dz * pRight.z;
                    float localY = -(dx * pForward.x + dz * pForward.z);

                    float sx = localX * scale;
                    float sy = localY * scale;
                    ImU32 finishColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
                    drawList->AddCircleFilled(ImVec2(center.x + sx, center.y + sy), 6.0f, finishColor, 16);
                }
            }

            if (hasClosest && minDistance > worldRadius)
            {
                float dx = closestTargetPos.x - playerPos.x;
                float dz = closestTargetPos.z - playerPos.z;

                float localX = dx * pRight.x + dz * pRight.z;
                float localY = -(dx * pForward.x + dz * pForward.z);

                float dirX = localX / minDistance;
                float dirY = localY / minDistance;

                ImU32 yellowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.84f, 0.22f, 1.0f));
                if (unpassedRings.empty() && finishLineEntity)
                {
                    yellowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 1.0f, 0.2f, 1.0f)); // Point to finish line with green
                }
                ImVec2 edgePos(center.x + dirX * (mapRadius - 5.0f), center.y + dirY * (mapRadius - 5.0f));
                drawList->AddCircleFilled(edgePos, 6.0f, yellowColor, 16);
            }

            ImU32 planeColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
            drawList->AddTriangleFilled(
                ImVec2(center.x, center.y - 7.0f),
                ImVec2(center.x - 5.0f, center.y + 5.0f),
                ImVec2(center.x + 5.0f, center.y + 5.0f),
                planeColor);
        }

        void renderDangerOverlay(glm::ivec2 screenSize, float intensity)
        {
            if (!healthBarMaterial || !healthBarMaterial->shader || !healthBarQuad)
                return;

            float clampedIntensity = glm::clamp(intensity, 0.0f, 1.0f);
            if (clampedIntensity <= 0.0f)
                return;

            glViewport(0, 0, screenSize.x, screenSize.y);

            float time = (float)glfwGetTime();
            float blink = glm::sin(time * 10.0f) > 0.0f ? 1.0f : 0.0f;
            float pulse = 0.85f + 0.15f * glm::sin(time * 6.0f);
            float overlayAlpha = glm::clamp(clampedIntensity * 0.35f * pulse * blink, 0.0f, 1.0f);

            auto previousBlendEquation = healthBarMaterial->pipelineState.blending.equation;
            auto previousSource = healthBarMaterial->pipelineState.blending.sourceFactor;
            auto previousDestination = healthBarMaterial->pipelineState.blending.destinationFactor;

            healthBarMaterial->pipelineState.blending.equation = GL_FUNC_ADD;
            healthBarMaterial->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
            healthBarMaterial->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;

            glm::mat4 fullScreenTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f)) *
                                            glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 1.0f));
            drawHealthBarQuad(fullScreenTransform, {1.0f, 0.0f, 0.0f, overlayAlpha});

            healthBarMaterial->pipelineState.blending.equation = previousBlendEquation;
            healthBarMaterial->pipelineState.blending.sourceFactor = previousSource;
            healthBarMaterial->pipelineState.blending.destinationFactor = previousDestination;
        }

        void renderBoundaryFlash(glm::ivec2 screenSize, float intensity)
        {
            if (!healthBarMaterial || !healthBarMaterial->shader || !healthBarQuad)
                return;

            float clampedIntensity = glm::clamp(intensity, 0.0f, 1.0f);
            if (clampedIntensity <= 0.0f)
                return;

            glViewport(0, 0, screenSize.x, screenSize.y);

            // Ease-out: quadratic so it snaps bright then fades smoothly
            float overlayAlpha = clampedIntensity * clampedIntensity * 0.5f;

            // Save blending state
            auto previousBlendEquation = healthBarMaterial->pipelineState.blending.equation;
            auto previousSource = healthBarMaterial->pipelineState.blending.sourceFactor;
            auto previousDestination = healthBarMaterial->pipelineState.blending.destinationFactor;

            healthBarMaterial->pipelineState.blending.equation = GL_FUNC_ADD;
            healthBarMaterial->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
            healthBarMaterial->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;

            // Full-screen quad: same transform used in renderDangerOverlay
            glm::mat4 fullScreenTransform =
                glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 1.0f));

            // White flash instead of red
            drawHealthBarQuad(fullScreenTransform, {1.0f, 1.0f, 1.0f, overlayAlpha});

            // Restore blending state
            healthBarMaterial->pipelineState.blending.equation = previousBlendEquation;
            healthBarMaterial->pipelineState.blending.sourceFactor = previousSource;
            healthBarMaterial->pipelineState.blending.destinationFactor = previousDestination;
        }

        // Clean up UI resources
        void destroy()
        {
            delete healthBarQuad;
            delete healthBarMaterial;
        }
    };

}
