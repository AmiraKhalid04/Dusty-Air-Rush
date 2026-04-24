#pragma once

#include "../ecs/component.hpp"
#include <glm/glm.hpp>

namespace our
{
    // Component for tornado-specific movement: rotation around Y-axis and left-right oscillation
    class TornadoMovementComponent : public Component
    {
    public:
        // Angular velocity around Y axis (radians per second)
        float angularVelocityY = 0.0f;
        
        // Speed of left-right movement along X axis (units per second)
        float moveSpeedX = 0.0f;
        
        // Left and right boundary positions along X axis
        float xBoundaryLeft = 0.0f;
        float xBoundaryRight = 0.0f;
        
        // Current direction: 1 for moving right, -1 for moving left
        int currentDirection = 1;
        
        // Initial X position to track oscillation
        float initialX = 0.0f;

        static std::string getID() { return "Tornado Movement"; }
        
        void deserialize(const nlohmann::json &) override {}
    };
}
