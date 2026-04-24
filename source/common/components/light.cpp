#include "light.hpp"
#include "../deserialize-utils.hpp"

namespace our {
    // Reads light parameters from the given json object
    void LightComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;

        std::string type = data.value("lightType", "directional");
        if (type == "point") {
            lightType = LightType::POINT;
        } else if (type == "spot") {
            lightType = LightType::SPOT;
        } else {
            lightType = LightType::DIRECTIONAL;
        }

        diffuse = data.value("color", diffuse);
        specular = data.value("color", specular);
        attenuation = data.value("attenuation", attenuation);
        
        if (data.contains("cone_angles")) {
            cone_angles.x = glm::radians(data["cone_angles"][0].get<float>());
            cone_angles.y = glm::radians(data["cone_angles"][1].get<float>());
        }
    }
}
