#include "light.hpp"
#include "../deserialize-utils.hpp"

namespace our {
    // Reads light parameters from the given json object
    void LightComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;

        std::string type = data.value("lightType", "directional");
        if (type == "point") {
            lightType = LightType::POINT;
        } else {
            lightType = LightType::DIRECTIONAL;
        }

        diffuse = data.value("color", diffuse);
        specular = data.value("color", specular);
        attenuation = data.value("attenuation", attenuation);
    }
}