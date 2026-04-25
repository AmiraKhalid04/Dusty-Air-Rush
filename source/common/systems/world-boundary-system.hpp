#pragma once
#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
#include "../components/collider.hpp"
#include "../asset-loader.hpp"
#include "track-system.hpp"
#include "text-popup-system.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <iostream>

namespace our
{

    struct WorldBounds
    {
        glm::vec3 min;
        glm::vec3 max;
    };

    class WorldBoundarySystem
    {
        WorldBounds bounds;
        bool initialized = false;
        bool wasClamped = false;

        float flashIntensity = 0.0f;
        static constexpr float FLASH_SPIKE = 1.0f;
        static constexpr float FLASH_DECAY = 3.0f;
        float lastDeltaTime = 0.016f;

        TextPopupSystem *textPopupSystem = nullptr;

    public:
        void setTextPopupSystem(TextPopupSystem *tps) { textPopupSystem = tps; }

        void initialize(TrackConfig &config)
        {
            float minX = config.startPosition.x - config.outerMargin;
            float maxX = config.endPosition.x + config.outerMargin;
            float minY = config.startPosition.y - config.outerMargin;
            float maxY = config.endPosition.y + config.outerMargin;
            float minZ = config.endPosition.z - config.outerMargin;
            float maxZ = config.startPosition.z + config.outerMargin;

            bounds.min = glm::vec3(minX, minY, minZ);
            bounds.max = glm::vec3(maxX, maxY, maxZ);

            initialized = true;

            std::cout << "[WorldBoundary] Bounds computed:\n"
                      << "  min: (" << bounds.min.x << ", "
                      << bounds.min.y << ", "
                      << bounds.min.z << ")\n"
                      << "  max: (" << bounds.max.x << ", "
                      << bounds.max.y << ", "
                      << bounds.max.z << ")\n";
        }

        void update(World *world, float deltaTime = 0.016f)
        {
            if (!initialized)
                return;

            // Decay the flash every frame
            flashIntensity = glm::max(0.0f, flashIntensity - FLASH_DECAY * deltaTime);

            for (auto entity : world->getEntities())
            {
                if (!entity->getComponent<CameraComponent>())
                    continue;

                glm::vec3 &pos = entity->localTransform.position;
                bool clamped = false;

                if (pos.x < bounds.min.x)
                {
                    pos.x = bounds.min.x;
                    clamped = true;
                }
                if (pos.x > bounds.max.x)
                {
                    pos.x = bounds.max.x;
                    clamped = true;
                }
                if (pos.y < bounds.min.y)
                {
                    pos.y = bounds.min.y;
                    clamped = true;
                }
                if (pos.y > bounds.max.y)
                {
                    pos.y = bounds.max.y;
                    clamped = true;
                }
                if (pos.z < bounds.min.z)
                {
                    pos.z = bounds.min.z;
                    clamped = true;
                }
                if (pos.z > bounds.max.z)
                {
                    pos.z = bounds.max.z;
                    clamped = true;
                }

                if (clamped && !wasClamped)
                {
                    flashIntensity = FLASH_SPIKE;
                    if (textPopupSystem)
                        textPopupSystem->spawn("Turn back! Boundary reached.", {1.0f, 0.2f, 0.2f, 1.0f});
                }

                wasClamped = clamped;
            }
        }

        const WorldBounds &getBounds() const { return bounds; }

        float getFlashIntensity() const { return flashIntensity; }
    };
}