#pragma once
#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
#include "../asset-loader.hpp"
#include <glm/glm.hpp>
#include <iostream>
#include <string>

namespace our
{
    // Configuration for the cone boundary lane that flanks the ring track.
    // X and Y of every cone are fixed — only Z advances along the track.
    // Set coneLateralOffset > (lateralVariance + half ring diameter) so cones
    // always sit outside the playable corridor.
    struct ConeBoundaryConfig
    {
        float trackStartZ = 0.0f;       // Z where the first cone pair is placed
        float trackEndZ = -252.0f;      // Z of the last cone pair (should match ring track end)
        float coneSpacing = 6.0f;       // distance between consecutive pairs along Z
        float coneLateralOffset = 5.0f; // fixed |X| for all cones
        float coneY = 0.0f;
        float scale = 1.0f;         // uniform scale applied to every cone mesh
    };

    class ConeBoundarySystem
    {
    public:
        void initialize(World *world, const ConeBoundaryConfig &config)
        {
            Mesh *coneMesh = AssetLoader<Mesh>::get("cone");
            Material *coneMaterial = AssetLoader<Material>::get("cone");

            if (!coneMesh || !coneMaterial)
            {
                std::cout << "[ConeBoundarySystem] 'cone' mesh or material not found — skipping.\n";
                return;
            }

            int pairCount = 0;
            for (float z = config.trackStartZ; z >= config.trackEndZ; z -= config.coneSpacing)
            {
                spawnCone(world,
                          "cone_left_" + std::to_string(pairCount),
                          glm::vec3(-config.coneLateralOffset, config.coneY, z),
                          config.scale,
                          coneMesh, coneMaterial);

                spawnCone(world,
                          "cone_right_" + std::to_string(pairCount),
                          glm::vec3(config.coneLateralOffset, config.coneY, z),
                          config.scale,
                          coneMesh, coneMaterial);

                ++pairCount;
            }

            std::cout << "[ConeBoundarySystem] Spawned " << pairCount * 2
                      << " cones (" << pairCount << " pairs).\n";
        }

    private:
        void spawnCone(World *world, const std::string &name,
                       glm::vec3 pos, float scale,
                       Mesh *mesh, Material *material)
        {
            Entity *cone = world->add();
            cone->name = name;
            cone->localTransform.position = pos;
            cone->localTransform.scale = glm::vec3(scale);
            cone->localTransform.rotation = glm::vec3(0.0f);

            auto *mr = cone->addComponent<MeshRendererComponent>();
            mr->mesh = mesh;
            mr->material = material;
        }
    };

}