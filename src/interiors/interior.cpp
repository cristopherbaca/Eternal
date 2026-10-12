#include "interiors/interior.hpp"
namespace er {
Interior::Interior(uint32_t seed,bool quiet):peaceful(quiet) {
    tiles.fill('#');
    auto room=[&](int x,int y,int w,int h) { for(int yy=y;yy<y+h;++yy) for(int xx=x;xx<x+w;++xx) tiles[size_t(yy*width+xx)]='.'; };
    room(2,8,11,11); room(12,11,27,5);
    int offset=int(hash(seed)%4);
    room(18,4+offset,11,17-offset); room(36,7,11,13);
    tiles[13*width+4]='<'; tiles[13*width+43]=peaceful?'b':'$';
    tiles[12*width+7]='t'; tiles[10*width+23]='t'; tiles[16*width+40]='t';
    tiles[13*width+31]=peaceful?'t':'^';
}
void Interior::render(Scene& s) const {
    for(int y=0;y<s.camera.height;++y) for(int x=0;x<s.camera.width;++x) {
        Vec2 p=s.camera.unproject({float(x),float(y)}); char tile=at(int(std::round(p.x)),int(std::round(p.y)));
        Color color=tile=='#'?Color{92,104,119}:Color{42,48,56}; char32_t glyph=tile=='#'?U'#':U'.';
        s.put(x,y,p,0,color,glyph,p.y-10);
    }
    for(int y=0;y<height;++y) for(int x=0;x<width;++x) {
        char tile=at(x,y); if(tile=='#'||tile=='.') continue;
        if(tile=='$'&&opened) tile='o'; if(tile=='^'&&trapSpent) tile='.';
        s.point({float(x),float(y)},.3f,tile=='^'?Color{201,86,75}:Color{232,187,109},char32_t(tile),{},.6f);
    }
    for(Vec2 p:{Vec2{5,9},Vec2{23,9},Vec2{41,9}}) { s.point(p,2,{255,184,91},U'*',{},1); s.lights.push_back({p,2,{255,181,98},12,1}); }
}
}
