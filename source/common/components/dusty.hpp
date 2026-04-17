#pragma once

#include "../ecs/component.hpp"
#include <string>

namespace our
{

    // A tag component that marks an entity as a collectible coin.
    // CoinSystem looks for this component to identify coins in the world.
    class DustyComponent : public Component
    {
    public:
        int score = 0;
        int health = 100;

        static std::string getID() { return "dusty"; }

        // Deserialize score from json if available
        void deserialize(const nlohmann::json &data) override
        {
            score = data.value("score", 0);
            health = data.value("health", 100);
        }
    };

}
