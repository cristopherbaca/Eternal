#include "world/world.hpp"
namespace er {
World::World(uint32_t s):seed(s) {
    for(int y=-65;y<65;y+=6) for(int x=-85;x<85;x+=6) {
        Vec2 p{float(x)+noise(x,y,s)*5,float(y)+noise(y,x,s+1)*5};
        if(water(p)||road(p)||(std::abs(p.x)<23&&p.y>-24&&p.y<4)||distance(p,{0,12})<9||distance(p,{30,-25})<16) continue;
        if(noise(x,y,s+8)>.62f) continue;
        trees.push_back({p,6+noise(x,y,s+3)*7,2+noise(y,x,s+9)*2,noise(x,y,s+4)>.45f});
    }
}
bool World::road(Vec2 p) const { return std::abs(p.x-p.y*.55f+8)<2.8f||std::abs(p.y+3+p.x*.14f)<1.6f; }
bool World::water(Vec2 p) const { return p.y>shore(p.x); }
bool World::blocked(Vec2 p) const {
    if(std::abs(p.x)>88||std::abs(p.y)>68) return true;
    if(std::abs(p.y+10)<.7f&&std::abs(p.x)<21&&std::abs(p.x)>3.3f) return true;
    if(std::abs(p.y+22)<.7f&&std::abs(p.x)<21) return true;
    for(const auto& t:trees) if(distance(p,t.pos)<.8f) return true;
    return false;
}
Vec2 World::move(Vec2 p,Vec2 d) const {
    Vec2 next=p+d; if(!blocked(next)) return next;
    if(!blocked({next.x,p.y})) p.x=next.x; if(!blocked({p.x,next.y})) p.y=next.y; return p;
}
void World::render(Scene& s,float time) const {
    const Camera& c=s.camera;
    for(int y=0;y<c.height;++y) for(int x=0;x<c.width;++x) {
        Vec2 p=c.unproject({float(x),float(y)}); int gx=int(std::floor(p.x*3)),gy=int(std::floor(p.y*3)); float n=noise(gx,gy,seed);
        if(water(p)) { s.put(x,y,p,0,{17,28,43},n>.95f?U'~':U' ',p.y-10); continue; }
        if(road(p)) {
            char32_t glyph=U"o0QOa.:"[int(n*6.99f)]; Color color=mix({66,81,90},{135,150,148},n);
            if((gy%4+4)%4==0) { glyph=U'-'; color=color*.65f; }
            s.put(x,y,p,0,color,glyph,p.y-10); continue;
        }
        if(std::abs(p.x)<20&&p.y<-10&&p.y>-22) { s.put(x,y,p,0,{45,55,65},n>.65f?U'=':U'.',p.y-10); continue; }
        float wind=std::sin(p.x*.23f+p.y*.18f+time*1.6f)+.4f*std::sin(p.y*.8f-time*2);
        char32_t blade=wind>.45f?U'/':wind<-.45f?U'\\':U'|'; if(n<.2f) blade=U','; else if(n>.97f) blade=U'.';
        Color color=n>.97f?Color{166,192,169}:mix({22,53,47},{65,116,84},n);
        s.put(x,y,p,0,color,blade,p.y-10);
    }
    // A shallow stone volume: back facade, side piers, front arches and crenellations.
    for(float wallY:{-22.f,-10.f}) for(float x=-21;x<=21;x+=.35f/c.zoom) for(float z=0;z<10;z+=.48f/c.zoom) {
        float ax=std::abs(x); bool arch=ax<3.2f&&z<4.0f+std::sqrt(std::max(0.f,10.24f-x*x));
        bool slit=(std::abs(x+13)<.7f||std::abs(x-13)<.7f)&&z>4&&z<7;
        if(arch||slit||(z>8.4f&&int((x+21)/2.6f)%2==0)) continue;
        float n=noise(int(x*4),int(z*4),seed); bool mortar=int(z*3)%4==0;
        Color col=mix({58,69,81},{121,136,143},n); if(mortar) col=col*.65f;
        s.point({x,wallY},z,col,mortar?U'=':U"0O8o@#"[int(n*5.99f)],{0,-1,.2f});
        if(((x>6&&x<7.4f)||(x<-6&&x>-7.4f))&&z>4&&z<8) s.point({x,wallY-.04f},z,{135,40,48},U'|');
    }
    for(float x:{-21.f,21.f}) for(float y=-22;y<=-10;y+=.4f/c.zoom) for(float z=0;z<11;z+=.45f/c.zoom) {
        if(z>9.5f&&int(y+22)%4<2) continue;
        float n=noise(int(y*5),int(z*3),seed); s.point({x,y},z,mix({45,61,76},{100,120,128},n),U"08#o="[int(n*4.99f)],{1,0,.3f});
    }
    for(Vec2 p:{Vec2{-4,-9.7f},Vec2{4,-9.7f}}) { s.point(p,4,{255,180,77},U'*',{0,0,1},1); s.point(p,3.4f,{158,97,52},U'|'); }
    for(const Tree& t:trees) {
        Vec2 base=c.project(t.pos); float ht=t.height*c.zoom*1.5f,rx=t.radius*c.zoom*2.2f;
        if(base.x+rx<0||base.x-rx>c.width||base.y<0||base.y-ht>c.height) continue;
        for(int yy=int(base.y-ht);yy<=int(base.y);++yy) for(int xx=int(base.x-rx);xx<=int(base.x+rx);++xx) {
            float v=(base.y-yy)/ht,u=(xx-base.x)/rx; bool leaf=false; float n=noise(xx-int(base.x),yy-int(base.y),uint32_t(t.pos.x*13)+seed);
            if(t.conifer) { float tier=std::fmod(v*4,1.f); leaf=v>.16f&&std::abs(u)<(1-v)*(.65f+.5f*tier); }
            else { float q=(v-.61f)/.37f; leaf=u*u+q*q<1+n*.2f; }
            if(leaf&&n>.08f) {
                Color col=t.conifer?mix({43,87,100},{141,200,204},n):mix({20,57,60},{84,155,136},n);
                char32_t glyph=t.conifer?U'^':U"oaOQd0"[int(n*5.99f)];
                s.put(xx,yy,t.pos,v*t.height,col,glyph,t.pos.y+v*.1f,{u,-.3f,.8f});
            } else if(std::abs(u)<.13f&&v<.65f) s.put(xx,yy,t.pos,v*t.height,{73,76,67},U'|',t.pos.y,{0,-1,.3f});
        }
    }
}
void drawCharacter(Scene& s,Vec2 p,Color color,float time,bool moving) {
    static const char* sprite[]={"   ^   ","  (o)  "," /#|\\ ","  ###  ","  / \\ "};
    Vec2 base=s.camera.project(p); float stride=moving?std::sin(time*13):0;
    for(int y=0;y<5;++y) for(int x=0;sprite[y][x];++x) if(sprite[y][x]!=' ') {
        int xx=int(base.x)+x-3+(y==4?int(stride):0); Color col=y==1?Color{179,164,137}:color;
        s.put(xx,int(base.y)-5+y,p,float(5-y)*.7f,col,char32_t(sprite[y][x]),p.y+.2f,{0,-1,.7f});
    }
    s.put(int(base.x)+4,int(base.y)-3,p,2,{199,160,77},U'|',p.y+.3f);
}
}
