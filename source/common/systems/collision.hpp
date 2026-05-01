#pragma once

#include "../ecs/world.hpp"
#include "../components/collider.hpp"
#include "audio-system.hpp"
#include "text-popup-system.hpp"

#include <glm/glm.hpp>
#include <iostream>
#include <unordered_set>
#include "../components/dusty.hpp"

namespace our
{
    class CollisionSystem
    {
        bool initialized = false;
        glm::vec3 playerStartPos = {0, 0, 0};
        AudioSystem *audioSystem = nullptr;
        TextPopupSystem *textPopupSystem = nullptr;
        float dangerIntensity = 0.0f;

        static constexpr float tornadoProximityRadius = 20.0f;
        static constexpr float minProximityVolume = 0.1f;
        static constexpr float maxProximityVolume = 1.0f;

        // Track entities that are currently in a state of overlap to prevent repeated triggers
        std::unordered_set<Entity *> activeCollisions;

    public:
        void setAudioSystem(AudioSystem *audio) { audioSystem = audio; }
        void setTextPopupSystem(TextPopupSystem *tps) { textPopupSystem = tps; }
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

            // Track who we hit THIS frame to update activeCollisions at the end
            std::unordered_set<Entity *> frameCollisions;

            // 2. Check collisions against all other colliders
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
                    // Only the main body can trigger the score gate
                    if (otherCollider->objectType == "ring_score" && player->name != "Body Collider")
                    {
                        continue;
                    }

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

