#pragma once

#include "../ecs/world.hpp"
#include "../components/camera.hpp"
#include "../components/free-camera-controller.hpp"
#include "../components/health-component.hpp"
#include "../mesh/mesh.hpp"
#include "../material/material.hpp"
#include "../asset-loader.hpp"
#include "../application.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace our
{
    // UIRenderSystem is responsible for rendering all UI elements
    // It manages the creation and rendering of UI components independently from the game state
    class UIRenderSystem
    {
    private:
        Mesh *healthBarQuad = nullptr;
        TintedMaterial *healthBarMaterial = nullptr;

        // Find the player's health component by looking for an entity with both
        // CameraComponent and FreeCameraControllerComponent
        HealthComponent *findPlayerHealth(World *world)
        {
            for (auto entity : world->getEntities())
            {
                if (entity->getComponent<CameraComponent>() &&
                    entity->getComponent<FreeCameraControllerComponent>())
                {
                    return entity->getComponent<HealthComponent>();
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
            auto *health = findPlayerHealth(world);
            if (!health || !healthBarMaterial || !healthBarMaterial->shader || !healthBarQuad)
                return;

            auto size = app->getFrameBufferSize();
            glViewport(0, 0, size.x, size.y);

            float healthRatio = 0.0f;
            if (health->maxHealth > 0.0f)
            {
                healthRatio = glm::clamp(health->currentHealth / health->maxHealth, 0.0f, 1.0f);
            }

            constexpr float left = -0.95f;
            constexpr float bottom = 0.85f;
            constexpr float maxWidth = 0.3f;
            constexpr float barHeight = 0.05f;
            constexpr float borderThickness = 0.004f;

            glm::vec4 fillColor = {1.0f, 0.0f, 0.0f, 1.0f};
            if (healthRatio > 0.6f)
            {
                fillColor = {0.0f, 1.0f, 0.0f, 1.0f};
            }
            else if (healthRatio > 0.3f)
            {
                fillColor = {1.0f, 1.0f, 0.0f, 1.0f};
            }

            const float visibleWidth = maxWidth * healthRatio;
            if (visibleWidth <= 0.0f)
            {
                return;
            }

            // Draw border quads
            drawHealthBarQuad(
                glm::translate(glm::mat4(1.0f), glm::vec3(left, bottom, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(visibleWidth, borderThickness, 1.0f)),
                {0.0f, 0.0f, 0.0f, 1.0f});
            drawHealthBarQuad(
                glm::translate(glm::mat4(1.0f), glm::vec3(left, bottom + barHeight - borderThickness, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(visibleWidth, borderThickness, 1.0f)),
                {0.0f, 0.0f, 0.0f, 1.0f});
            drawHealthBarQuad(
                glm::translate(glm::mat4(1.0f), glm::vec3(left, bottom, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(borderThickness, barHeight, 1.0f)),
                {0.0f, 0.0f, 0.0f, 1.0f});
            drawHealthBarQuad(
                glm::translate(glm::mat4(1.0f), glm::vec3(left + visibleWidth - borderThickness, bottom, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(borderThickness, barHeight, 1.0f)),
                {0.0f, 0.0f, 0.0f, 1.0f});

            // Draw fill quad
            if (visibleWidth > 2.0f * borderThickness)
            {
                const float innerWidth = visibleWidth - (2.0f * borderThickness);
                const float innerHeight = barHeight - (2.0f * borderThickness);
                glm::mat4 foregroundTransform = glm::translate(
                                                    glm::mat4(1.0f),
                                                    glm::vec3(left + borderThickness, bottom + borderThickness, 0.0f)) *
                                                glm::scale(glm::mat4(1.0f), glm::vec3(innerWidth, innerHeight, 1.0f));
                drawHealthBarQuad(foregroundTransform, fillColor);
            }
        }

        // Clean up UI resources
        void destroy()
        {
            delete healthBarQuad;
            delete healthBarMaterial;
        }
    };

}
