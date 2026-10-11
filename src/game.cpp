#include "game.hpp"
#include <cstdio>
namespace er {
void Game::setScene(const std::string& name) {
    if(name=="water") player={0,34}; else if(name=="forest") player={-33,3}; else if(name=="ruins") player={0,-2}; else if(name=="boss") player={30,-17};
    camera.center=player; aim=player+Vec2{7,-3};
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
    player=world.move(player,velocity*dt*(world.water(player)?.55f:1.f));
    world.tick(dt); world.pushGrass(player,2.8f,dt*30);
    camera.center=lerp(camera.center,player,1-std::exp(-dt*6));
}
void Game::render(Framebuffer& f,float ms) {
    camera.width=f.width; camera.height=f.height; scene.begin(camera); world.render(scene,time);
    drawCharacter(scene,player,{108,113,163},time,length(velocity)>0); world.reflections(scene,time); scene.convert(f);
    char label[80]; std::snprintf(label,sizeof(label),"FPS %.0f - Frame %.1f ms",1000.f/std::max(ms,.01f),ms); f.text(f.width-31,1,label,muted);
    f.text(2,f.height-2,"WASD move  +/- zoom  Q quit",muted);
}
}
