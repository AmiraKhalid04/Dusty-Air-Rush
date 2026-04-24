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

    struct RingConfig
    {
        // Track boundaries
        glm::vec3 startPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 endPosition = glm::vec3(0.0f, 0.0f, 300.0f);
        int ringsCount = 10;

        // Calculated at runtime
        float spacing = 0.0f; // will be calculated as (endPosition.z - startPosition.z) / ringsCount

        // Ring appearance
        float heightVariance = 3.0f;  // how much rings drift up/down
        float lateralVariance = 2.0f; // how much rings drift left/right
        float ringScale = 4.0f;
        float finishLineScale = 2.0f;
    };

    class RingSystem
    {
    public:
        std::vector<glm::vec3> initialize(World *world, RingConfig &config)
        {
            // Calculate spacing from track depth and ring count
            float trackDepth = config.startPosition.z - config.endPosition.z;
            config.spacing = trackDepth / config.ringsCount;

            std::vector<glm::vec3> ringPositions;
            Mesh *ringMesh = AssetLoader<Mesh>::get("ring");
            Material *ringMaterial = AssetLoader<Material>::get("ring");

            if (!ringMesh || !ringMaterial)
                return ringPositions;
            ;

            glm::vec3 cursor = config.startPosition; // starting position

            for (int i = 0; i < config.ringsCount; i++)
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
                ringPositions.push_back(cursor); // Store position for coin placement

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

                // ─── PERSISTENT HAZARD SEGMENTS (The outer frame) ───
                float frameRadius = 0.16f; // Average radius of the ring geometry
                for (int j = 0; j < 12; j++)
                {
                    Entity *segment = world->add();
                    segment->parent = entity; // Follow the ring's transform
                    segment->name = "ring_frame_" + std::to_string(j);

                    float angle = j * (glm::pi<float>() / 6.0f);
                    segment->localTransform.position = {
                        0.0f,
                        glm::cos(angle) * frameRadius + 0.20f, // 0.16 + 0.04
                        glm::sin(angle) * frameRadius};

                    auto *col = segment->addComponent<ColliderComponent>();
                    col->shapeType = ColliderType::Sphere; // Spheres avoid the rotated-box 'fat AABB' issue
                    col->objectType = "ring_frame";
                    col->sphereRadius = 0.04f;
                }

                // // ─── 1 SCORE TRIGGER (The inner hole) ───
                Entity *trigger = world->add();
                trigger->parent = entity;
                trigger->name = "ring_score_gate";
                trigger->localTransform.position = {0, 0.20f, 0};

                auto *col = trigger->addComponent<ColliderComponent>();
                col->shapeType = ColliderType::Sphere;
                col->objectType = "ring_score";
                col->sphereRadius = 0.10f; // Leaves a safe gap between hole boundary and the frames
            }

            // === FINISH LINE ===

            // Move one more step forward (after last ring)
            cursor.z -= config.spacing;

            int i = config.ringsCount;

            // Compute same track position
            cursor.y = 2.0f;
            cursor.x = 0.0f;

            // Create entity
            Entity *finish = world->add();
            finish->name = "finish_line";

            finish->localTransform.position = cursor;

            // Scale (adjust depending on your mesh)
            finish->localTransform.scale = glm::vec3(config.finishLineScale);

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

            return ringPositions; // Return finish line position for camera targeting
        }
    };

}