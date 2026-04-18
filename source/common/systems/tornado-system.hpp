#pragma once
#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
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
        int tornadoCount = 8;

        float spacing = 50.0f;         // MUST match ring spacing
        float heightVariance = 15.0f;  // MUST match ring config
        float lateralVariance = 10.0f; // MUST match ring config

        float sideOffset = 20.0f; // distance from ring center (left/right)
        float depthOffset = 20.0f;
        float scale = 10.0f;

        float spawnChance = 0.7f;
    };

    class TornadoSystem
    {
    public:
        void initialize(World *world, const TornadoConfig &config)
        {
            Mesh *mesh = AssetLoader<Mesh>::get("tornado");
            Material *mat = AssetLoader<Material>::get("tornado");

            if (!mesh || !mat)
                return;

            std::random_device rd;
            std::mt19937 rng(rd());
            std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);

            glm::vec3 cursor = glm::vec3(0, 2, 0);

            for (int i = 0; i < config.tornadoCount; i++)
            {
                cursor.z -= config.spacing;

                // SAME track logic as rings
                float baseY = 2.0f + config.heightVariance * sin(i * 0.4f);
                float baseX = config.lateralVariance * sin(i * 0.3f + 1.0f);

                // decide spawn
                if (chanceDist(rng) > config.spawnChance)
                    continue;

                Entity *entity = world->add();
                entity->name = "tornado_" + std::to_string(i);

                // left or right of ring center
                float side = (rand() % 2 == 0) ? 1.0f : -1.0f;

                glm::vec3 pos;
                pos.z = cursor.z + config.depthOffset;
                pos.y = baseY;
                pos.x = baseX + side * config.sideOffset;

                entity->localTransform.position = pos;

                // scale (2,1,2 style but configurable)
                float base = config.scale;
                entity->localTransform.scale = glm::vec3(base, base * 0.5f, base);

                // optional rotation
                entity->localTransform.rotation = glm::vec3(0, 0, 0);

                auto *mr = entity->addComponent<MeshRendererComponent>();
                mr->mesh = mesh;
                mr->material = mat;

                std::cout << "Tornado " << i << " at: "
                          << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
            }
        }
    };

}