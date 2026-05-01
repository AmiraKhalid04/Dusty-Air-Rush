#pragma once
#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
#include "../components/collider.hpp"
#include "../asset-loader.hpp"
#include "../utils/track-utils.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <random>

namespace our
{
    struct HealthPackConfig
    {
        // Track boundaries
        glm::vec3 trackStartPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 trackEndPosition = glm::vec3(0.0f, 0.0f, 300.0f);

        int healthPacksCount = 10;
        float margin = 5.0f;
        float depthOffset = 30.0f; // Offset to avoid intersecting with rings
        float spawnChance = 0.4f;  // Probability of spawning at each position

        float scale = 0.5f;
        float collectRadius = 2.0f;

        // Calculated at runtime
        float spacing = 0.0f;
    };

    class HealthPackSystem
    {
        std::mt19937 rng{std::random_device{}()};

    public:
        void initialize(World *world, const HealthPackConfig &config)
        {
            Mesh *mesh = AssetLoader<Mesh>::get("health_pack");
            Material *mat = AssetLoader<Material>::get("health_pack");
            if (!mesh || !mat)
                return;

            // Calculate spacing from track depth and health packs count
            float trackDepth = config.trackStartPosition.z - config.trackEndPosition.z;
            float calculatedSpacing = trackDepth / (config.healthPacksCount + 1.0f);

            // Calculate X and Y ranges based on track start/end positions with margin
            float xMin = std::min(config.trackStartPosition.x, config.trackEndPosition.x) + config.margin;
            float xMax = std::max(config.trackStartPosition.x, config.trackEndPosition.x) - config.margin;
            float yMin = std::min(config.trackStartPosition.y, config.trackEndPosition.y) + config.margin;
            float yMax = std::max(config.trackStartPosition.y, config.trackEndPosition.y) - config.margin;

            std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
            std::uniform_real_distribution<float> xDist(xMin, xMax);
            std::uniform_real_distribution<float> yDist(yMin, yMax);

            glm::vec3 cursor = config.trackStartPosition;
            cursor.z += config.depthOffset; // Start offset to avoid intersecting with rings

            for (int i = 0; i < config.healthPacksCount; i++)
            {
                cursor.z -= calculatedSpacing;

                // Random chance to skip this position
                if (chanceDist(rng) > config.spawnChance)
                    continue;

                Entity *entity = world->add();
                entity->name = "health_pack_" + std::to_string(i);

                // Random X and Y within track margins
                glm::vec3 spawnPos = {
                    xDist(rng) + trackCurveX(cursor.z),
                    yDist(rng),
                    cursor.z};

                entity->localTransform.position = spawnPos;
                entity->localTransform.scale = glm::vec3(config.scale);
                entity->localTransform.rotation = glm::vec3(0, 0, 0);

                auto *mr = entity->addComponent<MeshRendererComponent>();
                mr->mesh = mesh;
                mr->material = mat;

                auto *col = entity->addComponent<ColliderComponent>();
                col->shapeType = ColliderType::Sphere;
                col->objectType = "health";
                col->sphereRadius = config.collectRadius;
                col->center = glm::vec3(0.0f, 0.5f, 0.0f);

                std::cout << "Health pack " << i << " at: "
                          << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << std::endl;
            }
        }
    };
}