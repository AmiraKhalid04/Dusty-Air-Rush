#pragma once
#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
#include "../asset-loader.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>

namespace our
{
    // -----------------------------------------------------------------------
    //  RingArrowSystem
    //
    //  Spawns a downward-pointing cone arrow above every ring when the track
    //  is initialised.  Each frame it:
    //    • bobs the arrow up-and-down with a sine wave
    //    • removes the arrow once the ring it belongs to has been passed
    //      (detected by the ring's "ring_score_gate" child being gone from
    //       the world – the collision system marks it for removal when the
    //       player flies through the centre hole).
    // -----------------------------------------------------------------------
    class RingArrowSystem
    {
        // How far above the ring origin the arrow hovers (base height)
        static constexpr float BASE_OFFSET_Y  = 0.8f;
        // Total vertical travel of the bob animation
        static constexpr float BOB_AMPLITUDE  = 0.1f;
        // Cycles per second
        static constexpr float BOB_FREQUENCY  = 0.3f;
        // Stagger phase per ring index so they don't all move in unison
        static constexpr float PHASE_STEP     = 0.55f;

    public:
        // Call this AFTER RingSystem::initialize() so the ring entities exist.
        // It iterates the world looking for entities named "ring_N" and adds
        // a child arrow entity to each one.
        void initialize(World* world)
        {
            Mesh*     arrowMesh     = AssetLoader<Mesh>::get("arrow");
            Material* arrowMaterial = AssetLoader<Material>::get("arrow");

            if (!arrowMesh || !arrowMaterial)
            {
                std::cout << "[RingArrowSystem] 'arrow' mesh or material not found — "
                             "arrows will not be shown.\n";
                return;
            }

            int spawned = 0;
            for (auto* entity : world->getEntities())
            {
                // Match ring entities: name starts with "ring_" but is NOT
                // one of the child collider/trigger entities.
                const std::string& name = entity->name;
                if (name.rfind("ring_", 0) != 0)          continue; // not a ring
                if (name.find("frame") != std::string::npos) continue; // ring_frame_N
                if (name.find("score") != std::string::npos) continue; // ring_score_gate

                // e.g. "ring_0", "ring_1", …
                // Parse index for phase offset
                int idx = 0;
                try { idx = std::stoi(name.substr(5)); } catch (...) { idx = spawned; }

                Entity* arrow = world->add();
                arrow->parent = entity;
                arrow->name   = "ring_arrow_" + std::to_string(idx);

                // Arrow points up by default in most DCC tools; rotate 180° on Z
                // so the tip points DOWN toward the ring.
                arrow->localTransform.position = { 0.0f, BASE_OFFSET_Y, 0.0f };
                arrow->localTransform.rotation = { 0.0f, glm::pi<float>() / 2.0f, 0.0f }; // flip tip down
                arrow->localTransform.scale    = { 1.0f, 1.0f, 1.0f };

                auto* mr   = arrow->addComponent<MeshRendererComponent>();
                mr->mesh     = arrowMesh;
                mr->material = arrowMaterial;

                ++spawned;
            }

            std::cout << "[RingArrowSystem] Spawned " << spawned << " ring arrows.\n";
        }

        // Call every frame (before rendering).
        void update(World* world, float /*deltaTime*/)
        {
            const float time = static_cast<float>(glfwGetTime());

            // Build a quick set of which ring indices still have their score
            // gate alive (i.e. the player hasn't passed through yet).
            // We detect a "passed" ring by the absence of its ring_score_gate child.
            // The collision system calls world->markForRemoval() on that child, and
            // play-state calls world->deleteMarkedEntities() after the collision
            // pass — so once it's gone from the entity list the ring is passed.

            // Collect alive score-gate parents
            std::unordered_set<Entity*> aliveRings;
            for (auto* entity : world->getEntities())
            {
                if (entity->name == "ring_score_gate" && entity->parent)
                    aliveRings.insert(entity->parent);
            }

            for (auto* entity : world->getEntities())
            {
                const std::string& name = entity->name;
                if (name.rfind("ring_arrow_", 0) != 0) continue;

                // Determine the ring index for phase offset
                int idx = 0;
                try { idx = std::stoi(name.substr(11)); } catch (...) {}

                Entity* parentRing = entity->parent;

                // If the parent ring no longer has a live score gate, hide arrow.
                // We do this by pushing the arrow far out of view (or we could
                // mark it for removal — but keeping it avoids iterator invalidation
                // and lets us just re-hide every frame cheaply).
                if (!parentRing || aliveRings.find(parentRing) == aliveRings.end())
                {
                    // Move off-screen; scale to zero is the cleanest approach.
                    entity->localTransform.scale = glm::vec3(0.0f);
                    continue;
                }

                // Restore scale (in case it was zeroed earlier by a bug)
                entity->localTransform.scale = { 1.0f, 1.0f, 1.0f };

                // Bob animation: sinusoidal Y offset relative to base height
                float phase  = idx * PHASE_STEP;
                float offset = BOB_AMPLITUDE * glm::sin(BOB_FREQUENCY * glm::two_pi<float>() * time + phase);
                entity->localTransform.position.y = BASE_OFFSET_Y + offset;
            }
        }
    };

} // namespace our