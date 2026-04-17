#pragma once

#include "../ecs/world.hpp"
#include "../components/collider.hpp"

#include <glm/glm.hpp>
#include <iostream>

namespace our
{
    class CollisionSystem {
        bool initialized = false;
        glm::vec3 playerStartPos = {0, 0, 0};

    public:
        void update(World* world, float deltaTime) {
            std::vector<Entity*> playerParts;
            
            // 1. Find all player compound parts
            for (auto entity : world->getEntities()) {
                ColliderComponent* col = entity->getComponent<ColliderComponent>();
                if (col && col->objectType == "player") {
                    playerParts.push_back(entity);
                }
            }

            if (playerParts.empty()) return;

            // 2. Check collisions against all other colliders
            for (auto other : world->getEntities()) {
                ColliderComponent* otherCollider = other->getComponent<ColliderComponent>();
                if (!otherCollider || otherCollider->objectType == "player" || otherCollider->objectType == "pending_deletion") continue;

                glm::mat4 otherMatrix = other->getLocalToWorldMatrix();
                glm::vec3 otherPos = glm::vec3(otherMatrix * glm::vec4(otherCollider->center, 1.0f));
                glm::vec3 otherScale = glm::vec3(glm::length(otherMatrix[0]), glm::length(otherMatrix[1]), glm::length(otherMatrix[2]));
                
                glm::vec3 otherMin = otherPos - (otherCollider->aabbExtents * otherScale);
                glm::vec3 otherMax = otherPos + (otherCollider->aabbExtents * otherScale);

                bool isColliding = false;

                for (auto player : playerParts) {
                    ColliderComponent* playerCollider = player->getComponent<ColliderComponent>();
                    glm::mat4 playerMatrix = player->getLocalToWorldMatrix();
                    glm::vec3 playerPos = glm::vec3(playerMatrix * glm::vec4(playerCollider->center, 1.0f));
                    glm::vec3 playerScale = glm::vec3(glm::length(playerMatrix[0]), glm::length(playerMatrix[1]), glm::length(playerMatrix[2]));

                    if (playerCollider->shapeType == ColliderType::Sphere && otherCollider->shapeType == ColliderType::Sphere) {
                        float distance = glm::distance(playerPos, otherPos);
                        if (distance < (playerCollider->sphereRadius * playerScale.x + otherCollider->sphereRadius * otherScale.x)) {
                            isColliding = true; break;
                        }
                    }
                    else if (playerCollider->shapeType == ColliderType::AABB && otherCollider->shapeType == ColliderType::Sphere) {
                        glm::vec3 pMin = playerPos - (playerCollider->aabbExtents * playerScale);
                        glm::vec3 pMax = playerPos + (playerCollider->aabbExtents * playerScale);
                        glm::vec3 clampedCenter = glm::clamp(otherPos, pMin, pMax);
                        if (glm::distance(clampedCenter, otherPos) < otherCollider->sphereRadius * otherScale.x) {
                            isColliding = true; break;
                        }
                    }
                    else if (playerCollider->shapeType == ColliderType::Sphere && otherCollider->shapeType == ColliderType::AABB) {
                        glm::vec3 clampedCenter = glm::clamp(playerPos, otherMin, otherMax);
                        if (glm::distance(clampedCenter, playerPos) < playerCollider->sphereRadius * playerScale.x) {
                            isColliding = true; break;
                        }
                    }
                    else if (playerCollider->shapeType == ColliderType::AABB && otherCollider->shapeType == ColliderType::AABB) {
                        glm::vec3 pMin = playerPos - (playerCollider->aabbExtents * playerScale);
                        glm::vec3 pMax = playerPos + (playerCollider->aabbExtents * playerScale);
                        if ((pMin.x <= otherMax.x && pMax.x >= otherMin.x) &&
                            (pMin.y <= otherMax.y && pMax.y >= otherMin.y) &&
                            (pMin.z <= otherMax.z && pMax.z >= otherMin.z)) {
                            isColliding = true; break;
                        }
                    }
                }

                // 3. Resolve Collisions Logically
                if (isColliding) {
                    if (otherCollider->objectType == "monkey") {
                        std::cout << "[SYSTEM] Monkey Collected! Disappearing..." << std::endl;
                        world->markForRemoval(other);
                        otherCollider->objectType = "pending_deletion";
                    } 
                    else if (otherCollider->objectType == "glass") {
                        std::cout << "[SYSTEM] Shattered through Glass Surface!" << std::endl;
                        world->markForRemoval(other);
                        otherCollider->objectType = "pending_deletion";
                    } 
                    else if (otherCollider->objectType == "moon" || otherCollider->objectType == "obstacle") {
                        std::cout << "[SYSTEM] Obstacle Hit! Evaporating..." << std::endl;
                        world->markForRemoval(other);
                        otherCollider->objectType = "pending_deletion";
                    }
                }
            }
        }
    };
}
