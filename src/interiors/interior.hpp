#pragma once
#include "renderer/scene.hpp"
#include <array>
namespace er
{
    struct Interior
    {
        static constexpr int width = 49, height = 27;
        std::array<char, width * height> tiles{};
        bool peaceful = false, opened = false, trapSpent = false;
        Vec2 entrance{4, 13}, chest{43, 13};
        Interior(uint32_t seed, bool peaceful);
        char at(int x, int y) const { return x < 0 || y < 0 || x >= width || y >= height ? '#' : tiles[size_t(y * width + x)]; }
        bool blocked(Vec2 p) const { return at(int(std::round(p.x)), int(std::round(p.y))) == '#'; }
        void render(Scene &scene) const;
    };
}
