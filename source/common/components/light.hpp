#pragma once

#include "../ecs/component.hpp"
#include <glm/glm.hpp>

namespace our {

    // An enum that defines the type of the light (DIRECTIONAL, POINT, or SPOT) 
    enum class LightType {
        DIRECTIONAL,
        POINT,
        SPOT
    };

    class LightComponent : public Component {
    public:
        LightType lightType = LightType::DIRECTIONAL;
        glm::vec3 diffuse = {1.0f, 1.0f, 1.0f};
        glm::vec3 specular = {1.0f, 1.0f, 1.0f};
        glm::vec3 attenuation = {1.0f, 0.0f, 0.0f}; // [constant, linear, quadratic]
        glm::vec2 cone_angles = {glm::radians(20.0f), glm::radians(30.0f)}; // [inner, outer]

        // The ID of this component type is "Light"
        static std::string getID() { return "Light"; }

        // Reads light parameters from the given json object
        void deserialize(const nlohmann::json& data) override;
    };

}
