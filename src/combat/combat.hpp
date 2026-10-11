#pragma once
#include "assets/data.hpp"
#include "world/world.hpp"
namespace er {
struct Particle { Vec2 pos,velocity; float height=1,life=1,maxLife=1; Color color{}; char32_t glyph=U'.'; };
struct Projectile { Vec2 pos,velocity; int owner=0; float power=0,radius=1,life=2; std::string effect; Color color{}; std::vector<int> hitIds{}; };
struct Area { Vec2 pos; float radius=4,life=4,delay=0,interval=1,timer=0,power=20; int owner=1; std::string effect="fire"; Vec2 direction{1,0}; };
bool hostile(const Entity& a,const Entity& b);
float damageAfterArmor(float power,float armor);
float dealDamage(Entity& e,float power,std::vector<FloatingText>& texts);
float heal(Entity& e,float amount,std::vector<FloatingText>& texts);
class Combat {
public:
    std::vector<Particle> particles; std::vector<Projectile> projectiles; std::vector<Area> areas;
    std::string cast(Entity& caster,int slot,Vec2 aim,int target,const std::vector<Entity*>& entities,const Data& data,World& world,std::vector<FloatingText>& texts);
    void tick(float dt,const std::vector<Entity*>& entities,World& world,std::vector<FloatingText>& texts);
    void render(Scene& scene,float time) const;
    void burst(Vec2 pos,Color color,int count=26,float radius=4);
private: uint32_t serial=0;
};
}
