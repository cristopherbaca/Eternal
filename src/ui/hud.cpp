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
    Vec2 p=c.project(e.pos); int y=int(p.y)-(e.kind==Kind::Player?(c.height<80?9:12):int((e.kind==Kind::Boss?16:7)*c.scale())); int x=int(p.x)-int(e.name.size()/2);
    if(y<2||y>=f.height-4) return;
    f.text(x,y,e.name,e.kind==Kind::Player?Color{213,239,246}:targeted?Color{255,164,138}:white);
    if(e.hp<e.maxHp||targeted) bar(f,int(p.x)-3,y+1,7,e.hp,e.maxHp,{200,66,62});
}
void floatingLabels(Framebuffer& f,const Camera& c,const std::vector<FloatingText>& texts) {
    for(const auto& t:texts) { Vec2 p=c.project(t.pos); f.text(int(p.x)-int(t.text.size()/2),int(p.y)-7-int((t.maxLife-t.life)*4),t.text,t.color*clamp(t.life/.5f,0.f,1.f)); }
}
void playerAvatar(Framebuffer& f,const Camera& camera,const Entity& hero,float time) {
    // Compact armor: each symbol is drawn once, never enlarged into repeated blocks.
    const char* rows[]={R"(     ^     )",R"(    /A\    )",R"(   .[=].   )",R"(  /|:::|\  )",R"( / |:::| | )",R"(/  |===| | )",R"(   |:::| | )",R"(   /| |\   )",R"(   || ||   )",R"(  /_J L_\  )"};
    const bool walking=length(hero.velocity)>.05f;
    const int phase=int(time*(walking?9.f:2.f))%4;
    const char* legs[4][3]={
        {R"(   /| |\   )",R"(  /|   |   )",R"( /_J   L_  )"},
        {R"(   /| |\   )",R"(   || ||   )",R"(   L_ /_J  )"},
        {R"(   /| |\   )",R"(   |   |\  )",R"(  _J   L_\ )"},
        {R"(   /| |\   )",R"(   || ||   )",R"(  /_J L_   )"}
    };
    Vec2 base=camera.project(hero.pos);
    // Use a shorter version on low-density terminal grids.
    const bool compact=camera.height<80;
    const int height=compact?7:10;
    for(int y=0;y<height;++y) {
        int sy=compact?(y<4?y:y+3):y;
        for(int x=0;x<11;++x) {
            char32_t glyph=(walking&&sy>=7?legs[phase][sy-7]:rows[sy])[x];
            if(!walking&&sy==2&&x==5) glyph=phase==3?U'-':U'=';
            if(glyph==U' ') continue;
            if(x<3&&hero.equipment[0]<0) continue;
            if(sy>=3&&sy<=6&&glyph==U':'&&hero.equipment[1]<0) glyph=U'.';
            Color color=x<=2?Color{131,191,207}:sy<3?Color{233,244,246}:Color{178,213,224};
            if(sy>=3&&sy<=6&&hero.equipment[1]<0) color={193,185,160};
            // Arms swing opposite the feet; shoulders breathe while standing.
            int swing=walking&&sy>=3&&sy<=5?(x<3?(phase<2?1:0):x>7?(phase>=2?1:0):0):0;
            int bob=sy<7?(walking?(phase%2):phase==1?1:0):0;
            f.put(int(base.x)+x-5,int(base.y)-height+y-bob+swing,glyph,color);
        }
    }

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
