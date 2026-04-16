// ring-tag.hpp
#pragma once
#include "../ecs/component.hpp"

namespace our
{
    // Tag component — marks an entity as a passable ring
    class RingTagComponent : public Component
    {
    public:
        bool passed = false;
        static std::string getID() { return "Ring Tag"; }
        void deserialize(const nlohmann::json &) override {}
    };
}