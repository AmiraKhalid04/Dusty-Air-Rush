#pragma once

#include "../ecs/world.hpp"
#include "../components/camera.hpp"
#include "../components/free-camera-controller.hpp"
#include "../components/mesh-renderer.hpp"
#include "../components/movement.hpp"
#include "../components/coin-component.hpp"
#include "../components/collider.hpp"
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
    // CoinSystem spawns coins along the ring track path with small random offsets,
    // so they guide the player through the course rather than scatter randomly.
    class CoinSystem
    {
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist;

        // ── Track path parameters — must match RingTrackConfig in play-state ──
        static constexpr float trackSpacing = 30.0f;    // same as trackConfig.spacing
        static constexpr float trackHeightVar = 15.0f;  // same as trackConfig.heightVariance
        static constexpr float trackLateralVar = 10.0f; // same as trackConfig.lateralVariance

        // ── Coin placement tunables ────────────────────────────────────────────
        static constexpr int coinsPerSegment = 3; // coins between each pair of rings
        static constexpr float xJitter = 2.5f;    // max random lateral offset
        static constexpr float yJitter = 3.5f;    // max random vertical offset
        static constexpr float collectRadius = 2.0f;
        // ──────────────────────────────────────────────────────────────────────

        // Total coins = ringCount * coinsPerSegment. We lazily spawn up to this.
        static constexpr int ringCount = 10; // must match trackConfig.ringCount
        static constexpr int maxCoins = ringCount * coinsPerSegment;

        // Returns the track centre position at a given ring index t (can be fractional).
        static glm::vec3 trackPosition(float t)
        {
            float z = -t * trackSpacing;
            float y = trackHeightVar * glm::sin(t * 0.4f);
            float x = trackLateralVar * glm::sin(t * 0.3f + 1.0f);
            return {x, y, z};
        }

    public:
        CoinSystem()
            : rng(std::random_device{}()), dist(-1.0f, 1.0f) {}

    public:
        bool initialized = false;

        void initialize(World *world, const std::vector<glm::vec3> &ringPositions)
        {
            if (initialized)
                return;
            initialized = true;

            Mesh *coinMesh = AssetLoader<Mesh>::get("coin");
            Material *coinMaterial = AssetLoader<Material>::get("coin");
            if (!coinMesh || !coinMaterial)
                return;

            for (int ring = 0; ring < (int)ringPositions.size() - 1; ring++)
            {
                std::cout << "ring position size: " << ringPositions.size() << std::endl;
                glm::vec3 a = ringPositions[ring];
                glm::vec3 b = ringPositions[ring + 1];

                for (int j = 0; j < coinsPerSegment; j++)
                {
                    float frac = (j + 1.0f) / (coinsPerSegment + 1.0f);
                    glm::vec3 base = glm::mix(a, b, frac); // Even spacing along the segment
                    float dx = dist(rng) * xJitter;
                    float dy = dist(rng) * yJitter;

                    Entity *coin = world->add();
                    coin->name = "coin_dynamic";
                    coin->localTransform.position = {base.x + dx, base.y + dy, base.z};
                    coin->localTransform.scale = {0.5f, 0.5f, 0.5f};

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
                }
            }
        }

        void reset() { initialized = false; }
    };
}