#pragma once
#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
#include "../asset-loader.hpp"
#include <glm/glm.hpp>
#include <iostream>
#include <string>

namespace our
{
    struct RunwayLightConfig
    {
        glm::vec3 startPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 endPosition = glm::vec3(0.0f, 0.0f, 300.0f);
        float spacing = 6.0f;
        float scale = 1.0f;
    };

    class RunwayLightSystem
    {
    public:
        void initialize(World *world, const RunwayLightConfig &config)
        {
            Mesh *runwayLightMesh = AssetLoader<Mesh>::get("sphere");
            Material *runwayLightMaterial = AssetLoader<Material>::get("runway_light");

            if (!runwayLightMesh || !runwayLightMaterial)
            {
                std::cout << "[RunwayLightSystem] 'cone' mesh or material not found — skipping.\n";
                return;
            }

            int pairCount = 0;
            for (float z = config.startPosition.z; z >= config.endPosition.z; z -= config.spacing)
            {
                spawnCone(world,
                          "cone_left_" + std::to_string(pairCount),
                          glm::vec3(config.startPosition.x, config.startPosition.y, z),
                          config.scale,
                          runwayLightMesh, runwayLightMaterial);

                spawnCone(world,
                          "cone_right_" + std::to_string(pairCount),
                          glm::vec3(config.endPosition.x, config.startPosition.y, z),
                          config.scale,
                          runwayLightMesh, runwayLightMaterial);

                ++pairCount;
            }

            std::cout << "[RunwayLightSystem] Spawned " << pairCount * 2
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