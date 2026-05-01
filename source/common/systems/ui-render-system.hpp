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
        void renderScore(World *world, Application *app, float playTime)
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

            std::string scoreText = "Rings: " + std::to_string(dusty->ringsPassed) + 
                                    " Coins: " + std::to_string(dusty->coins) + 
                                    " Time: " + std::string(timeBuf) + 
                                    " Score: " + std::to_string(dusty->score);

            ImFont *font = ImGui::GetFont();
            const float fontSize = 34.0f;
            constexpr float margin = 60.0f;
            ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, scoreText.c_str());
            ImVec2 pos(screenSize.x - textSize.x - margin, margin);

            ImU32 panelColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.05f, 0.07f, 0.10f, 0.72f));
            ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.84f, 0.22f, 0.85f));
            ImU32 shadowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 0.65f));
            ImU32 textColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.95f, 0.78f, 1.0f));

            ImVec2 padding(18.0f, 12.0f);
            ImVec2 panelMin(pos.x - padding.x, pos.y - padding.y);
            ImVec2 panelMax(pos.x + textSize.x + padding.x, pos.y + textSize.y + padding.y);

            // drawList->AddRectFilled(panelMin, panelMax, panelColor, 10.0f);
            drawList->AddRect(panelMin, panelMax, borderColor, 10.0f, 0, 2.0f);
            drawList->AddText(font, fontSize, ImVec2(pos.x + 2.0f, pos.y + 2.0f), shadowColor, scoreText.c_str());
            drawList->AddText(font, fontSize, pos, textColor, scoreText.c_str());
        }

        void renderMiniMap(World *world, Application *app)
        {
            auto *dusty = findPlayerDusty(world);
            if (!dusty) return;

            Entity* playerEntity = nullptr;
            for (auto entity : world->getEntities())
            {
                if (entity->getComponent<CameraComponent>() &&
                    entity->getComponent<FreeCameraControllerComponent>())
                {
                    playerEntity = entity;
                    break;
                }
            }
            if (!playerEntity) return;

            glm::ivec2 screenSize = app->getFrameBufferSize();
            ImDrawList *drawList = ImGui::GetForegroundDrawList();
            if (!drawList) return;

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
            Entity* finishLineEntity = nullptr;
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
            pForward.y = 0.0f; pRight.y = 0.0f;
            if(glm::length(pForward) > 0.001f) pForward = glm::normalize(pForward);
            else pForward = glm::vec3(0.0f, 0.0f, -1.0f);
            if(glm::length(pRight) > 0.001f) pRight = glm::normalize(pRight);
            else pRight = glm::vec3(1.0f, 0.0f, 0.0f);

            float minDistance = FLT_MAX;
            glm::vec3 closestTargetPos(0.0f);
            bool hasClosest = false;

            ImU32 ringColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.8f, 1.0f, 1.0f)); 
            
            if (!unpassedRings.empty())
            {
                for (const auto& ringPos : unpassedRings)
                {
                    float dx = ringPos.x - playerPos.x;
                    float dz = ringPos.z - playerPos.z;
                    float dist = std::sqrt(dx*dx + dz*dz);
                    
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
                float dist = std::sqrt(dx*dx + dz*dz);
                
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
                if (unpassedRings.empty() && finishLineEntity) {
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
                planeColor
            );
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
