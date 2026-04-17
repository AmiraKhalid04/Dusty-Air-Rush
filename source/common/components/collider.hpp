#pragma once

#include "../ecs/component.hpp"
#include <glm/glm.hpp>
#include <string>

namespace our {

    // Identifies the type of geometric volume this collider represents
    enum class ColliderType {
        Sphere,
        AABB
    };

    class ColliderComponent : public Component {
    public:
        ColliderType shapeType = ColliderType::Sphere;
        
        // This is used by the collision system to identify WHAT this object functionally is 
        std::string objectType = "default";
        
        // General offset from the Entity's origin
        glm::vec3 center = {0, 0, 0};
        
        // Specific to Sphere type:
        float sphereRadius = 1.0f;
        
        // Specific to AABB type (half dimensions: size from center to edge)
        glm::vec3 aabbExtents = {0.5f, 0.5f, 0.5f};

        // The ID of this component type is "Collider"
        static std::string getID() { return "Collider"; }

        // Reads the bounds config from the given json object
        void deserialize(const nlohmann::json& data) override;
    };

}
