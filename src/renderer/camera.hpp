#pragma once
#include "core/math.hpp"
#include <array>
namespace er {
class World;
struct Vec3 {
    float x=0,y=0,z=0;
    Vec3 operator+(Vec3 v) const { return {x+v.x,y+v.y,z+v.z}; }
    Vec3 operator-(Vec3 v) const { return {x-v.x,y-v.y,z-v.z}; }
    Vec3 operator*(float f) const { return {x*f,y*f,z*f}; }
};
inline float dot(Vec3 a,Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
struct View3D { std::array<float,4> eye,right,up,forward,lens; };
struct Camera {
    const World* terrain=nullptr;
    bool perspective=false,firstPerson=false;
    float jumpHeight=0;
    float eyeHeight=1.75f,lookPitch=8*pi/180;
    float yaw=0,tilt=55*pi/180,fov=55*pi/180;
    Vec2 center{0,12}; float zoom=1; int width=200,height=60;
    float scale() const { return zoom*std::max(.7f,height/60.f); }
    float ground(Vec2 p) const;
    float viewDepth(Vec2 p,float height=0) const;
    float deviceDepth(Vec2 p,float height=0) const;
    float spriteScale(Vec2 p,float height=2.4f) const;
    View3D view() const;
    Vec2 project(Vec2 p,float z=0) const;
    bool pick(Vec2 screen,Vec2& hit) const;
    Vec2 unproject(Vec2 screen) const;
};
}
