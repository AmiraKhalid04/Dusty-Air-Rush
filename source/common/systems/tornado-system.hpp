#pragma once
#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
#include "../components/collider.hpp"
#include "../asset-loader.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <random>

namespace our
{

    struct TornadoConfig
    {
        glm::vec3 trackStartPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 trackEndPosition = glm::vec3(0.0f, 0.0f, 300.0f);

        int tornadosCount = 10;
        float spacing = 0.0f; // will be calculated same as rings

        float margin = 10.0f;      // fixed distance from X boundaries of track (left or right)
        float depthOffset = 20.0f; // offset to start tornadoes before rings
        float scale = 10.0f;

        float spawnChance = 0.7f;
    };

    class TornadoSystem
    {
    public:
        void initialize(World *world, TornadoConfig &config)
        {
            Mesh *mesh = AssetLoader<Mesh>::get("tornado");
            Material *mat = AssetLoader<Material>::get("tornado");

            if (!mesh || !mat)
                return;

            std::random_device rd;
            std::mt19937 rng(rd());
            std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);

            // Calculate Y range for random height
            float yMin = std::min(config.trackStartPosition.y, config.trackEndPosition.y) + config.margin;
            float yMax = std::max(config.trackStartPosition.y, config.trackEndPosition.y) - config.margin;
            std::uniform_real_distribution<float> yDist(yMin, yMax);

            // Calculate X boundaries with margin
            float xLeft = std::min(config.trackStartPosition.x, config.trackEndPosition.x) + config.margin;
            float xRight = std::max(config.trackStartPosition.x, config.trackEndPosition.x) - config.margin;
            std::uniform_real_distribution<float> sideDist(0.0f, 1.0f);

            // Calculate spacing based on track depth and rings count
            float trackDepth = config.trackStartPosition.z - config.trackEndPosition.z;
            config.spacing = trackDepth / config.tornadosCount;

            glm::vec3 cursor = config.trackStartPosition;
            // Start offset (before first ring position)
            cursor.z += config.depthOffset;

            for (int i = 0; i < config.tornadosCount; i++)
            {
                cursor.z -= config.spacing;

                // decide spawn
                if (chanceDist(rng) > config.spawnChance)
                    continue;

                Entity *entity = world->add();
                entity->name = "tornado_" + std::to_string(i);

                // Random height within track Y range
                float randomY = yDist(rng);

                // Random left or right at fixed margin distance
                float randomSide = sideDist(rng);
                float posX = (randomSide < 0.5f) ? xLeft : xRight;

                glm::vec3 pos;
                pos.z = cursor.z;
                pos.y = randomY;
                pos.x = posX;

                entity->localTransform.position = pos;

                float base = config.scale;
                entity->localTransform.scale = glm::vec3(base, base * 0.5f, base);

                entity->localTransform.rotation = glm::vec3(0, 0, 0);

                auto *mr = entity->addComponent<MeshRendererComponent>();
                mr->mesh = mesh;
                mr->material = mat;

                // Dynamic collider for collision detection
                auto *col = entity->addComponent<ColliderComponent>();
                col->shapeType = ColliderType::AABB;
                col->objectType = "tornado";
                col->aabbExtents = glm::vec3(1.0f, 2.0f, 1.0f);
                col->center = glm::vec3(0.0f, 0.9f, 0.0f);

                std::cout << "Tornado " << i << " at: "
                          << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
            }
        }
    };

}