#include "renderer/terrain3d.hpp"
namespace er {
void Terrain3D::update(const World& world,Vec2 center) {
    const auto& c=world.config;
    if(source!=&world||seed!=world.terrainSeed||settings.biomeScale!=c.biomeScale||settings.lakeThreshold!=c.lakeThreshold) { chunks.clear(); source=&world; seed=world.terrainSeed; settings=c; ++revision; }
    auto key=World::chunkAt(center);
    constexpr int radius=3;
    for(auto it=chunks.begin();it!=chunks.end();) {
        if(std::abs(it->first.first-key.first)>radius||std::abs(it->first.second-key.second)>radius) { it=chunks.erase(it); ++revision; } else ++it;
    }
    constexpr Color colors[]={{98,139,65},{108,151,166},{146,126,99},{58,112,87},{205,165,92},{211,224,229},{190,181,121},{55,108,63}};
    for(int y=key.second-radius;y<=key.second+radius;++y) for(int x=key.first-radius;x<=key.first+radius;++x) {
        World::ChunkKey chunk{x,y}; if(chunks.count(chunk)) continue;
        auto& mesh=chunks[chunk]; mesh.vertices.reserve(16*16*6);
        auto vertex=[&](int xx,int yy) {
            Vec2 p{float(x*64+xx*4),float(y*64+yy*4)}; auto t=world.terrain(p); auto climate=world.climate(t);
            Vec2 slope{(world.elevation(p+Vec2{1,0})-world.elevation(p-Vec2{1,0}))*.5f,(world.elevation(p+Vec2{0,1})-world.elevation(p-Vec2{0,1}))*.5f};
            float n=1/std::sqrt(1+dot(slope,slope)),r=0,g=0,b=0;
            for(int i=0;i<8;++i) { r+=colors[i].r*climate.weights[size_t(i)]; g+=colors[i].g*climate.weights[size_t(i)]; b+=colors[i].b*climate.weights[size_t(i)]; }
            if(world.water(p)) { r=36;g=94;b=126; } else if(world.road(p)) { r=139;g=129;b=108; }
            return TerrainVertex{{p.x,p.y,t.height,1},{-slope.x*n,-slope.y*n,n,0},{r/255,g/255,b/255,1}};
        };
        std::array<TerrainVertex,17*17> grid;
        for(int yy=0;yy<=16;++yy) for(int xx=0;xx<=16;++xx) grid[size_t(yy*17+xx)]=vertex(xx,yy);
        for(int yy=0;yy<16;++yy) for(int xx=0;xx<16;++xx) { int a=yy*17+xx,b=a+1,c=a+17,d=c+1; for(int i:{a,b,c,b,d,c}) mesh.vertices.push_back(grid[size_t(i)]); }
        ++revision;
    }
}
}
