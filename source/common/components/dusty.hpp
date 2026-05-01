#pragma once

#include "../ecs/component.hpp"
#include <algorithm>
#include <string>

namespace our
{
    class DustyComponent : public Component
    {
    public:
        float maxHealth = 100.0f;
        float currentHealth = 100.0f;
        int coins = 0;
        int score = 0;
        int ringsPassed = 0;
        int totalRings = 0;
        bool isDead = false;
        bool isWon = false;

        static std::string getID() { return "dusty"; }

        void deserialize(const nlohmann::json &data) override
        {
            maxHealth = data.value("maxHealth", maxHealth);
            currentHealth = maxHealth; // Always start at full health
            coins = data.value("coins", coins);
            score = data.value("score", score);
            ringsPassed = data.value("ringsPassed", ringsPassed);
            totalRings = data.value("totalRings", totalRings);
        }
    };
}
