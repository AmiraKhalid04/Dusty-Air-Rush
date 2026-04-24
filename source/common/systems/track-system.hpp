#pragma once
#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
#include "../components/collider.hpp"
#include "../asset-loader.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <iostream>

namespace our
{
    struct TrackConfig
    {
        glm::vec3 startPosition = glm::vec3(-25.0f, 0.0f, 0.0f);
        glm::vec3 endPosition = glm::vec3(25.0f, 0.0f, -500.0f);
        int stagesCount = 10;
        float innerMargin = 5.0f;
        float outerMargin = 20.0f;
    };

    struct WorldBounds
    {
        glm::vec3 min;
        glm::vec3 max;
    };

    // WorldBoundarySystem computes the playable region from the ring track
    // configuration and clamps the camera/player position every frame.
    //
    // Usage:
    //   1. Call initialize() once, passing the same RingTrackConfig used to
    //      spawn the track and the boundary config read from app.jsonc.
    //   2. Call update() every frame before rendering.
    class WorldBoundarySystem
    {
        WorldBounds bounds;
        bool initialized = false;

    public:
        // ------------------------------------------------------------------ //
        //  Initialization                                                      //
        // ------------------------------------------------------------------ //

        // Derives the world AABB from ring track parameters + margin config.
        // This mirrors the position math inside RingTrackSystem::initialize()
        // so it stays in sync without needing to reference actual ring entities.
        void initialize(TrackConfig &config)
        {
            float minX = config.startPosition.x - config.outerMargin;
            float maxX = config.endPosition.x + config.outerMargin;
            float minY = config.startPosition.y - config.outerMargin;
            float maxY = config.endPosition.y + config.outerMargin;
            float minZ = config.endPosition.z - config.outerMargin;
            float maxZ = config.startPosition.z + config.outerMargin;

            // ---- apply per-axis margins ----------------------------------- //
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

        // ------------------------------------------------------------------ //
        //  Per-Frame Update                                                    //
        // ------------------------------------------------------------------ //

        // Clamps the position of every entity that owns a CameraComponent
        // (i.e. the player) to stay within the computed world bounds.
        void update(World *world)
        {
            if (!initialized)
                return;

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

                (void)clamped; // suppress unused-variable warning in release
            }
        }

        // ------------------------------------------------------------------ //
        //  Accessors                                                           //
        // ------------------------------------------------------------------ //

        const WorldBounds &getBounds() const { return bounds; }
    };
}