#pragma once
#include "world/world.hpp"
#include "platform/terminal.hpp"
#include "entities/entity.hpp"
namespace er {
class Game {
public:
    World world; Camera camera; Entity hero; std::vector<Entity> party,mobs; Entity dragon;
    std::vector<FloatingText> texts; Vec2 velocity{},aim{5,12}; float time=0; bool quit=false;
    explicit Game(uint32_t seed=42);
    void setScene(const std::string& name);
    void input(const Input& in); void tick(float dt); void render(Framebuffer& f,float ms);
private: float movementTime=0; Scene scene;
};
}
