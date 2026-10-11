#include "world/world.hpp"
namespace er {
void World::tick(float dt) {
    for(auto& b:blades) { b.velocity+=(b.bend*-22-b.velocity*7)*dt; b.bend+=b.velocity*dt; }
}
void World::pushGrass(Vec2 p,float radius,float strength) {
    int minx=clamp(int(p.x-grassCenter.x-radius)+90,0,180),maxx=clamp(int(p.x-grassCenter.x+radius)+90,0,180);
    int miny=clamp(int(p.y-grassCenter.y-radius)+70,0,140),maxy=clamp(int(p.y-grassCenter.y+radius)+70,0,140);
    for(int y=miny;y<=maxy;++y) for(int x=minx;x<=maxx;++x) {
        Vec2 d=Vec2{float(x-90),float(y-70)}+grassCenter-p; float len=length(d);
        if(len<radius) { auto& b=blades[size_t(y*181+x)]; b.velocity+=normalized(d)*(strength*(1-len/radius)); }
    }
}
Vec2 World::grassBend(Vec2 p) const { int x=clamp(int(std::round(p.x-grassCenter.x))+90,0,180),y=clamp(int(std::round(p.y-grassCenter.y))+70,0,140); return blades[size_t(y*181+x)].bend; }
World::World(uint32_t s,GenerationConfig settings):seed(s^settings.seedSalt),config(settings) { focus({0,12}); }
float World::terrainNoise(Vec2 p,uint32_t salt,float scale) const {
    float fx=p.x/scale,fy=p.y/scale; int x=int(std::floor(fx)),y=int(std::floor(fy));
    float u=fx-x,v=fy-y; u=u*u*(3-2*u); v=v*v*(3-2*v);
    float a=noise(x,y,seed+salt),b=noise(x+1,y,seed+salt),c=noise(x,y+1,seed+salt),d=noise(x+1,y+1,seed+salt);
    return (a+(b-a)*u)*(1-v)+(c+(d-c)*u)*v;
}
int World::biome(Vec2 p) const {
    if(std::abs(p.x)<180&&std::abs(p.y)<180) {
        if(p.x<-65&&p.y<20) return 1;
        if(p.x>65) return 2;
        if(p.y>70) return 3;
        return 0;
    }
    float climate=terrainNoise(p,991,config.biomeScale);
    return climate<.3f?1:climate>.7f?2:terrainNoise(p,157,config.biomeScale)<.4f?3:0;
}
bool World::treeAt(int x,int y,Tree& tree) const {
    if(config.treeDensity<=0||noise(x,y,seed+8)>config.treeDensity) return false;
    Vec2 p{float(x*8)+noise(x,y,seed)*6,float(y*8)+noise(y,x,seed+1)*6};
    for(const auto& landmark:landmarks) if(distance(p,landmark.pos)<7) return false;
    if(water(p)||road(p)||(std::abs(p.x)<23&&p.y>-24&&p.y<4)||distance(p,{0,12})<9||distance(p,{30,-25})<16) return false;
    tree={p,6+noise(x,y,seed+3)*7,2+noise(y,x,seed+9)*2,biome(p)==1||noise(x,y,seed+4)>.6f};
    return true;
}
void World::streamChunks(Vec2 pos) {
    auto key=chunkAt(pos); if(key==centerChunk) return;
    centerChunk=key;
    for(auto it=chunks.begin();it!=chunks.end();) {
        if(std::abs(it->first.first-key.first)>chunkRadius||std::abs(it->first.second-key.second)>chunkRadius) it=chunks.erase(it); else ++it;
    }
    for(int y=key.second-chunkRadius;y<=key.second+chunkRadius;++y) for(int x=key.first-chunkRadius;x<=key.first+chunkRadius;++x) {
        ChunkKey chunk{x,y}; if(chunks.count(chunk)) continue;
        auto& output=chunks[chunk];
        for(int gy=y*8;gy<(y+1)*8;++gy) for(int gx=x*8;gx<(x+1)*8;++gx) { Tree t; if(treeAt(gx,gy,t)) output.push_back(t); }
    }
    trees.clear();
    for(const auto& entry:chunks) trees.insert(trees.end(),entry.second.begin(),entry.second.end());
}
void World::focus(Vec2 pos) {
    bool changed=chunkAt(pos)!=centerChunk;
    streamChunks(pos);
    if(std::abs(pos.x-grassCenter.x)>=40||std::abs(pos.y-grassCenter.y)>=30) {
        grassCenter={std::round(pos.x/32)*32,std::round(pos.y/32)*32};
        std::fill(blades.begin(),blades.end(),Blade{}); changed=true;
    }
    if(changed) rebuildShadows();
}
void World::rebuildShadows() {
    std::fill(shadows.begin(),shadows.end(),1.f);
    for(const auto& tree:trees) {
        Vec2 center=tree.pos-grassCenter+Vec2{tree.height*.16f,1}; float radius=tree.radius+1;
        for(int y=clamp(int(center.y-radius)+70,0,140);y<=clamp(int(center.y+radius)+70,0,140);++y)
            for(int x=clamp(int(center.x-radius)+90,0,180);x<=clamp(int(center.x+radius)+90,0,180);++x) {
                float strength=clamp(1-distance(center,{float(x-90),float(y-70)})/radius,0.f,1.f);
                shadows[size_t(y*181+x)]*=1-strength*.68f;
            }
    }
}
bool World::road(Vec2 p) const {
    if(std::abs(p.x-p.y*.55f+8)<2.8f||std::abs(p.y+3+p.x*.14f)<1.6f) return true;
    // Continuous, gently winding routes repeat as corridors, not terrain tiles.
    float lane=p.x+12*std::sin(p.y*.009f);
    return std::abs(lane-std::round(lane/256)*256)<2.2f;
}
bool World::water(Vec2 p) const {
    float edge=std::max(std::abs(p.x),std::abs(p.y));
    if(edge<180) return p.y>shore(p.x)&&p.y<85+5*std::sin(p.x*.07f)&&std::abs(p.x)<72;
    if(road(p)) return false;
    float blend=clamp((edge-180)/60,0.f,1.f);
    return 1-blend+blend*terrainNoise(p,317,config.biomeScale*.7f)<config.lakeThreshold;
}
const char* World::region(Vec2 p) const {
    if(distance(p,{30,-30})<25) return "Dragon Hollow";
    const char* names[]={"Ashenmere Wilds","Frostwood","Ember Hills","Reedlands"}; return names[biome(p)];
}
bool World::blocked(Vec2 p) const {
    // Float-based entities have a documented precision envelope, not a 500-unit map wall.
    if(!std::isfinite(p.x)||!std::isfinite(p.y)||std::abs(p.x)>1000000||std::abs(p.y)>1000000) return true;
    if(std::abs(p.y+10)<.7f&&std::abs(p.x)<21&&std::abs(p.x)>3.3f) return true;
    if(std::abs(p.y+22)<.7f&&std::abs(p.x)<21) return true;
    int x=int(std::floor(p.x/8)),y=int(std::floor(p.y/8));
    for(int yy=y-1;yy<=y+1;++yy) for(int xx=x-1;xx<=x+1;++xx) { Tree t; if(treeAt(xx,yy,t)&&distance(p,t.pos)<.8f) return true; }
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
        float shadow=shadows[size_t(clamp(int(p.y-grassCenter.y)+70,0,140)*181+clamp(int(p.x-grassCenter.x)+90,0,180))];
        if(water(p)) { s.put(x,y,p,0,{17,28,43},n>.95f?U'~':U' ',p.y-10); continue; }
        if(road(p)) {
            char32_t glyph=U"o0QOa.:"[int(n*6.99f)]; Color color=mix({66,81,90},{135,150,148},n);
            if((gy%4+4)%4==0) { glyph=U'-'; color=color*.65f; }
            s.put(x,y,p,0,color*shadow,glyph,p.y-10); continue;
        }
        if(std::abs(p.x)<20&&p.y<-10&&p.y>-22) { s.put(x,y,p,0,{45,55,65},n>.65f?U'=':U'.',p.y-10); continue; }
        float wind=std::sin(p.x*.23f+p.y*.18f+time*1.6f)+.4f*std::sin(p.y*.8f-time*2);
        Vec2 bend=grassBend(p); wind+=bend.x*2;
        char32_t blade=wind>.45f?U'/':wind<-.45f?U'\\':U'|'; if(n<.2f) blade=U','; else if(n>.97f) blade=U'.';
        if(length(bend)>.55f) blade=std::abs(bend.x)>.3f?U'-':U',';
        Color color=n>.97f?Color{166,192,169}:mix({22,53,47},{65,116,84},n);
        int climate=biome(p);
        if(climate==2) color=mix(color,{135,95,62},.4f);
        else if(climate==1) color=mix(color,{106,159,178},.35f);
        else if(climate==3) color=mix(color,{46,111,93},.25f);
        color=color*(1-clamp(length(bend)*.28f,0.f,.55f))*shadow;
        s.put(x,y,p,0,color,blade,p.y-10);
    }
    if(std::abs(c.center.x)<120&&std::abs(c.center.y)<120) {
    // A shallow stone volume: back facade, side piers, front arches and crenellations.
    for(float wallY:{-22.f,-10.f}) for(float x=-21;x<=21;x+=.35f/c.scale()) for(float z=0;z<10;z+=.48f/c.scale()) {
        float ax=std::abs(x); bool arch=ax<3.2f&&z<4.0f+std::sqrt(std::max(0.f,10.24f-x*x));
        bool slit=(std::abs(x+13)<.7f||std::abs(x-13)<.7f)&&z>4&&z<7;
        if(arch||slit||(z>8.4f&&int((x+21)/2.6f)%2==0)) continue;
        float n=noise(int(x*4),int(z*4),seed); bool mortar=int(z*3)%4==0;
        Color col=mix({58,69,81},{121,136,143},n); if(mortar) col=col*.65f;
        s.point({x,wallY},z,col,mortar?U'=':(n<.4f?0:U"0O8o@#"[int(n*5.99f)]),{0,-1,.2f});
        if(((x>6&&x<7.4f)||(x<-6&&x>-7.4f))&&z>4&&z<8) s.point({x,wallY-.04f},z,{135,40,48},U'|');
    }
    for(float x:{-21.f,21.f}) for(float y=-22;y<=-10;y+=.4f/c.scale()) for(float z=0;z<11;z+=.45f/c.scale()) {
        if(z>9.5f&&int(y+22)%4<2) continue;
        float n=noise(int(y*5),int(z*3),seed); s.point({x,y},z,mix({45,61,76},{100,120,128},n),U"08#o="[int(n*4.99f)],{1,0,.3f});
    }
    for(Vec2 p:{Vec2{-4,-9.7f},Vec2{4,-9.7f}}) {
        s.lights.push_back({p,4,{255,138,47},12,1.6f+.15f*std::sin(time*11+p.x)});
        s.point(p,4,{255,214,122},U'*',{0,0,1},1); s.point(p,3.4f,{158,97,52},U'|');
    }
    }
    for(const auto& landmark:landmarks) {
        for(int i=-2;i<=2;++i) s.point(landmark.pos+Vec2{float(i),0},.2f,{179,162,115},U'=');
        for(int z=0;z<5;++z) s.point(landmark.pos, float(z),{197,176,127},z==4?U'*':U'|');
    }
    for(const Tree& t:trees) {
        Vec2 base=c.project(t.pos); float ht=t.height*c.scale()*1.5f,rx=t.radius*c.scale()*2.2f;
        if(base.x+rx<0||base.x-rx>c.width||base.y<0||base.y-ht>c.height) continue;
        for(int yy=int(base.y-ht);yy<=int(base.y);++yy) for(int xx=int(base.x-rx);xx<=int(base.x+rx);++xx) {
            float v=(base.y-yy)/ht,u=(xx-base.x)/rx; bool leaf=false; float n=noise(xx-int(base.x),yy-int(base.y),uint32_t(int(t.pos.x*13))+seed);
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
void World::reflections(Scene& s,float time) const {
    // Reflect the completed scene around the projected shoreline, then distort.
    const auto source=s.pixels; const auto& c=s.camera;
    std::vector<float> banks(size_t(c.width*c.height),0);
    bool generated=std::abs(c.center.x)>120||std::abs(c.center.y)>120;
    if(generated) for(int x=0;x<c.width;++x) {
        float bank=c.unproject({float(x),-60}).y;
        for(int y=-60;y<c.height;++y) {
            Vec2 point=c.unproject({float(x),float(y)});
            if(!water(point)) bank=point.y;
            if(y>=0) banks[size_t(y*c.width+x)]=bank;
        }
    }
    for(int y=0;y<c.height;++y) for(int x=0;x<c.width;++x) {
        Vec2 target=c.unproject({float(x),float(y)}); if(!water(target)) continue;
        float shoreline=shore(target.x);
        if(generated&&std::max(std::abs(target.x),std::abs(target.y))>=180) shoreline=banks[size_t(y*c.width+x)];
        float shoreY=c.project({target.x,shoreline}).y,deep=y-shoreY;
        int sy=int(2*shoreY-y),sx=x+int(std::sin(y*.7f+time*2.3f)*1.5f+std::sin(y*.19f-time)*2);
        if(sx<0||sx>=c.width||sy<0||sy>=c.height) continue;
        const auto& p=source[size_t(sy*c.width+sx)];
        float fade=.36f*std::exp(-deep*.045f);
        if((p.height>.4f||deep<3)&&p.shape!=U' ') {
            auto reflected=p; reflected.color=mix(p.color,{30,51,74},.28f)*fade; reflected.world=target; reflected.height=0; reflected.emission=0;
            if(int(deep+time*4)%7==0) reflected.shape=U'~';
            s.pixels[size_t(y*c.width+x)]=reflected;
        }
        float ring=std::fmod(distance(target,{0,37})-time*2+1000,5.f);
        if(ring<.12f) s.pixels[size_t(y*c.width+x)]={Color{28,43,55},U'-',target.y,target,0,{0,0,1},0};
    }
}
void drawCharacter(Scene& s,Vec2 p,Color color,float time,bool moving,int style) {
    static const char* sprites[5][5]={
        {"  [O]  ","  |#| /"," [###]|","  |#|  ","  / \\ "},
        {"   ^   ","  (o)  "," /#|\\ ","  ###  ","  / \\ "},
        {"   ,   ","  (o) )","  /#==)","   #  )","  / \\ "},
        {"   +   ","  (o)  ","  /+\\ ","  |#|  ","  / \\ "},
        {"  .^.  ","  (:)  "," </#\\>","   #   ","  / \\ "}};
    const auto& sprite=sprites[clamp(style,0,4)];
    Vec2 base=s.camera.project(p); float stride=moving?std::sin(time*13):0,scale=s.camera.scale();
    for(int y=0;y<int(std::ceil(5*scale));++y) for(int x=0;x<int(std::ceil(7*scale));++x) {
        int sy=clamp(int(y/scale),0,4),sx=clamp(int(x/scale),0,6);
        // Sprite rows are padded by their terminator; absent cells stay transparent.
        size_t length=std::char_traits<char>::length(sprite[sy]);
        if(size_t(sx)>=length||sprite[sy][sx]==' ') continue;
        int xx=int(base.x)+x-int(3*scale)+(sy==4?int(stride*scale):0);
        Color col=sy==1?Color{179,164,137}:color;
        s.put(xx,int(base.y)-int(5*scale)+y,p,float(5-sy)*.7f,col,char32_t(sprite[sy][sx]),p.y+.2f,{0,-1,.7f});
    }
    s.put(int(base.x+4*scale),int(base.y-3*scale),p,2,{199,160,77},U'|',p.y+.3f);
}
void drawSkeleton(Scene& s,Vec2 p,Color color,float time) {
    const char* rows[]={"  (o)  ","  /|\\ "," -{#}- ","   |   ","  / \\ "};
    Vec2 base=s.camera.project(p); float scale=s.camera.scale();
    for(int y=0;y<int(std::ceil(5*scale));++y) for(int x=0;x<int(std::ceil(7*scale));++x) {
        int sy=clamp(int(y/scale),0,4),sx=clamp(int(x/scale),0,6);
        if(size_t(sx)>=std::char_traits<char>::length(rows[sy])||rows[sy][sx]==' ') continue;
        int sway=sy==4?int(std::sin(time*9)*scale):0;
        s.put(int(base.x)+x-int(3*scale)+sway,int(base.y)-int(5*scale)+y,p,float(5-sy),color,char32_t(rows[sy][sx]),p.y+.2f,{0,-1,.8f});
    }
}
}
