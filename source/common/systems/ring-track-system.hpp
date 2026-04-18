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

    struct RingTrackConfig
    {
        int ringCount = 20;
        float spacing = 12.0f;        // distance between rings
        float heightVariance = 3.0f;  // how much rings drift up/down
        float lateralVariance = 2.0f; // how much rings drift left/right
        float ringScale = 4.0f;
    };

    class RingTrackSystem
    {
    public:
        void initialize(World *world, const RingTrackConfig &config)
        {
            Mesh *ringMesh = AssetLoader<Mesh>::get("ring");
            Material *ringMaterial = AssetLoader<Material>::get("ring");

            if (!ringMesh || !ringMaterial)
                return;

            glm::vec3 cursor = glm::vec3(0, 2, 0); // starting position

            for (int i = 0; i < config.ringCount; i++)
            {
                Entity *entity = world->add();
                entity->name = "ring_" + std::to_string(i);

                // Advance along Z, with sinusoidal height & lateral drift
                cursor.z -= config.spacing;
                cursor.y = config.heightVariance * glm::sin(i * 0.4f);
                cursor.x = config.lateralVariance * glm::sin(i * 0.3f + 1.0f);
                std::cout << "Ring " << i << " position: " << cursor.x << ", " << cursor.y << ", " << cursor.z << std::endl;

                entity->localTransform.position = cursor;
                entity->localTransform.scale = glm::vec3(config.ringScale);

                // Tilt the ring to face the direction of travel
                glm::vec3 nextPos = cursor;
                nextPos.z -= config.spacing;
                nextPos.y = config.heightVariance * glm::sin((i + 1) * 0.4f);
                nextPos.x = config.lateralVariance * glm::sin((i + 1) * 0.3f + 1.0f);

                glm::vec3 dir = glm::normalize(nextPos - cursor);
                std::cout << "Ring " << i << " direction: " << dir.x << ", " << dir.y << ", " << dir.z << std::endl;

                // Compute rotation to align ring normal with direction
                float pitch = glm::degrees(glm::asin(glm::clamp(-dir.y, -1.0f, 1.0f)));
                float yaw = glm::degrees(std::atan2(dir.x, -dir.z));

                std::cout << "Ring " << i << " pitch: " << glm::degrees(pitch) << ", yaw: " << glm::degrees(yaw) / 2 << std::endl;

                entity->localTransform.rotation = glm::vec3(0, glm::pi<float>() / 2, 0.0f);

                auto *mr = entity->addComponent<MeshRendererComponent>();
                mr->mesh = ringMesh;
                mr->material = ringMaterial;

                // Dynamic collider for collision detection
                auto *col = entity->addComponent<ColliderComponent>();
                col->shapeType = ColliderType::AABB;
                col->objectType = "ring";
                col->aabbExtents = glm::vec3(0.01f, 0.2f, 0.2f);
                col->center = glm::vec3(0.0f, 0.2f, 0.0f);
            }

            // === FINISH LINE ===

            // Move one more step forward (after last ring)
            cursor.z -= config.spacing;

            int i = config.ringCount;

            // Compute same track position
            cursor.y = 2.0f + config.heightVariance * glm::sin(i * 0.4f);
            cursor.x = config.lateralVariance * glm::sin(i * 0.3f + 1.0f);

            // Create entity
            Entity *finish = world->add();
            finish->name = "finish_line";

            finish->localTransform.position = cursor;

            // Scale (adjust depending on your mesh)
            finish->localTransform.scale = glm::vec3(2.0f, 2.0f, 2.0f);

            // Optional: rotate to face player
            finish->localTransform.rotation = glm::vec3(0, 0, 0);

            // Load assets
            Mesh *finishMesh = AssetLoader<Mesh>::get("finish_line");
            Material *finishMaterial = AssetLoader<Material>::get("finish_line");

            if (finishMesh && finishMaterial)
            {
                auto *mr = finish->addComponent<MeshRendererComponent>();
                mr->mesh = finishMesh;
                mr->material = finishMaterial;
            }

            std::cout << "Finish line at: "
                      << cursor.x << ", " << cursor.y << ", " << cursor.z << std::endl;
        }
    };

}