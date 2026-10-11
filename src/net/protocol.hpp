#pragma once
#include "game.hpp"
namespace er::net {
struct Snapshot {
    uint64_t tick=0; float time=0; bool active=false; int phase=1;
    std::vector<Entity> entities; std::vector<Area> areas; std::vector<Projectile> projectiles; std::vector<Particle> particles; std::vector<FloatingText> texts; std::vector<Loot> loot;
};
std::string snapshot(Game& game,uint64_t tick);
Snapshot parseSnapshot(const std::string& message);
}
