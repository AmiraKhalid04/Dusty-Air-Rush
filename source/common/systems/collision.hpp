#pragma once

#include "../ecs/world.hpp"
#include "../components/collider.hpp"
#include "audio-system.hpp"

#include <glm/glm.hpp>
#include <iostream>

namespace our
{
    class CollisionSystem
    {
        bool initialized = false;
        glm::vec3 playerStartPos = {0, 0, 0};
        AudioSystem *audioSystem = nullptr;
        float dangerIntensity = 0.0f;

        static constexpr float tornadoProximityRadius = 20.0f;
        static constexpr float minProximityVolume = 0.1f;
        static constexpr float maxProximityVolume = 1.0f;

    public:
        void setAudioSystem(AudioSystem *audio) { audioSystem = audio; }
        float getDangerIntensity() const { return dangerIntensity; }

        void update(World *world, float deltaTime)
        {
            std::vector<Entity *> playerParts;

            // 1. Find all player compound parts
            for (auto entity : world->getEntities())
            {
                ColliderComponent *col = entity->getComponent<ColliderComponent>();
                if (col && col->objectType == "player")
                {
                    playerParts.push_back(entity);
                }
            }

            if (playerParts.empty())
            {
                dangerIntensity = 0.0f;
                if (audioSystem)
                    audioSystem->stopProximity();
                return;
            }

            // 2. Proximity scan against tornados (larger radius than collision)
            float minTornadoDistance = tornadoProximityRadius;
            bool tornadoInRange = false;

            for (auto other : world->getEntities())
            {
                ColliderComponent *otherCollider = other->getComponent<ColliderComponent>();
                if (!otherCollider || otherCollider->objectType != "tornado")
                    continue;

                glm::mat4 otherMatrix = other->getLocalToWorldMatrix();
                glm::vec3 otherPos = glm::vec3(otherMatrix * glm::vec4(otherCollider->center, 1.0f));

                for (auto player : playerParts)
                {
                    ColliderComponent *playerCollider = player->getComponent<ColliderComponent>();
                    glm::mat4 playerMatrix = player->getLocalToWorldMatrix();
                    glm::vec3 playerPos = glm::vec3(playerMatrix * glm::vec4(playerCollider->center, 1.0f));

                    float distance = glm::distance(playerPos, otherPos);
                    if (distance < minTornadoDistance)
                    {
                        minTornadoDistance = distance;
                        tornadoInRange = true;
                    }
                }
            }

            if (tornadoInRange)
            {
                float closeness = 1.0f - glm::clamp(minTornadoDistance / tornadoProximityRadius, 0.0f, 1.0f);
                dangerIntensity = closeness;

                if (audioSystem)
                {
                    audioSystem->startProximity("assets/sounds/tornado.mp3", minProximityVolume);
                    float volume = minProximityVolume + (maxProximityVolume - minProximityVolume) * closeness;
                    audioSystem->setProximityVolume(volume);
                }
            }
            else
            {
                dangerIntensity = 0.0f;
                if (audioSystem)
                    audioSystem->stopProximity();
            }

            // 3. Check collisions against all other colliders
            for (auto other : world->getEntities())
            {
                ColliderComponent *otherCollider = other->getComponent<ColliderComponent>();
                if (!otherCollider || otherCollider->objectType == "player" || otherCollider->objectType == "pending_deletion")
                    continue;

                glm::mat4 otherMatrix = other->getLocalToWorldMatrix();
                auto getAABB = [](const glm::mat4 &matrix, const glm::vec3 &extents, const glm::vec3 &center)
                {
                    glm::vec3 worldPos = glm::vec3(matrix * glm::vec4(center, 1.0f));
                    glm::mat3 absMatrix = glm::mat3(
                        glm::abs(matrix[0]),
                        glm::abs(matrix[1]),
                        glm::abs(matrix[2]));
                    glm::vec3 worldExtents = absMatrix * extents;
                    return std::make_pair(worldPos - worldExtents, worldPos + worldExtents);
                };

                auto worldAABB = getAABB(otherMatrix, otherCollider->aabbExtents, otherCollider->center);
                glm::vec3 otherMin = worldAABB.first;
                glm::vec3 otherMax = worldAABB.second;
                glm::vec3 otherPos = glm::vec3(otherMatrix * glm::vec4(otherCollider->center, 1.0f));
                glm::vec3 otherScale = glm::vec3(glm::length(otherMatrix[0]), glm::length(otherMatrix[1]), glm::length(otherMatrix[2]));

                bool isColliding = false;

                for (auto player : playerParts)
                {
                    ColliderComponent *playerCollider = player->getComponent<ColliderComponent>();
                    glm::mat4 playerMatrix = player->getLocalToWorldMatrix();
                    glm::vec3 playerPos = glm::vec3(playerMatrix * glm::vec4(playerCollider->center, 1.0f));
                    glm::vec3 playerScale = glm::vec3(glm::length(playerMatrix[0]), glm::length(playerMatrix[1]), glm::length(playerMatrix[2]));

                    if (playerCollider->shapeType == ColliderType::Sphere && otherCollider->shapeType == ColliderType::Sphere)
                    {
                        float distance = glm::distance(playerPos, otherPos);
                        if (distance < (playerCollider->sphereRadius * playerScale.x + otherCollider->sphereRadius * otherScale.x))
                        {
                            isColliding = true;
                            break;
                        }
                    }
                    else if (playerCollider->shapeType == ColliderType::AABB && otherCollider->shapeType == ColliderType::Sphere)
                    {
                        auto pAABB = getAABB(playerMatrix, playerCollider->aabbExtents, playerCollider->center);
                        glm::vec3 clampedCenter = glm::clamp(otherPos, pAABB.first, pAABB.second);
                        if (glm::distance(clampedCenter, otherPos) < otherCollider->sphereRadius * otherScale.x)
                        {
                            isColliding = true;
                            break;
                        }
                    }
                    else if (playerCollider->shapeType == ColliderType::Sphere && otherCollider->shapeType == ColliderType::AABB)
                    {
                        glm::vec3 clampedCenter = glm::clamp(playerPos, otherMin, otherMax);
                        if (glm::distance(clampedCenter, playerPos) < playerCollider->sphereRadius * playerScale.x)
                        {
                            isColliding = true;
                            break;
                        }
                    }
                    else if (playerCollider->shapeType == ColliderType::AABB && otherCollider->shapeType == ColliderType::AABB)
                    {
                        auto pAABB = getAABB(playerMatrix, playerCollider->aabbExtents, playerCollider->center);
                        glm::vec3 pMin = pAABB.first;
                        glm::vec3 pMax = pAABB.second;
                        if ((pMin.x <= otherMax.x && pMax.x >= otherMin.x) &&
                            (pMin.y <= otherMax.y && pMax.y >= otherMin.y) &&
                            (pMin.z <= otherMax.z && pMax.z >= otherMin.z))
                        {
                            isColliding = true;
                            break;
                        }
                    }
                }

                // 4. Resolve Collisions Logically
                if (isColliding)
                {
                    if (otherCollider->objectType == "coin")
                    {
                        std::cout << "[COLLECT] +1 Coin picked up!" << std::endl;
                        if (audioSystem)
                            audioSystem->playSound("assets/sounds/coin.mp3");
                        world->markForRemoval(other);
                        otherCollider->objectType = "pending_deletion";
                    }
                    else if (otherCollider->objectType == "health")
                    {
                        std::cout << "[COLLECT] +HP Health pack acquired!" << std::endl;
                        if (audioSystem)
                            audioSystem->playSound("assets/sounds/bonus.mp3");
                        world->markForRemoval(other);
                        otherCollider->objectType = "pending_deletion";
                    }
                    else if (otherCollider->objectType == "ring")
                    {
                        std::cout << "[TRIGGER] Flew through a Ring!" << std::endl;
                        world->markForRemoval(other);
                        otherCollider->objectType = "pending_deletion";
                    }
                    else if (otherCollider->objectType == "tornado" || otherCollider->objectType == "obstacle")
                    {
                        std::cout << "[DANGER] Hit a Tornado! Taking damage..." << std::endl;
                        world->markForRemoval(other);
                        otherCollider->objectType = "pending_deletion";
                    }
                }
            }
        }
    };
}
