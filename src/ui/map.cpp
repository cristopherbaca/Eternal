#include "game.hpp"
#include "ui/hud.hpp"
namespace er {
void Game::drawMap(Framebuffer& f,int x,int y,int width,int height,Vec2 center,float step,bool full) const {
    for(int row=0;row<height;++row) for(int col=0;col<width;++col) {
        Vec2 p=center+Vec2{(col-width/2)*step,(row-height/2)*step*2};
        char32_t glyph=U' '; Color color{48,60,74};
        if(atlas.seen(p)||(full&&hero.testing)) {
            glyph=world.water(p)?U'~':world.road(p)?U':':world.biome(p)==1?U'^':world.biome(p)==2?U'.':U',';
            color=world.water(p)?Color{83,156,200}:world.road(p)?Color{193,166,111}:Color{80,125,104};
        }
        f.put(x+col,y+row,glyph,color);
    }
    auto mark=[&](Vec2 p,char32_t glyph,Color color) {
        int col=int(std::round((p.x-center.x)/step))+width/2,row=int(std::round((p.y-center.y)/(step*2)))+height/2;
        if(col>=0&&row>=0&&col<width&&row<height) f.put(x+col,y+row,glyph,color);
    };
    if(full&&hero.testing) {
        auto a=Atlas::tile(center-Vec2{width*step/2,height*step}),b=Atlas::tile(center+Vec2{width*step/2,height*step});
        for(int yy=a.second;yy<=b.second;++yy) for(int xx=a.first;xx<=b.first;++xx) { auto p=atlas.place({xx,yy}); mark(p.pos,atlas.types[size_t(p.type)].glyph,{200,152,91}); }
    }
    for(auto id:atlas.known) { auto p=atlas.place(id); mark(p.pos,atlas.types[size_t(p.type)].glyph,{250,214,128}); }
    if(!full) for(const auto& e:mobs) if(e.alive&&distance(e.pos,hero.pos)<38) mark(e.pos,U'!',{244,107,103});
    if(mapSelected) mark(atlas.place(*mapSelected).pos,U'X',{140,232,241});
    mark(hero.pos,U'@',{246,248,255});
}
void Game::minimap(Framebuffer& f) const {
    if(f.width<85||f.height<30) return; // Keep the existing HUD readable in small terminals.
    int w=25,h=11,x=f.width-w-1,y=3;
    box(f,x,y,w,h,{95,140,155}); f.text(x+2,y," N / MINIMAP ",{175,215,220});
    drawMap(f,x+1,y+1,w-2,h-3,hero.pos,5,false);
    f.text(x+1,y+h-2,"@ you  ! foe  M map",muted);
    if(!atlas.nearby.empty()) {
        const Place* closest=&atlas.nearby.front();
        for(const auto& p:atlas.nearby) if(distance(p.pos,hero.pos)<distance(closest->pos,hero.pos)) closest=&p;
        int d=int(distance(closest->pos,hero.pos));
        std::string label=closest->name+" "+std::to_string(d)+"m";
        f.text(x,y+h,label.substr(0,size_t(w)),{220,196,145});
    }
}
}
