#pragma once
#include "world/world.hpp"
namespace er {
struct TerrainVertex { std::array<float,4> position,normal,color; };
struct TerrainMesh { std::vector<TerrainVertex> vertices; };
class Terrain3D {
public:
    std::map<World::ChunkKey,TerrainMesh> chunks;
    uint64_t revision=0;
    void update(const World& world,Vec2 center);
private:
    const World* source=nullptr;
    uint32_t seed=0;
    GenerationConfig settings{};
};
}
