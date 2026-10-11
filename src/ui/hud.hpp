#pragma once
#include "entities/entity.hpp"
#include "renderer/scene.hpp"
namespace er {
void box(Framebuffer& f,int x,int y,int w,int h,Color color=muted);
void bar(Framebuffer& f,int x,int y,int w,float value,float max,Color color);
void nameLabel(Framebuffer& f,const Camera& c,const Entity& e,bool targeted=false);
void floatingLabels(Framebuffer& f,const Camera& c,const std::vector<FloatingText>& texts);
void hud(Framebuffer& f,const Entity& hero,const std::vector<Entity>& party,const Entity* boss,float ms);
}
