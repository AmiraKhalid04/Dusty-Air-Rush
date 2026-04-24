#pragma once

namespace our
{
    struct TrackConfig
    {
        glm::vec3 startPosition = glm::vec3(-25.0f, 0.0f, 0.0f);
        glm::vec3 endPosition = glm::vec3(25.0f, 0.0f, -500.0f);
        int stagesCount = 10;
        float innerMargin = 5.0f;
        float outerMargin = 20.0f;
    };
}