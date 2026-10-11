#pragma once
#include "world/world.hpp"
#include "platform/terminal.hpp"
#include "entities/entity.hpp"
#include "combat/combat.hpp"
namespace er {
struct BossFight { bool active=false,won=false; float elapsed=0,ring=3,breath=6,wing=11,adds=16; int phase=1; };
class Game {
public:
    Data data; World world; Combat combat; Camera camera; Entity hero; std::vector<Entity> party,mobs; Entity dragon;
    std::vector<FloatingText> texts; Vec2 velocity{},aim{5,12}; float time=0; bool quit=false;
    bool classMenu=false,helpMenu=false; std::string message="Welcome to Ashenmere. C: class  H: help";
    bool inventoryMenu=false; int inventoryIndex=0; std::vector<Loot> loot;
    BossFight fight;
    explicit Game(uint32_t seed=42,const std::filesystem::path& dataPath={});
    void setScene(const std::string& name);
    void input(const Input& in); void tick(float dt); void render(Framebuffer& f,float ms);
    std::vector<Entity*> entities();
    void spawnMob(Vec2 pos,int archetype);
    void collectLoot(); void equipSelected(); void respawn(); void rewards();
    void resetBoss();
private: float movementTime=0; Scene scene; std::vector<int> casts;
};
}
