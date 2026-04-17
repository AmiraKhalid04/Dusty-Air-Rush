#include "collider.hpp"
#include "../ecs/entity.hpp"
#include "../deserialize-utils.hpp"

namespace our {
    // Reads Collider data from the given json object
    void ColliderComponent::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;
        
        std::string typeStr = data.value("shapeType", "sphere");
        if (typeStr == "aabb" || typeStr == "AABB") {
            shapeType = ColliderType::AABB;
        } else {
            shapeType = ColliderType::Sphere;
        }

        objectType = data.value("objectType", objectType);
        center = data.value("center", center);
        sphereRadius = data.value("sphereRadius", sphereRadius);
        aabbExtents = data.value("aabbExtents", aabbExtents);
    }
}
