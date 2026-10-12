#pragma once
#include "renderer/framebuffer.hpp"
#include <limits>
#include "renderer/camera.hpp"
namespace er {
struct Normal { float x=0,y=0,z=1; };
struct Pixel {
    Color color=background; char32_t shape=U' '; float depth=-1e9f; Vec2 world{}; float height=0; Normal normal{}; float emission=0;
};
struct Light { Vec2 pos; float height=2; Color color{255,130,40}; float radius=10,intensity=1; };
class Scene {
public:
    Camera camera; View3D cameraView{}; std::vector<Pixel> pixels; std::vector<Light> lights;
    void begin(Camera c) { camera=c; if(c.perspective) cameraView=c.view(); pixels.assign(size_t(c.width*c.height),{}); lights.clear(); }
    void put(int x,int y,Vec2 world,float h,Color color,char32_t glyph,float depth,Normal n={0,0,1},float emission=0) {
        if(x<0||y<0||x>=camera.width||y>=camera.height) return;
        if(camera.perspective) {
            const auto& v=cameraView;
            depth=-((world.x-v.eye[0])*v.forward[0]+(world.y-v.eye[1])*v.forward[1]+(camera.ground(world)+h-v.eye[2])*v.forward[2]);
            if(-depth<=v.lens[2]||-depth>=v.lens[3]) return;
        }
        auto& p=pixels[size_t(y*camera.width+x)]; if(depth>=p.depth) p={color,glyph,depth,world,h,n,emission};
    }
    void point(Vec2 p,float z,Color color,char32_t glyph,Normal n={0,0,1},float emission=0) {
        Vec2 s=camera.project(p,z); put(int(std::round(s.x)),int(std::round(s.y)),p,z,color,glyph,p.y+z*.012f,n,emission);
    }
    void convert(Framebuffer& f) const;
};
}
