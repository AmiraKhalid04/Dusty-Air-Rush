#pragma once

#include "../ecs/component.hpp"
#include <algorithm>
#include <string>

namespace our
{

    class HealthComponent : public Component
    {
    public:
        float maxHealth = 100.0f;
        float currentHealth = 100.0f;

        static std::string getID() { return "Health"; }

        void deserialize(const nlohmann::json &data) override
        {
            maxHealth = data.value("maxHealth", maxHealth);
            currentHealth = data.value("currentHealth", currentHealth);
            currentHealth = std::clamp(currentHealth, 0.0f, maxHealth);
        }
    };

}
