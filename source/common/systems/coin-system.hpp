#pragma once

#include "../ecs/world.hpp"
#include "../components/camera.hpp"
#include "../components/free-camera-controller.hpp"
#include "../components/mesh-renderer.hpp"
#include "../components/movement.hpp"
#include "../components/coin-component.hpp"
#include "../asset-loader.hpp"
#include "../mesh/mesh.hpp"
#include "../material/material.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cstdlib>
#include <cmath>
#include <string>

namespace our
{

    // CoinSystem manages dynamic coin spawning and collection.
    //
    //  - Each frame it counts living coin entities (tagged with CoinComponent).
    //  - If fewer than maxCoins exist, it spawns new ones randomly around the player.
    //  - If the player walks within collectRadius of a coin, that coin is removed.
    //
    // Usage: call update(world, deltaTime) every frame from your game state (e.g. play-state.hpp).
    class CoinSystem
    {

        // ── Tuneable constants ─────────────────────────────────────────────────
        static constexpr int maxCoins = 8;           // max coins alive at once
        static constexpr float spawnRadius = 15.0f;  // how far from player coins can appear
        static constexpr float minRadius = 5.0f;     // minimum spawn distance (avoid spawning on player)
        static constexpr float collectRadius = 2.0f; // distance at which a coin is collected
        static constexpr float coinHeight = 0.5f;    // Y position of spawned coins
        // ──────────────────────────────────────────────────────────────────────

        // Returns a random float in [0, 1]
        static float randF()
        {
            return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        }

    public:
        void update(World *world, float deltaTime)
        {

            // ── 1. Find the player (entity that has both Camera + FreeCameraController) ──
            Entity *player = nullptr;
            for (auto entity : world->getEntities())
            {
                if (entity->getComponent<CameraComponent>() &&
                    entity->getComponent<FreeCameraControllerComponent>())
                {
                    player = entity;
                    break;
                }
            }
            if (!player)
                return;

            glm::vec3 playerPos = player->localTransform.position;

            // ── 2. Check each coin: collect if close enough ───────────────────
            int coinCount = 0;
            for (auto entity : world->getEntities())
            {
                if (!entity->getComponent<CoinComponent>())
                    continue;

                float dist = glm::length(playerPos - entity->localTransform.position);
                if (dist < collectRadius)
                {
                    world->markForRemoval(entity);
                }
                else
                {
                    coinCount++;
                }
            }
            // Actually remove collected coins now so the spawn step sees the
            // correct count and can immediately refill.
            world->deleteMarkedEntities();

            // ── 3. Spawn new coins until we reach maxCoins ───────────────────
            // Grab coin assets once (they are cached by AssetLoader).
            Mesh *coinMesh = AssetLoader<Mesh>::get("coin");
            Material *coinMaterial = AssetLoader<Material>::get("coin");

            while (coinCount < maxCoins)
            {
                // Random angle and random radius in [minRadius, spawnRadius]
                float angle = randF() * 2.0f * glm::pi<float>();
                float radius = minRadius + randF() * (spawnRadius - minRadius);

                float offsetX = std::cos(angle) * radius;
                float offsetZ = std::sin(angle) * radius;

                Entity *coin = world->add();
                coin->name = "coin_dynamic";

                coin->localTransform.position = {
                    playerPos.x + offsetX,
                    coinHeight,
                    playerPos.z + offsetZ};
                coin->localTransform.scale = {0.5f, 0.5f, 0.5f};

                // Render the coin mesh
                auto *mr = coin->addComponent<MeshRendererComponent>();
                mr->mesh = coinMesh;
                mr->material = coinMaterial;

                // Spin around Y axis (180 degrees per second)
                auto *mv = coin->addComponent<MovementComponent>();
                mv->angularVelocity = {0.0f, glm::pi<float>(), 0.0f};

                // Tag so CoinSystem can identify it next frame
                coin->addComponent<CoinComponent>();

                coinCount++;
            }
        }
    };

}
