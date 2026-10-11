#include "game.hpp"
#include "ui/hud.hpp"
#include <cstdio>
namespace er {
Game::Game(uint32_t seed):world(seed) {
    Entity e; e.kind=Kind::Party; e.id=2; e.name="Stasia_apk"; e.role=Role::Cleric; e.pos={-4,14}; e.hp=e.maxHp=244; e.mp=e.maxMp=556; e.level=5; e.color={148,186,137}; party.push_back(e);
    e.id=3; e.name="Jade"; e.role=Role::Warrior; e.pos={5,16}; e.hp=e.maxHp=786; e.mp=e.maxMp=53; e.level=1; e.color={151,151,171}; party.push_back(e);
    e.id=4; e.name="Elowen"; e.role=Role::Mage; e.pos={-6,9}; e.hp=e.maxHp=183; e.mp=e.maxMp=350; e.level=7; e.color={132,137,196}; party.push_back(e);
    dragon.alive=false;
}
void Game::setScene(const std::string& name) {
    if(name=="water") hero.pos={0,34}; else if(name=="forest") hero.pos={-33,3}; else if(name=="ruins") hero.pos={0,-2}; else if(name=="boss") hero.pos={30,-17};
    camera.center=hero.pos; aim=hero.pos+Vec2{7,-3};
    for(size_t i=0;i<party.size();++i) party[i].pos=hero.pos+Vec2{float(i)*5-5,3};
}
void Game::input(const Input& in) {
    Vec2 direction{};
    for(char k:in.keys) {
        if(k=='q') quit=true;
        if(k=='w') direction.y-=1; if(k=='s') direction.y+=1;
        if(k=='a') direction.x-=1; if(k=='d') direction.x+=1;
        if(k=='='||k=='+') camera.zoom=clamp(camera.zoom+.1f,.55f,2.f); if(k=='-') camera.zoom=clamp(camera.zoom-.1f,.55f,2.f);
    }
    if(length(direction)>0) { velocity=normalized(direction)*8; movementTime=.16f; }
    if(in.scroll) camera.zoom=clamp(camera.zoom+in.scroll*.1f,.55f,2.f);
}
void Game::tick(float dt) {
    time+=dt; movementTime-=dt; if(movementTime<=0) velocity={};
    hero.pos=world.move(hero.pos,velocity*dt*(world.water(hero.pos)?.55f:1.f)); hero.velocity=velocity;
    world.tick(dt); world.pushGrass(hero.pos,2.8f,dt*30);
    for(const auto& e:party) world.pushGrass(e.pos,2.5f,dt*25);
    for(auto& t:texts) t.life-=dt;
    texts.erase(std::remove_if(texts.begin(),texts.end(),[](const auto& t){return t.life<=0;}),texts.end());
    camera.center=lerp(camera.center,hero.pos,1-std::exp(-dt*6));
}
void Game::render(Framebuffer& f,float ms) {
    camera.width=f.width; camera.height=f.height; scene.begin(camera); world.render(scene,time);
    drawCharacter(scene,hero.pos,hero.color,time,length(velocity)>0);
    for(const auto& e:party) drawCharacter(scene,e.pos,e.color,time);
    world.reflections(scene,time); scene.convert(f);
    nameLabel(f,camera,hero); for(const auto& e:party) nameLabel(f,camera,e);
    floatingLabels(f,camera,texts); hud(f,hero,party,nullptr,ms);
    f.text(2,f.height-2,"WASD move  +/- zoom  Q quit",muted);
}
}
