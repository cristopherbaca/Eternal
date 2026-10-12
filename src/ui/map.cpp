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
    for(auto id:atlas.known) { auto p=atlas.place(id); mark(p.pos,atlas.types[size_t(p.type)].glyph,atlas.visited.count(id)?Color{250,214,128}:Color{135,201,218}); }
    if(!full) for(const auto& e:mobs) if(e.alive&&distance(e.pos,hero.pos)<38) mark(e.pos,U'!',{244,107,103});
    for(const auto& q:quests) if(!q.rewarded) mark(atlas.place(questReady(q)?PlaceId{std::get<0>(q.giver),std::get<1>(q.giver)}:q.target).pos,U'?',{151,220,250});
    if(!full) for(const auto& p:population.nearby) if(!population.away.count(p.id)&&(!population.changes.count(p.id)||!population.changes.at(p.id).recruited)&&distance(p.pos,hero.pos)<45) mark(p.pos,U'n',{135,211,168});
    for(const auto& q:quests) if(q.kind==QuestKind::Escort&&!q.complete) mark(q.escortPos,U'e',{255,222,139});
    for(const auto& q:quests) if(q.kind==QuestKind::Delivery&&!q.complete)
        for(const auto& p:population.nearby) if(p.id==q.contact) mark(p.pos,U'?',{151,220,250});
    if(mapSelected) mark(atlas.place(*mapSelected).pos,U'X',{140,232,241});
    mark(hero.pos,U'@',{246,248,255});
}
void Game::minimap(Framebuffer& f) const {
    if(world.interior) {
        if(f.width<85||f.height<30) return;
        int x=f.width-26,y=3; box(f,x,y,25,11); f.text(x+2,y," INTERIOR ");
        for(int row=0;row<8;++row) for(int col=0;col<23;++col) {
            int xx=col*2,yy=row*3; char glyph=world.interior->at(xx,yy);
            f.put(x+1+col,y+1+row,glyph,{131,153,173});
        }
        f.put(x+1+int(world.interior->entrance.x)/2,y+1+int(world.interior->entrance.y)/3,U'<',{241,211,131});
        f.put(x+1+int(world.interior->chest.x)/2,y+1+int(world.interior->chest.y)/3,world.interior->peaceful?U'b':world.interior->opened?U'o':U'$',{241,211,131});
        f.put(x+1+int(hero.pos.x)/2,y+1+int(hero.pos.y)/3,U'@',white);
        f.text(x+1,y+9,"< exit  $ chest  b bed"); return;
    }
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
        f.text(x,y+h+1,std::to_string(int(worldHour(time)))+":00 / "+weatherName(weatherAt(world.terrainSeed,time)),muted);
    }
}
}
