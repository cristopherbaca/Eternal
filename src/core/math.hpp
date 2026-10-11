#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace er {
constexpr float pi=3.14159265358979323846f;
template<class T> T clamp(T v,T lo,T hi) { return std::max(lo,std::min(hi,v)); }
struct Vec2 {
    float x=0,y=0;
    Vec2 operator+(Vec2 b) const { return {x+b.x,y+b.y}; }
    Vec2 operator-(Vec2 b) const { return {x-b.x,y-b.y}; }
    Vec2 operator*(float s) const { return {x*s,y*s}; }
    Vec2 operator/(float s) const { return {x/s,y/s}; }
    Vec2& operator+=(Vec2 b) { x+=b.x; y+=b.y; return *this; }
};
inline float dot(Vec2 a,Vec2 b) { return a.x*b.x+a.y*b.y; }
inline float length(Vec2 v) { return std::sqrt(dot(v,v)); }
inline Vec2 normalized(Vec2 v) { float n=length(v); return n>0.0001f?v/n:Vec2{}; }
inline float distance(Vec2 a,Vec2 b) { return length(a-b); }
inline Vec2 lerp(Vec2 a,Vec2 b,float t) { return a+(b-a)*t; }
inline uint32_t hash(uint32_t n) { n^=n>>16; n*=0x7feb352du; n^=n>>15; n*=0x846ca68bu; return n^(n>>16); }
inline float noise(int x,int y,uint32_t seed=1) { return float(hash(uint32_t(x)*73856093u^uint32_t(y)*19349663u^seed)&65535u)/65535.f; }
struct Color {
    uint8_t r=0,g=0,b=0;
    bool operator==(Color c) const { return r==c.r&&g==c.g&&b==c.b; }
    bool operator!=(Color c) const { return !(*this==c); }
};
inline Color rgb(float r,float g,float b) { return {uint8_t(clamp(r,0.f,255.f)),uint8_t(clamp(g,0.f,255.f)),uint8_t(clamp(b,0.f,255.f))}; }
inline Color operator*(Color c,float s) { return rgb(c.r*s,c.g*s,c.b*s); }
inline Color operator+(Color a,Color b) { return rgb(float(a.r)+b.r,float(a.g)+b.g,float(a.b)+b.b); }
inline Color mix(Color a,Color b,float t) { return rgb(a.r+(b.r-a.r)*t,a.g+(b.g-a.g)*t,a.b+(b.b-a.b)*t); }
constexpr Color background{11,11,26},white{216,228,224},muted{105,132,146};
}
