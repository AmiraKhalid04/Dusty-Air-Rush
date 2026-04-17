#pragma once
#include "../ecs/component.hpp"

namespace our
{
    // Tag component
    class TornadoTagComponent : public Component
    {
    public:
        bool passed = false;
        static std::string getID() { return "Tornado Tag"; }
        void deserialize(const nlohmann::json &) override {}
    };
}