                // 3. Resolve Collisions Logically
                if (isColliding)
                {
                    Entity *collisionTracker = other;
                    // Group ring parts by their parent ring entity
                    if (otherCollider->objectType == "ring_score" || otherCollider->objectType == "ring_frame")
                    {
                        if (other->parent)
                            collisionTracker = other->parent;
                    }

                    frameCollisions.insert(collisionTracker);

                    DustyComponent* dusty = nullptr;
                    for (auto p : playerParts) {
                        Entity* root = p->parent ? p->parent->parent : nullptr;
                        if (root) {
                            dusty = root->getComponent<DustyComponent>();
                            if (dusty) break;
                        }
                    }

                    // Apply physics effects every frame while the player is inside the tornado
                    if (otherCollider->objectType == "tornado")
                    {
                        // `playerParts[0]` is the Body Collider. Its parent is the Plane Mesh.
                        // The Plane Mesh's parent is the Camera/Player root.
                        Entity *planeMesh = playerParts[0]->parent;
                        Entity *playerRoot = planeMesh ? planeMesh->parent : nullptr;

                        if (playerRoot)
                        {
                            // Calculate the XZ direction from the player to the tornado center
                            glm::vec3 pRootPos = glm::vec3(playerRoot->getLocalToWorldMatrix()[3]);
                            glm::vec3 toTornado = otherPos - pRootPos;
                            toTornado.y = 0.0f;

                            if (glm::length(toTornado) > 0.001f)
                            {
                                glm::vec3 dirToTornado = glm::normalize(toTornado);
                                // The player's forward vector (assuming -Z is forward in local space)
                                glm::vec3 forwardVec = glm::normalize(glm::vec3(playerRoot->getLocalToWorldMatrix() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
                                forwardVec.y = 0.0f;
                                if (glm::length(forwardVec) > 0.001f)
                                {
                                    forwardVec = glm::normalize(forwardVec);
                                }

                                // If dot product is > 0, we are generally facing/approaching the tornado
                                float approachDot = glm::dot(forwardVec, dirToTornado);

                                if (approachDot > 0.0f)
                                {
                                    // Pull the player inside towards the center
                                    float pullSpeed = 4.0f;
                                    playerRoot->localTransform.position += dirToTornado * pullSpeed * deltaTime;

                                    // Cross product Y is positive if tornado is to our right, negative if to our left
                                    float side = glm::cross(forwardVec, dirToTornado).y;
                                    float turnSpeed = 4.0f;
                                    playerRoot->localTransform.rotation.y += side * turnSpeed * deltaTime;
                                }
                            }
                        }
                    }

                    // Only trigger the collision logic if we weren't already colliding with this entity last frame
                    // (and haven't already processed another part of this same compound entity this frame)
                    if (activeCollisions.find(collisionTracker) == activeCollisions.end())
                    {
                        activeCollisions.insert(collisionTracker); // Prevent other parts of this ring dealing damage this frame

                        if (otherCollider->objectType == "coin")
                        {
                            std::cout << "[COLLECT] +1 Coin picked up! +100 SCORE" << std::endl;
                            if (dusty) {
                                dusty->coins += 1;
                                dusty->score += 100;
                            }
                            if (audioSystem)
                                audioSystem->playSound("assets/sounds/coin.mp3");
                            if (textPopupSystem)
                                textPopupSystem->spawn("+100 SCORE", {1.0f, 0.84f, 0.0f, 1.0f});
                            world->markForRemoval(other);
                            otherCollider->objectType = "pending_deletion";
                        }
                        else if (otherCollider->objectType == "health")
                        {
                            std::cout << "[COLLECT] +HP Health pack acquired!" << std::endl;
                            if (dusty) dusty->currentHealth = std::min(dusty->currentHealth + 20.0f, dusty->maxHealth);
                            if (audioSystem)
                                audioSystem->playSound("assets/sounds/bonus.mp3");
                            if (textPopupSystem)
                                textPopupSystem->spawn("+40 HP", {0.2f, 1.0f, 0.4f, 1.0f});
                            world->markForRemoval(other);
                            otherCollider->objectType = "pending_deletion";
                        }
                        else if (otherCollider->objectType == "ring_score")
                        {
                            std::cout << "[SCORE] +1! Passed through center! +1000 SCORE" << std::endl;
                            if (dusty) {
                                dusty->ringsPassed += 1;
                                dusty->score += 1000;
                            }
                            if (audioSystem)
                                audioSystem->playSound("assets/sounds/all-right.mp3");
                            if (textPopupSystem)
                                textPopupSystem->spawn("+1000 SCORE", {0.4f, 0.8f, 1.0f, 1.0f});
                            world->markForRemoval(other);
                            otherCollider->objectType = "pending_deletion";
                        }
                        else if (otherCollider->objectType == "ring_frame")
                        {
                            std::cout << "[DAMAGE] Hit the Ring Frame! -20 HP, -200 SCORE" << std::endl;
                            if (dusty) {
                                dusty->currentHealth -= 20.0f;
                                dusty->score -= 200;
                            }
                            if (audioSystem)
                                audioSystem->playSound("assets/sounds/ouch.mp3");
                            if (textPopupSystem)
                                textPopupSystem->spawn("-20 HP\n-200 SCORE", {1.0f, 0.2f, 0.2f, 1.0f});
                            if (dusty && dusty->currentHealth <= 0.0f) {
                                dusty->isDead = true;
                            }
                        }
                        else if (otherCollider->objectType == "tornado")
                        {
                            std::cout << "[DAMAGE] Hit hazard! -20 HP, -500 SCORE" << std::endl;
                            if (dusty) {
                                dusty->currentHealth -= 20.0f;
                                dusty->score -= 500;
                            }
                            if (audioSystem)
                                audioSystem->playSound("assets/sounds/ouch.mp3");
                            if (textPopupSystem)
                                textPopupSystem->spawn("-20 HP\n-500 SCORE", {1.0f, 0.2f, 0.2f, 1.0f});
                            if (dusty && dusty->currentHealth <= 0.0f) {
                                dusty->isDead = true;
                            }
                        }
                        else if (otherCollider->objectType == "finish_line")
                        {
                            if (dusty) {
                                if (dusty->ringsPassed >= dusty->totalRings) {
                                    dusty->isWon = true;

                                    std::cout << "\n=============================================" << std::endl;
                                    std::cout << "               GAME FINISHED!                " << std::endl;
                                    std::cout << " Rings Passed: " << dusty->ringsPassed << std::endl;
                                    std::cout << " Coins Collected: " << dusty->coins << std::endl;
                                    std::cout << " Final Health: " << dusty->currentHealth << "%" << std::endl;
                                    std::cout << " Final Score: " << dusty->score << std::endl;
                                    std::cout << "=============================================\n" << std::endl;
                                } else {
                                    std::cout << "You still need to collect " << (dusty->totalRings - dusty->ringsPassed) << " rings! Finish line rejected\n" << std::endl;
                                    if (textPopupSystem)
                                        textPopupSystem->spawn("You still need to collect " + std::to_string(dusty->totalRings - dusty->ringsPassed) + " rings!", {1.0f, 0.2f, 0.2f, 1.0f});
                                }
                            }
                        }
                    }
                }
            }
            // Sync state: anyone in frameCollisions is now 'active' for next frame
            activeCollisions = frameCollisions;
        }
    };
}
