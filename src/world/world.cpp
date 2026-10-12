#include "world/world.hpp"
#include "worldgen/atlas.hpp"
namespace er {
namespace {
float smooth(float a,float b,float v) { float t=clamp((v-a)/(b-a),0.f,1.f); return t*t*(3-2*t); }
struct Palette { Color low,high; float grass,trees,rocks; char32_t plant,particle; };
// IDs preserve existing climate-dependent terrain consumers; deep forest adds ID 7.
constexpr Palette palettes[]={
    {{51,86,48},{125,157,76},.83f,.45f,.025f,U'*',U'.'},
    {{67,94,109},{148,181,185},.48f,.95f,.08f,U'+',U'*'},
    {{94,85,76},{176,158,128},.25f,.12f,.27f,U'v',U'.'},
    {{39,77,67},{91,143,101},.80f,.28f,.02f,U';',U'*'},
    {{141,105,58},{220,184,109},.03f,.015f,.10f,U'Y',U':'},
    {{124,149,168},{232,239,237},.03f,.06f,.22f,U'+',U'*'},
    {{132,129,93},{212,198,139},.18f,.06f,.05f,U',',U'.'},
    {{34,69,45},{86,131,70},.67f,1.4f,.06f,U'*',U'.'}
};
}
float World::rawHeight(Vec2 p) const {
    Vec2 warped=p+Vec2{terrainNoise(p,421,310)-.5f,terrainNoise(p,727,310)-.5f}*130;
    float continent=terrainNoise(warped,1103,780);
    float hills=0,amp=1,total=0;
    for(int i=0;i<4;++i) { hills+=terrainNoise(warped,1501+uint32_t(i)*71,150.f/float(1<<i))*amp; total+=amp; amp*=.5f; }
    hills=hills/total-.5f;
    float ridge=1-std::abs(2*terrainNoise(warped,2003,220)-1);
    float mountain=clamp((terrainNoise(warped,2111,540)-.46f)*3,0.f,1.f);
    float h=(continent-.43f)*42+hills*19+std::pow(ridge,5.f)*mountain*46;
    // Broad shelves with narrow but continuous escarpments; no chunk-local random state.
    float shelf=terrainNoise(warped,2909,170);
    float t=clamp((shelf-.48f)/.10f,0.f,1.f); h+=12*t*t*(3-2*t);
    // Keep the established ruins, boss arena and landmark arrival points accessible.
    float safe=clamp((distance(p,{0,-5})-38)/35,0.f,1.f);
    for(const auto& l:landmarks) safe=std::min(safe,clamp((distance(p,l.pos)-9)/18,0.f,1.f));
    return h*safe+6*(1-safe);
}
World::HeightSample World::rawSample(Vec2 p) const {
    float h=rawHeight(p);
    float moisture=terrainNoise(p,4001,config.biomeScale*.85f);
    float temperature=terrainNoise(p,5003,config.biomeScale*1.25f)-std::max(0.f,h-14)*.009f;
    // Smooth climate influences retain recognizable legacy regions without square borders.
    float frost=1-smooth(25,140,distance(p,landmarks[1].pos));
    float ember=1-smooth(20,125,distance(p,landmarks[2].pos));
    float reeds=1-smooth(25,130,distance(p,landmarks[3].pos));
    temperature=temperature*(1-frost)+.12f*frost; moisture=moisture*(1-frost)+.62f*frost;
    temperature=temperature*(1-ember)+.72f*ember; moisture=moisture*(1-ember)+.32f*ember;
    moisture=moisture*(1-reeds)+.95f*reeds;
    float home=1-smooth(30,90,distance(p,{0,12}));
    moisture=moisture*(1-home)+.36f*home; temperature=temperature*(1-home)+.56f*home;
    float river=terrainNoise(p+Vec2{terrainNoise(p,6029,250)*90,0},6007,310)-.5f;
    return {h,moisture,temperature,river};
}
World::Climate World::climate(const Terrain& s) const {
    Climate c; float left=1;
    auto take=[&](int id,float amount) { c.weights[size_t(id)]=left*clamp(amount,0.f,1.f); left-=c.weights[size_t(id)]; };
    take(6,1-smooth(-1,4,s.height));
    take(5,std::max(smooth(38,49,s.height),smooth(20,34,s.height)*(1-smooth(.18f,.45f,s.temperature))));
    take(2,smooth(15,28,s.height)*.90f);
    take(3,smooth(.65f,.86f,s.moisture)*(1-smooth(8,18,s.height)));
    take(4,smooth(.45f,.70f,s.temperature)*(1-smooth(.24f,.48f,s.moisture)));
    take(1,1-smooth(.24f,.45f,s.temperature));
    take(7,smooth(.42f,.72f,s.moisture));
    c.weights[0]=left;
    for(int i=1;i<8;++i) if(c.weights[size_t(i)]>c.weights[size_t(c.dominant)]) c.dominant=i;
    return c;
}
const char* World::biomeName(int id) {
    const char* names[]={"Roadlands","Frostwood","Ember Hills","Reedlands","Sunscar Dunes","Highspire Snowfields","Saltwind Coast","Deepwood"};
    return names[clamp(id,0,7)];
}
void World::buildHeight(ChunkKey key) {
    auto& grid=heights[key];
    for(int y=0;y<terrainSide;++y) for(int x=0;x<terrainSide;++x)
        grid[size_t(y*terrainSide+x)]=rawSample({float(key.first*chunkSize+x*terrainStep),float(key.second*chunkSize+y*terrainStep)});
}
World::Terrain World::terrain(Vec2 p) const {
    if(interior) return {};
    auto key=chunkAt(p);
    int cx=key.first-centerChunk.first+chunkRadius,cy=key.second-centerChunk.second+chunkRadius;
    const auto* grid=(cx>=0&&cx<7&&cy>=0&&cy<7)?&heightLookup[size_t(cy*7+cx)]:nullptr;
    int x=int(std::floor(p.x/terrainStep)),y=int(std::floor(p.y/terrainStep));
    float u=p.x/terrainStep-x,v=p.y/terrainStep-y;
    HeightSample a,b,c,d;
    if(grid) {
        int ix=x-key.first*(chunkSize/terrainStep),iy=y-key.second*(chunkSize/terrainStep);
        const auto& g=*grid; size_t i=size_t(iy*terrainSide+ix);
        a=g[i]; b=g[i+1]; c=g[i+terrainSide]; d=g[i+terrainSide+1];
    } else { // Queries outside the working set use the identical global lattice.
        a=rawSample({float(x*terrainStep),float(y*terrainStep)}); b=rawSample({float((x+1)*terrainStep),float(y*terrainStep)});
        c=rawSample({float(x*terrainStep),float((y+1)*terrainStep)}); d=rawSample({float((x+1)*terrainStep),float((y+1)*terrainStep)});
    }
    auto blend=[&](float HeightSample::*field) { float aa=a.*field,bb=b.*field,cc=c.*field,dd=d.*field; return (aa+(bb-aa)*u)*(1-v)+(cc+(dd-cc)*u)*v; };
    return {blend(&HeightSample::height),{((b.height-a.height)*(1-v)+(d.height-c.height)*v)/terrainStep,((c.height-a.height)*(1-u)+(d.height-b.height)*u)/terrainStep},blend(&HeightSample::moisture),blend(&HeightSample::temperature),blend(&HeightSample::river)};
}

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
World::World(uint64_t s,GenerationConfig settings):seed(s),terrainSeed(uint32_t(s)^hash(uint32_t(s>>32))^settings.seedSalt),config(settings) { focus({0,12}); }
float World::terrainNoise(Vec2 p,uint32_t salt,float scale) const {
    float fx=p.x/scale,fy=p.y/scale; int x=int(std::floor(fx)),y=int(std::floor(fy));
    float u=fx-x,v=fy-y; u=u*u*(3-2*u); v=v*v*(3-2*v);
    float a=noise(x,y,terrainSeed+salt),b=noise(x+1,y,terrainSeed+salt),c=noise(x,y+1,terrainSeed+salt),d=noise(x+1,y+1,terrainSeed+salt);
    return (a+(b-a)*u)*(1-v)+(c+(d-c)*u)*v;
}
int World::biome(Vec2 p) const { return climate(terrain(p)).dominant; }

bool World::treeAt(int x,int y,Tree& tree) const {
    if(config.treeDensity<=0||noise(x,y,terrainSeed+8)>config.treeDensity) return false;
    Vec2 p{float(x*8)+noise(x,y,terrainSeed)*6,float(y*8)+noise(y,x,terrainSeed+1)*6};
    for(const auto& landmark:landmarks) if(distance(p,landmark.pos)<7) return false;
    if(placeClearing(p,terrainSeed)||water(p)||road(p)||(std::abs(p.x)<23&&p.y>-24&&p.y<4)||distance(p,{0,12})<9||distance(p,{30,-25})<16) return false;
    auto weights=climate(terrain(p)); float density=0;
    for(int i=0;i<8;++i) density+=weights.weights[size_t(i)]*palettes[i].trees;
    if(noise(x,y,terrainSeed+67)>density||length(terrain(p).slope)>.85f) return false;
    tree={p,6+noise(x,y,terrainSeed+3)*7,2+noise(y,x,terrainSeed+9)*2,weights.dominant==1||weights.dominant==5,weights.dominant};
    if(tree.species==3) { tree.height*=.75f; tree.radius*=1.15f; }
    return true;
}
void World::streamChunks(Vec2 pos) {
    auto key=chunkAt(pos); if(key==centerChunk) return;
    centerChunk=key;
    for(auto it=heights.begin();it!=heights.end();) {
        if(std::abs(it->first.first-key.first)>chunkRadius||std::abs(it->first.second-key.second)>chunkRadius) it=heights.erase(it); else ++it;
    }
    for(int y=key.second-chunkRadius;y<=key.second+chunkRadius;++y) for(int x=key.first-chunkRadius;x<=key.first+chunkRadius;++x)
        if(!heights.count({x,y})) buildHeight({x,y});
    for(int y=0;y<7;++y) for(int x=0;x<7;++x) heightLookup[size_t(y*7+x)]=heights.at({key.first+x-chunkRadius,key.second+y-chunkRadius});
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
    if(interior) return;
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
    if(connectingRoad(p,terrainSeed)) return true;
    if(std::abs(p.x-p.y*.55f+8)<2.8f||std::abs(p.y+3+p.x*.14f)<1.6f) return true;
    // Continuous, gently winding routes repeat as corridors, not terrain tiles.
    float lane=p.x+12*std::sin(p.y*.009f);
    return std::abs(lane-std::round(lane/256)*256)<2.2f;
}
bool World::water(Vec2 p) const {
    if(interior) return false;
    if(placeClearing(p,terrainSeed)) return false;
    if(road(p)) return false;
    // Keep the reference lake; fade in height-derived frontier basins and river beds.
    float edge=std::max(std::abs(p.x),std::abs(p.y));
    if(edge<140) return p.y>shore(p.x)&&p.y<85+5*std::sin(p.x*.07f)&&std::abs(p.x)<72;
    auto land=terrain(p);
    float extent=smooth(140,230,edge);
    float level=-2+(config.lakeThreshold-.3f)*16;
    return land.height<level-(1-extent)*50||(extent>.5f&&land.height<20&&std::abs(land.river)<.010f+land.moisture*.008f);
}
const char* World::region(Vec2 p) const {
    if(distance(p,{30,-30})<25) return "Dragon Hollow";
    for(const auto& landmark:landmarks) if(distance(p,landmark.pos)<40) return landmark.name;
    return biomeName(biome(p));
}
bool World::blocked(Vec2 p) const {
    if(interior) return interior->blocked(p);
    // Float-based entities have a documented precision envelope, not a 500-unit map wall.
    if(!std::isfinite(p.x)||!std::isfinite(p.y)||std::abs(p.x)>1000000||std::abs(p.y)>1000000) return true;
    if(!road(p)&&!placeClearing(p,terrainSeed)&&length(terrain(p).slope)>1.15f) return true;
    if(std::abs(p.y+10)<.7f&&std::abs(p.x)<21&&std::abs(p.x)>3.3f) return true;
    if(std::abs(p.y+22)<.7f&&std::abs(p.x)<21) return true;
    int x=int(std::floor(p.x/8)),y=int(std::floor(p.y/8));
    for(int yy=y-1;yy<=y+1;++yy) for(int xx=x-1;xx<=x+1;++xx) { Tree t; if(treeAt(xx,yy,t)&&distance(p,t.pos)<.8f) return true; }
    return false;
}
Vec2 World::move(Vec2 p,Vec2 d) const {
    if(!interior&&length(d)>0) {
        float uphill=dot(terrain(p).slope,normalized(d));
        d=d*(1/(1+std::max(0.f,uphill)*1.8f));
    }
    int steps=std::max(1,int(std::ceil(length(d)/.5f)));
    if(steps>1) { Vec2 step=d/float(steps); for(int i=0;i<steps;++i) {
        Vec2 next=p+step;
        if(!blocked(next)) p=next;
        else { if(!blocked({next.x,p.y})) p.x=next.x; if(!blocked({p.x,next.y})) p.y=next.y; }
    } return p; }
    Vec2 next=p+d; if(!blocked(next)) return next;
    if(!blocked({next.x,p.y})) p.x=next.x; if(!blocked({p.x,next.y})) p.y=next.y; return p;
}
void World::render(Scene& s,float time) const {
    if(interior) { interior->render(s); return; }
    const Camera& c=s.camera;
    surface.resize(size_t(c.width*c.height));
    for(int y=0;y<c.height;++y) for(int x=0;x<c.width;++x) {
        Vec2 p=c.unproject({float(x),float(y)}); surface[size_t(y*c.width+x)]=p; int gx=int(std::floor(p.x*3)),gy=int(std::floor(p.y*3)); float n=noise(gx,gy,terrainSeed);
        auto land=terrain(p); float slope=length(land.slope);
        float inv=1/std::sqrt(1+dot(land.slope,land.slope));
        Normal normal{-land.slope.x*inv,-land.slope.y*inv,inv};
        float relief=clamp(.87f+land.height*.012f,.60f,1.25f);
        float shadow=shadows[size_t(clamp(int(p.y-grassCenter.y)+70,0,140)*181+clamp(int(p.x-grassCenter.x)+90,0,180))];
        if(water(p)) {
            float wave=.5f+.5f*std::sin(p.x*.42f+p.y*.28f-time*1.4f);
            bool river=land.height>-2;
            Color blue=mix({28,64,88},{73,139,153},wave*.65f+(river?.2f:0));
            s.put(x,y,p,0,blue,wave>.86f?U'~':n>.55f?U'-':U'.',p.y-10); continue;
        }
        if(road(p)) {
            char32_t glyph=U"o0QOa.:"[int(n*6.99f)]; Color color=mix({80,81,88},{145,139,129},n);
            if((gy%4+4)%4==0) { glyph=U'-'; color=color*.65f; }
            s.put(x,y,p,0,color*shadow,glyph,p.y-10); continue;
        }
        if(std::abs(p.x)<20&&p.y<-10&&p.y>-22) { s.put(x,y,p,0,{45,55,65},n>.65f?U'=':U'.',p.y-10); continue; }
        float wind=std::sin(p.x*.23f+p.y*.18f+time*1.6f)+.4f*std::sin(p.y*.8f-time*2);
        Vec2 bend=grassBend(p); wind+=bend.x*2;
        char32_t blade=wind>.45f?U'/':wind<-.45f?U'\\':U'|'; if(n<.2f) blade=U','; else if(n>.97f) blade=U'.';
        if(length(bend)>.55f) blade=std::abs(bend.x)>.3f?U'-':U',';
        Color color=n>.97f?Color{166,192,169}:mix({30,50,43},{69,97,70},n);
        auto ecology=climate(land); int climate=ecology.dominant;
        float choice=noise(gx,gy,terrainSeed+1231),sum=0;
        for(int i=0;i<8;++i) { sum+=ecology.weights[size_t(i)]; if(choice<=sum) { climate=i; break; } }
        float red=0,green=0,blue=0,grass=0;
        for(int i=0;i<8;++i) { float w=ecology.weights[size_t(i)]; Color col=mix(palettes[i].low,palettes[i].high,n);
            red+=col.r*w; green+=col.g*w; blue+=col.b*w; grass+=palettes[i].grass*w;
        }
        color=rgb(red,green,blue);
        if(n>grass) blade=U".,:`"[int(n*3.99f)];
        // Ordered coordinate noise mixes glyphs at ecotones without animated boundary shimmer.
        if(climate==4) { float dune=.5f+.5f*std::sin(p.x*.12f+p.y*.035f+land.height*.3f); color=color*(.78f+dune*.28f); blade=dune>.8f?U'~':U'.'; }
        if(climate==5) blade=n>.92f?U'*':n>.6f?U':':U'.';
        if(climate==3&&n<grass) blade=U";|,|"[int(n*3.99f)];
        if(n>.992f) { blade=palettes[climate].plant; color=mix(color,climate==0?Color{234,196,107}:Color{177,187,157},.7f); }
        float rock=clamp((slope-.32f)/.75f,0.f,1.f);
        color=mix(color,mix({75,82,94},{169,170,159},n),rock);
        if(rock>.4f&&n<rock) blade=slope>1.15f?U"|/\\#"[int(n*3.99f)]:(land.slope.x*land.slope.y>0?U'/':U'\\');
        color=color*(1-clamp(length(bend)*.28f,0.f,.55f))*shadow*relief;
        s.put(x,y,p,0,color,blade,p.y-10,normal);
    }
    if(std::abs(c.center.x)<120&&std::abs(c.center.y)<120) {
    // A shallow stone volume: back facade, side piers, front arches and crenellations.
    for(float wallY:{-22.f,-10.f}) for(float x=-21;x<=21;x+=.35f/c.scale()) for(float z=0;z<10;z+=.48f/c.scale()) {
        float ax=std::abs(x); bool arch=ax<3.2f&&z<4.0f+std::sqrt(std::max(0.f,10.24f-x*x));
        bool slit=(std::abs(x+13)<.7f||std::abs(x-13)<.7f)&&z>4&&z<7;
        if(arch||slit||(z>8.4f&&int((x+21)/2.6f)%2==0)) continue;
        float n=noise(int(x*4),int(z*4),terrainSeed); bool mortar=int(z*3)%4==0;
        Color col=mix({58,69,81},{121,136,143},n); if(mortar) col=col*.65f;
        s.point({x,wallY},z,col,mortar?U'=':(n<.4f?0:U"0O8o@#"[int(n*5.99f)]),{0,-1,.2f});
        if(((x>6&&x<7.4f)||(x<-6&&x>-7.4f))&&z>4&&z<8) s.point({x,wallY-.04f},z,{135,40,48},U'|');
    }
    for(float x:{-21.f,21.f}) for(float y=-22;y<=-10;y+=.4f/c.scale()) for(float z=0;z<11;z+=.45f/c.scale()) {
        if(z>9.5f&&int(y+22)%4<2) continue;
        float n=noise(int(y*5),int(z*3),terrainSeed); s.point({x,y},z,mix({45,61,76},{100,120,128},n),U"08#o="[int(n*4.99f)],{1,0,.3f});
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
    // World-anchored boulders and understory, culled to the viewport. No persistent entities.
    Vec2 lo=c.unproject({-12,float(c.height+20)}),hi=c.unproject({float(c.width+12),-20});
    int minX=int(std::floor(std::min(lo.x,hi.x)/8)),maxX=int(std::ceil(std::max(lo.x,hi.x)/8));
    int minY=int(std::floor(std::min(lo.y,hi.y)/8)),maxY=int(std::ceil(std::max(lo.y,hi.y)/8));
    for(int yy=minY;yy<=maxY;++yy) for(int xx=minX;xx<=maxX;++xx) {
        float n=noise(xx,yy,terrainSeed+8191); Vec2 p{xx*8+noise(xx,yy,terrainSeed+89)*6,yy*8+noise(xx,yy,terrainSeed+97)*6};
        auto ec=climate(terrain(p)); float rock=0; for(int b=0;b<8;++b) rock+=ec.weights[size_t(b)]*palettes[b].rocks;
        if(road(p)||water(p)||placeClearing(p,terrainSeed)) continue;
        if(n<rock) {
            float size=.6f+noise(xx,yy,terrainSeed+79)*1.1f;
            Color stone=ec.dominant==4?Color{177,125,78}:ec.dominant==5?Color{198,212,219}:Color{129,142,148};
            s.point(p+Vec2{-size*.6f,0},size*.4f,stone*.65f,U'/',{-.7f,0,.5f});
            s.point(p,size,stone,U'^',{-.3f,-.5f,.8f});
            s.point(p+Vec2{size*.6f,0},size*.4f,stone*.85f,U'\\',{.7f,0,.5f});
        } else if(n>.94f) {
            int b=ec.dominant; Color plant=b==4?Color{121,151,82}:b==0?Color{224,168,115}:palettes[b].high;
            s.point(p,.7f,plant,palettes[b].plant);
            if(b==4) { s.point(p,1.5f,plant,U'Y'); s.point(p,.2f,plant,U'|'); }
        }
    }
    // At most 48 ambient motes, anchored to world cells; no accumulating particle list.
    int ax=int(std::floor(c.center.x/20)),ay=int(std::floor(c.center.y/20));
    for(int y=-3;y<3;++y) for(int x=-4;x<4;++x) {
        float n=noise(ax+x,ay+y,terrainSeed+9001);
        Vec2 p{(ax+x)*20+n*18,(ay+y)*20+noise(ay+y,ax+x,terrainSeed+9007)*18};
        int b=biome(p); float phase=time*(b==1||b==5?.6f:.24f)+n*6;
        p.x+=std::sin(phase)*2; p.y+=std::cos(phase*.7f);
        float height=b==1||b==5?4-std::fmod(time*.7f+n*4,4.f):1.5f+std::sin(phase);
        Color color=b==3?Color{175,213,100}:b==4?Color{174,136,85}:b==1||b==5?Color{177,206,224}:Color{183,173,113};
        float intensity=.45f+.35f*std::sin(phase*2);
        s.point(p,height,color*intensity,palettes[b].particle,{},b==3?.5f:0);
    }
    for(const Tree& t:trees) {
        Vec2 base=c.project(t.pos); float ht=t.height*c.scale()*1.5f,rx=t.radius*c.scale()*2.2f;
        if(base.x+rx<0||base.x-rx>c.width||base.y<0||base.y-ht>c.height) continue;
        for(int yy=int(base.y-ht);yy<=int(base.y);++yy) for(int xx=int(base.x-rx);xx<=int(base.x+rx);++xx) {
            float v=(base.y-yy)/ht,u=(xx-base.x)/rx; bool leaf=false; float n=noise(xx-int(base.x),yy-int(base.y),uint32_t(int(t.pos.x*13))+terrainSeed);
            if(t.conifer) { float tier=std::fmod(v*4,1.f); leaf=v>.16f&&std::abs(u)<(1-v)*(.65f+.5f*tier); }
            else { float q=(v-.61f)/.37f; leaf=u*u+q*q<1+n*.2f; }
            if(leaf&&n>.08f) {
                Color col=t.conifer?mix({48,81,95},{130,170,170},n):mix({32,62,40},{100,147,76},n);
                if(t.species==3) col=mix(col,{89,131,104},.3f);
                if(t.species==5||(t.species==1&&v>.68f&&n>.5f)) col=mix(col,{198,215,219},.65f);
                char32_t glyph=t.conifer?U'^':t.species==3?U';':U"oaOQd0"[int(n*5.99f)];
                s.put(xx,yy,t.pos,v*t.height,col,glyph,t.pos.y+v*.1f,{u,-.3f,.8f});
            } else if(std::abs(u)<.13f&&v<.65f) s.put(xx,yy,t.pos,v*t.height,{73,76,67},U'|',t.pos.y,{0,-1,.3f});
        }
    }
}
void World::reflections(Scene& s,float time) const {
    if(interior) return;
    // Reflect the completed scene around the projected shoreline, then distort.
    const auto source=s.pixels; const auto& c=s.camera;
    std::vector<float> banks(size_t(c.width*c.height),0);
    bool generated=std::abs(c.center.x)>120||std::abs(c.center.y)>120;
    if(generated) for(int x=0;x<c.width;++x) {
        float bank=c.unproject({float(x),-60}).y;
        for(int y=-60;y<c.height;++y) {
            Vec2 point=y>=0?surface[size_t(y*c.width+x)]:c.unproject({float(x),float(y)});
            if(!water(point)) bank=point.y;
            if(y>=0) banks[size_t(y*c.width+x)]=bank;
        }
    }
    for(int y=0;y<c.height;++y) for(int x=0;x<c.width;++x) {
        Vec2 target=surface[size_t(y*c.width+x)]; if(!water(target)) continue;
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
