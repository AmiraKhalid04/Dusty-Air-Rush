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
    struct RingArrowConfig
    {
        // How far above the ring origin the arrow hovers (base height)
        float baseOffsetY = 0.8f;
        // Total vertical travel of the bob animation
        float bobAmplitude = 0.1f;
        // Cycles per second
        float bobFrequency = 0.3f;
        // Stagger phase per ring index so they don't all move in unison
        float phaseStep = 0.55f;
    };

    class RingArrowSystem
    {
    private:
        RingArrowConfig config;

    public:
        void initialize(World* world, const RingArrowConfig& arrowConfig = RingArrowConfig())
        {
            config = arrowConfig;
            
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
                arrow->localTransform.position = { 0.0f, config.baseOffsetY, 0.0f };
                arrow->localTransform.rotation = { 0.0f, glm::pi<float>() / 2.0f, 0.0f }; // flip tip down
                arrow->localTransform.scale    = { 1.0f, 1.0f, 1.0f };

                auto* mr   = arrow->addComponent<MeshRendererComponent>();
                mr->mesh     = arrowMesh;
                mr->material = arrowMaterial;

                ++spawned;
            }

            std::cout << "[RingArrowSystem] Spawned " << spawned << " ring arrows.\n";
        }

        void update(World* world)
        {
            const float time = static_cast<float>(glfwGetTime());

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

                if (!parentRing || aliveRings.find(parentRing) == aliveRings.end())
                {
                    // Move off-screen
                    entity->localTransform.scale = glm::vec3(0.0f);
                    continue;
                }

                // Restore scale
                entity->localTransform.scale = { 1.0f, 1.0f, 1.0f };

                // Bob animation: sinusoidal Y offset relative to base height
                float phase  = idx * config.phaseStep;
                float offset = config.bobAmplitude * glm::sin(config.bobFrequency * glm::two_pi<float>() * time + phase);
                entity->localTransform.position.y = config.baseOffsetY + offset;
            }
        }
    };

} // namespace our