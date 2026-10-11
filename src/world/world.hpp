#pragma once
#include "renderer/scene.hpp"
namespace er {
struct Tree { Vec2 pos; float height=8,radius=3; bool conifer=false; };
class World {
public:
    uint32_t seed; std::vector<Tree> trees;
    explicit World(uint32_t seed=42);
    bool road(Vec2 p) const; bool water(Vec2 p) const;
    float shore(float x) const { return 37+3*std::sin(x*.09f)+1.5f*std::sin(x*.23f); }
    bool blocked(Vec2 p) const;
    Vec2 move(Vec2 p,Vec2 delta) const;
    void tick(float dt);
    void pushGrass(Vec2 pos,float radius,float strength);
    Vec2 grassBend(Vec2 pos) const;
    void render(Scene& scene,float time) const;
    void reflections(Scene& scene,float time) const;
private:
    struct Blade { Vec2 bend{},velocity{}; };
    std::vector<Blade> blades=std::vector<Blade>(181*141);
};
void drawCharacter(Scene& s,Vec2 p,Color color,float time,bool moving=false,int style=1);
}
