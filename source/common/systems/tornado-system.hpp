#pragma once
#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
#include "../components/collider.hpp"
#include "../components/tornado-movement.hpp"
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
        float spacing = 0.0f;

        float margin = 10.0f;
        float depthOffset = 20.0f;
        float scale = 10.0f;

        float spawnChance = 0.7f;

        float angularVelocity = 3.0f;
        float moveSpeedXMin = 5.0f;
        float moveSpeedXMax = 20.0f;
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

            // Random distribution for movement
            std::uniform_real_distribution<float> moveSpeedDist(config.moveSpeedXMin, config.moveSpeedXMax);

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

                // Add tornado movement component with random speeds
                auto *tornadoMove = entity->addComponent<TornadoMovementComponent>();
                tornadoMove->angularVelocityY = config.angularVelocity;
                tornadoMove->moveSpeedX = moveSpeedDist(rng);
                tornadoMove->initialX = posX;
                tornadoMove->xBoundaryLeft = xLeft;
                tornadoMove->xBoundaryRight = xRight;

                // Randomly decide starting direction
                tornadoMove->currentDirection = (sideDist(rng) < 0.5f) ? 1 : -1;

                std::cout << "Tornado " << i << " at: "
                          << pos.x << ", " << pos.y << ", " << pos.z
                          << " | Angular Vel: " << tornadoMove->angularVelocityY
                          << " rad/s | Move Speed X: " << tornadoMove->moveSpeedX
                          << " units/s | Direction: " << tornadoMove->currentDirection << std::endl;
            }
        }

        void update(World *world, float deltaTime)
        {
            for (auto entity : world->getEntities())
            {
                TornadoMovementComponent *tornadoMovement = entity->getComponent<TornadoMovementComponent>();

                if (tornadoMovement)
                {
                    // Apply angular velocity rotation around Y axis
                    entity->localTransform.rotation.y += deltaTime * tornadoMovement->angularVelocityY;

                    // Keep rotation in range [0, 2*pi] to avoid floating point overflow
                    if (entity->localTransform.rotation.y > glm::two_pi<float>())
                    {
                        entity->localTransform.rotation.y -= glm::two_pi<float>();
                    }

                    // Update position along X axis with current direction
                    float newX = entity->localTransform.position.x +
                                 (deltaTime * tornadoMovement->moveSpeedX * tornadoMovement->currentDirection);

                    // Check if we've reached or passed a boundary, and reverse direction if needed
                    if (tornadoMovement->currentDirection > 0)
                    {
                        // Moving right
                        if (newX >= tornadoMovement->xBoundaryRight)
                        {
                            newX = tornadoMovement->xBoundaryRight;
                            tornadoMovement->currentDirection = -1; // Reverse to move left
                        }
                    }
                    else
                    {
                        // Moving left
                        if (newX <= tornadoMovement->xBoundaryLeft)
                        {
                            newX = tornadoMovement->xBoundaryLeft;
                            tornadoMovement->currentDirection = 1; // Reverse to move right
                        }
                    }

                    entity->localTransform.position.x = newX;
                }
            }
        }
    };

}