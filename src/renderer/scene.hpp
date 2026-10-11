#pragma once
#include "renderer/framebuffer.hpp"
#include <limits>
namespace er {
struct Camera {
    Vec2 center{0,12}; float zoom=1; int width=200,height=60;
    float scale() const { return zoom*std::max(.7f,height/60.f); }
    Vec2 project(Vec2 p,float z=0) const { float zf=scale(); return {width*.5f+(p.x-center.x)*2.2f*zf,height*.55f+(p.y-center.y)*.9f*zf-z*1.5f*zf}; }
    Vec2 unproject(Vec2 s) const { float zf=scale(); return {center.x+(s.x-width*.5f)/(2.2f*zf),center.y+(s.y-height*.55f)/(.9f*zf)}; }
};
struct Normal { float x=0,y=0,z=1; };
struct Pixel {
    Color color=background; char32_t shape=U' '; float depth=-1e9f; Vec2 world{}; float height=0; Normal normal{}; float emission=0;
};
struct Light { Vec2 pos; float height=2; Color color{255,130,40}; float radius=10,intensity=1; };
class Scene {
public:
    Camera camera; std::vector<Pixel> pixels; std::vector<Light> lights;
    void begin(Camera c) { camera=c; pixels.assign(size_t(c.width*c.height),{}); lights.clear(); }
    void put(int x,int y,Vec2 world,float h,Color color,char32_t glyph,float depth,Normal n={0,0,1},float emission=0) {
        if(x<0||y<0||x>=camera.width||y>=camera.height) return;
        auto& p=pixels[size_t(y*camera.width+x)]; if(depth>=p.depth) p={color,glyph,depth,world,h,n,emission};
    }
    void point(Vec2 p,float z,Color color,char32_t glyph,Normal n={0,0,1},float emission=0) {
        Vec2 s=camera.project(p,z); put(int(std::round(s.x)),int(std::round(s.y)),p,z,color,glyph,p.y+z*.012f,n,emission);
    }
    void convert(Framebuffer& f) const;
};
}
