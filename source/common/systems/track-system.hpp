#pragma once
#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
#include "../components/collider.hpp"
#include "../asset-loader.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <iostream>

namespace our
{
    struct TrackConfig
    {
        glm::vec3 startPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 endPosition = glm::vec3(50.0f, 0.0f, -300.0f);
        int stagesCount = 10;
        float innerMargin = 5.0f;
        float outerMargin = 20.0f;
    };
}