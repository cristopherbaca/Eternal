#include "ui/hud.hpp"
#include <cstdio>
namespace er {
void box(Framebuffer& f,int x,int y,int w,int h,Color color) {
    for(int yy=y;yy<y+h;++yy) for(int xx=x;xx<x+w;++xx) f.put(xx,yy,U' ');
    for(int xx=x+1;xx<x+w-1;++xx) { f.put(xx,y,U'─',color); f.put(xx,y+h-1,U'─',color); }
    for(int yy=y+1;yy<y+h-1;++yy) { f.put(x,yy,U'│',color); f.put(x+w-1,yy,U'│',color); }
    f.put(x,y,U'┌',color); f.put(x+w-1,y,U'┐',color); f.put(x,y+h-1,U'└',color); f.put(x+w-1,y+h-1,U'┘',color);
}
void bar(Framebuffer& f,int x,int y,int w,float v,float max,Color color) {
    int filled=int(w*clamp(max>0?v/max:0,0.f,1.f)); for(int i=0;i<w;++i) f.put(x+i,y,i<filled?U'█':U'░',i<filled?color:Color{40,44,59});
}
void nameLabel(Framebuffer& f,const Camera& c,const Entity& e,bool targeted) {
    if(!e.alive) return;
    Vec2 p=c.project(e.pos); int y=int(p.y)-int((e.kind==Kind::Boss?16:e.kind==Kind::Player?9:7)*c.scale()); int x=int(p.x)-int(e.name.size()/2);
    if(y<2||y>=f.height-4) return;
    f.text(x,y,e.name,e.kind==Kind::Player?Color{255,223,133}:targeted?Color{255,164,138}:white);
    if(e.hp<e.maxHp||targeted) bar(f,int(p.x)-3,y+1,7,e.hp,e.maxHp,{200,66,62});
}
void floatingLabels(Framebuffer& f,const Camera& c,const std::vector<FloatingText>& texts) {
    for(const auto& t:texts) { Vec2 p=c.project(t.pos); f.text(int(p.x)-int(t.text.size()/2),int(p.y)-7-int((t.maxLife-t.life)*4),t.text,t.color*clamp(t.life/.5f,0.f,1.f)); }
}
void playerAvatar(Framebuffer& f,const Camera& camera,const Entity& hero,float time) {
    // A warm, outlined local-player silhouette remains readable behind canopies.
    const char* rows[]={R"(    ^    )",R"(   /O\   )",R"(  /[#]\ |)",R"( / /#\ \|)",R"(   |#|  |)",R"(   / \   )",R"(  /   \  )"};
    Vec2 base=camera.project(hero.pos); float scale=std::max(.85f,camera.scale());
    struct Mark { int x,y; char32_t glyph; Color color; };
    std::vector<Mark> marks;
    for(int y=0;y<int(std::ceil(7*scale));++y) for(int x=0;x<int(std::ceil(9*scale));++x) {
        int sy=clamp(int(y/scale),0,6),sx=clamp(int(x/scale),0,8);
        char32_t glyph=rows[sy][sx]; if(glyph==U' ') continue;
        if(sy==0&&hero.role==Role::Warrior) glyph=U'@';
        if(sy==0&&hero.role==Role::Cleric) glyph=U'+';
        if(sx==8&&hero.role==Role::Archer) glyph=U')';
        Color color=sy==1?Color{255,249,224}:sx==8?Color{125,232,255}:Color{255,204,103};
        if((sy==2||sy==3)&&sx!=4&&sx!=8) color={247,119,115};
        if(sy>=5) color={242,225,174};
        int stride=sy==6&&length(hero.velocity)>0?int(std::sin(time*12)*scale*.5f):0;
        marks.push_back({int(base.x)+x-int(4*scale)+stride,int(base.y)-int(7*scale)+y,glyph,color});
    }
    for(const auto& mark:marks) for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx) {
        int x=mark.x+dx,y=mark.y+dy;
        if(x>=0&&y>=0&&x<f.width&&y<f.height) { auto c=f.at(x,y); f.put(x,y,c.glyph,c.fg*.2f); }
    }
    for(const auto& mark:marks) f.put(mark.x,mark.y,mark.glyph,mark.color);
    f.text(int(base.x)-2,int(base.y)+1,"- ^ -",{255,204,103});
}
void hud(Framebuffer& f,const Entity& hero,const std::vector<Entity>& party,const Entity* boss,float ms) {
    if(f.width<80||f.height<28) {
        f.text(0,0,hero.name+" HP "+std::to_string(int(hero.hp))+"/"+std::to_string(int(hero.maxHp))+" MP "+std::to_string(int(hero.mp)));
    } else {
        const int w=29; box(f,1,1,w,6); f.text(3,2,hero.name+" - "+roleName(hero.role));
        f.text(3,3,"HP"); bar(f,6,3,10,hero.hp,hero.maxHp,{202,56,54}); f.text(17,3,std::to_string(int(hero.hp))+"/"+std::to_string(int(hero.maxHp)));
        f.text(3,4,"MP"); bar(f,6,4,10,hero.mp,hero.maxMp,{65,113,193}); f.text(17,4,std::to_string(int(hero.mp))+"/"+std::to_string(int(hero.maxMp)));
        f.text(3,5,"LV "+std::to_string(hero.level)+" - "+std::to_string(hero.gold)+"g",muted);
        int y=7;
        for(const auto& e:party) {
            if(y+5>f.height-9) break;
            box(f,1,y,w,5); f.text(3,y+1,e.name+" - "+std::to_string(e.level));
            f.text(3,y+2,"HP"); bar(f,6,y+2,10,e.hp,e.maxHp,{174,73,66}); f.text(17,y+2,std::to_string(int(e.hp))+"/"+std::to_string(int(e.maxHp)));
            f.text(3,y+3,"MP"); bar(f,6,y+3,10,e.mp,e.maxMp,{65,111,168}); f.text(17,y+3,std::to_string(int(e.mp))+"/"+std::to_string(int(e.maxMp))); y+=5;
        } if(!party.empty()) f.text(2,y,"party",white);
    }
    if(boss&&boss->alive) {
        int x=f.width>110?34:2,w=std::min(90,f.width-x-3),phase=boss->hp/boss->maxHp>.66f?1:boss->hp/boss->maxHp>.33f?2:3;
        std::string label="Dragon"+(phase>1?" - phase "+std::to_string(phase):"")+"  "+std::to_string(int(boss->hp))+"/"+std::to_string(int(boss->maxHp));
        f.text(x,1,label); bar(f,x,2,w,boss->hp,boss->maxHp,{193,55,44});
    }
    if(f.width>110) { char label[80]; std::snprintf(label,sizeof(label),"Sent %.0f fps / %.1f ms",1000.f/std::max(ms,.01f),ms); f.text(f.width-31,0,label,muted); }
}
}
