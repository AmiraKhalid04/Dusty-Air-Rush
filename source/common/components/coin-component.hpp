#pragma once

#include "../ecs/component.hpp"
#include <string>

namespace our
{

    // A tag component that marks an entity as a collectible coin.
    // CoinSystem looks for this component to identify coins in the world.
    class CoinComponent : public Component
    {
    public:
        static std::string getID() { return "Coin"; }

        // No data to deserialize – this is purely a tag component.
        void deserialize(const nlohmann::json &) override {}
    };

}
