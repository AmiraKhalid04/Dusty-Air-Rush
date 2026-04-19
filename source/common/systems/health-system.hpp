#pragma once
#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
#include "../components/collider.hpp"
#include "../components/coin-component.hpp" // reuse or make a BandageComponent
#include "../asset-loader.hpp"
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
        int bandageCount = 10;    // max possible bandages (one slot per ring)
        float spawnChance = 0.4f; // probability of spawning at each ring (0-1)
        int minRingsBefore = 2;   // don't spawn before this many rings have passed

        float scale = 0.5f;
        float collectRadius = 2.0f;

        // Offset from ring center — bandage appears beside/above/below the ring
        float maxSideOffset = 6.0f; // max X offset
        float maxVertOffset = 4.0f; // max Y offset
    };

    class HealthPackSystem
    {
        std::mt19937 rng{std::random_device{}()};

    public:
        void initialize(World *world,
                        const std::vector<glm::vec3> &ringPositions,
                        const HealthPackConfig &config)
        {
            Mesh *mesh = AssetLoader<Mesh>::get("health_pack");
            Material *mat = AssetLoader<Material>::get("health_pack");
            if (!mesh || !mat)
                return;

            std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
            std::uniform_real_distribution<float> sideDist(-1.0f, 1.0f);
            std::uniform_real_distribution<float> vertDist(-1.0f, 1.0f);

            for (int i = config.minRingsBefore; i < (int)ringPositions.size(); i++)
            {
                // Random chance to skip this ring entirely
                if (chanceDist(rng) > config.spawnChance)
                    continue;

                glm::vec3 ringPos = ringPositions[i];

                // Fully random offset in all directions around the ring
                float dx = sideDist(rng) * config.maxSideOffset;
                float dy = vertDist(rng) * config.maxVertOffset;

                glm::vec3 spawnPos = {
                    ringPos.x + dx,
                    ringPos.y + dy,
                    ringPos.z};

                Entity *entity = world->add();
                entity->name = "health_pack_" + std::to_string(i);

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