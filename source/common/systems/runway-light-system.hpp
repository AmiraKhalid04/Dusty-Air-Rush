#pragma once
#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
#include "../asset-loader.hpp"
#include "../components/light.hpp"
#include "../utils/track-utils.hpp"
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
                float curveOffset = trackCurveX(z);
                
                spawnCone(world,
                          "cone_left_" + std::to_string(pairCount),
                          glm::vec3(config.startPosition.x + curveOffset, config.startPosition.y, z),
                          config.scale,
                          runwayLightMesh, runwayLightMaterial);

                spawnCone(world,
                          "cone_right_" + std::to_string(pairCount),
                          glm::vec3(config.endPosition.x + curveOffset, config.startPosition.y, z),
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

            auto *light = cone->addComponent<LightComponent>();
            light->lightType = LightType::POINT;
            light->diffuse = glm::vec3(1.0f, 0.6f, 0.2f); // Matching the tint from app.jsonc
            light->specular = glm::vec3(1.0f, 0.6f, 0.2f);
            light->attenuation = glm::vec3(1.0f, 0.05f, 0.01f);
        }
    };

}