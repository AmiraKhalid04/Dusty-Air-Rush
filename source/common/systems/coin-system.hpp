#pragma once

#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
#include "../components/coin-component.hpp"
#include "../components/collider.hpp"
#include "../asset-loader.hpp"
#include "../utils/track-utils.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <random>

namespace our
{
    struct CoinConfig
    {
        // Track boundaries
        glm::vec3 trackStartPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 trackEndPosition = glm::vec3(0.0f, 0.0f, 300.0f);

        int coinsCount = 30;       // Total number of coin spawn positions
        float margin = 5.0f;       // Distance from track edges
        float depthOffset = 15.0f; // Offset from start to avoid intersecting with rings
        float spawnChance = 0.8f;  // Probability of spawning at each position

        float scale = 0.5f;
        float collectRadius = 0.4f;

        // Calculated at runtime
        float spacing = 0.0f;
    };

    // CoinSystem spawns coins along the track with proper offset to avoid intersecting with rings and other objects
    class CoinSystem
    {
        std::mt19937 rng{std::random_device{}()};

    public:
        bool initialized = false;

        void initialize(World *world, const CoinConfig &config)
        {
            if (initialized)
                return;
            initialized = true;

            Mesh *coinMesh = AssetLoader<Mesh>::get("coin");
            Material *coinMaterial = AssetLoader<Material>::get("coin");
            if (!coinMesh || !coinMaterial)
                return;

            // Calculate spacing from track depth and coins count
            float trackDepth = config.trackStartPosition.z - config.trackEndPosition.z;
            float calculatedSpacing = trackDepth / (config.coinsCount + 1.0f);

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

            for (int i = 0; i < config.coinsCount; i++)
            {
                cursor.z -= calculatedSpacing;

                // Random chance to skip this position
                if (chanceDist(rng) > config.spawnChance)
                    continue;

                Entity *coin = world->add();
                coin->name = "coin_" + std::to_string(i);

                // Random X and Y within track margins
                glm::vec3 spawnPos = {
                    xDist(rng) + trackCurveX(cursor.z),
                    yDist(rng),
                    cursor.z};

                coin->localTransform.position = spawnPos;
                coin->localTransform.scale = glm::vec3(config.scale);
                coin->localTransform.rotation = glm::vec3(0, 0, 0);

                auto *mr = coin->addComponent<MeshRendererComponent>();
                mr->mesh = coinMesh;
                mr->material = coinMaterial;

                coin->addComponent<CoinComponent>();
                // Dynamic collider for collision detection

                auto *col = coin->addComponent<ColliderComponent>();
                col->shapeType = ColliderType::Sphere;
                col->objectType = "coin";
                col->sphereRadius = 0.4f;
                col->center = glm::vec3(0.0f, 0.8f, 0.0f);

                std::cout << "Coin " << i << " at: "
                          << spawnPos.x << ", " << spawnPos.y << ", " << spawnPos.z << std::endl;
            }
        }

        void reset() { initialized = false; }
    };
}