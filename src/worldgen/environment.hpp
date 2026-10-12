#pragma once
#include "world/world.hpp"
#include "entities/entity.hpp"
namespace er {
struct Animal { Vec2 pos,home; uint32_t key=0; bool bird=false; };
class Environment {
public:
    std::vector<Animal> animals;
    void tick(const World& world,Vec2 hero,const std::vector<Entity>& threats,float time,float dt);
    void render(Scene& scene,float time) const;
    void atmosphere(Framebuffer& frame,const Camera& camera,uint32_t seed,float time) const;
private: World::ChunkKey center{2147483647,2147483647};
};
}
