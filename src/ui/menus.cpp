#include "game.hpp"
#include "ui/hud.hpp"
#include <fstream>
namespace er {
bool Game::loadSettings(const std::filesystem::path& path) {
    std::ifstream in(path); int version=0,test=0,allies=0,fps=0,density=0,names=0,hints=0,role=0; float zoom=0;
    if(!(in>>version>>test>>allies>>fps>>density>>zoom>>names>>hints>>role)||version!=1) return false;
    if((test!=0&&test!=1)||(allies!=0&&allies!=1)||(names!=0&&names!=1)||(hints!=0&&hints!=1)||
       fps<20||fps>240||density<0||density>2||!std::isfinite(zoom)||zoom<.55f||zoom>2||role<0||role>4) return false;
    presentationFps=fps; detail=density; camera.zoom=zoom; showNames=names; showHelp=hints;
    data.applyClass(hero,Role(role)); setCompanions(allies); setTesting(test); return true;
}
bool Game::saveSettings(const std::filesystem::path& path) const {
    std::ofstream out(path);
    out<<1<<' '<<hero.testing<<' '<<companions<<' '<<presentationFps<<' '<<detail<<' '
       <<camera.zoom<<' '<<showNames<<' '<<showHelp<<' '<<int(hero.role)<<'\n';
    out.close(); return !out.fail();
}
void Game::awardXP(int amount) {
    hero.xp+=std::max(0,amount);
    while(hero.xp>=100+hero.level*25) {
        hero.xp-=100+hero.level*25; ++hero.level;
        hero.maxHp+=8; hero.power+=2;
        if(hero.alive) hero.hp=hero.maxHp;
        texts.push_back({hero.pos,"LEVEL UP",{255,223,115},3,3});
    }
}
void Game::setTesting(bool enabled) {
    hero.testing=enabled;
    hero.cooldown.fill(0);
    if(enabled) { hero.alive=true; hero.hp=hero.maxHp; hero.mp=hero.maxMp; hero.poison=hero.slow=0; }
    message=enabled?"Testing powers enabled: invulnerable, infinite MP, 12x damage, quick cooldowns":"Normal adventure rules enabled";
}
void Game::setCompanions(bool enabled) {
    companions=enabled; party.clear();
    if(!enabled) return;
    const Role roles[]={Role::Cleric,Role::Warrior,Role::Mage};
    const char* names[]={"Stasia_apk","Jade","Elowen"};
    for(int i=0;i<3;++i) {
        Entity e; e.id=2+i; e.kind=Kind::Party; e.name=names[i]; e.level=hero.level;
        data.applyClass(e,roles[i]); e.pos=hero.pos+Vec2{float(i)*4-4,3}; party.push_back(e);
    }
}
void Game::menuInput(const Input& input) {
    std::string keys=input.keys;
    if(input.click) {
        int width=std::min(78,camera.width-2),height=std::min(30,camera.height-2);
        int x=(camera.width-width)/2,y=(camera.height-height)/2;
        if(input.mouseX>=x+2&&input.mouseX<x+width-2) {
            int row=input.mouseY-y;
            if(screen==Screen::Map&&row>=13&&row<=21&&(row-13)%2==0) keys+=char('1'+(row-13)/2);
            else if(screen!=Screen::Map&&row>=5&&(row-5)%2==0) {
                int visible=std::max(1,(height-7)/2),first=std::max(0,menuIndex-visible+1);
                int selected=first+(row-5)/2,count=screen==Screen::Title?3:screen==Screen::Paused?5:9;
                if(selected<count&&selected<first+visible) { menuIndex=selected; keys+='\r'; }
            }
        }
    }
    for(char k:keys) {
        if(k=='q') { quit=true; return; }
        if(screen==Screen::Map) {
            if(k==27||k=='m'||k=='p') { screen=returnScreen; menuIndex=0; return; }
            if(k=='t'&&hero.testing) {
                Vec2 destination=hero.pos+Vec2{512,256};
                if(std::abs(destination.x)>999000||std::abs(destination.y)>999000) { message="Coordinate precision limit reached; return to a landmark."; return; }
                for(int i=0;i<64&&(world.blocked(destination)||world.water(destination));++i) destination+=Vec2{5,3};
                if(world.blocked(destination)||world.water(destination)) destination.x=std::round(destination.x/256)*256-12*std::sin(destination.y*.009f);
                hero.pos=destination; camera.center=hero.pos; aim=hero.pos+Vec2{5,0};
                for(size_t i=0;i<party.size();++i) party[i].pos=hero.pos+Vec2{float(i)*4-4,3};
                velocity={}; world.focus(hero.pos); streamEnemies(); screen=Screen::Playing;
                message="Procedural frontier reached. Terrain and enemies are generated from the seed."; return;
            }
            if(k>='1'&&k<='5') {
                size_t index=size_t(k-'1');
                if((hero.testing||discovered[index])&&hero.alive&&(!fight.active||hero.testing)) {
                    hero.pos=landmarks[index].pos; camera.center=hero.pos; aim=hero.pos+Vec2{5,0};
                    for(size_t i=0;i<party.size();++i) party[i].pos=hero.pos+Vec2{float(i)*4-4,3};
                    velocity={}; world.focus(hero.pos); screen=Screen::Playing;
                    message=std::string("Arrived at ")+landmarks[index].name;
                } else message="Discover this landmark first; travel is unavailable during combat.";
                return;
            }
            continue;
        }
        int count=screen==Screen::Title?3:screen==Screen::Paused?5:9;
        if(k=='w'||k=='^') menuIndex=(menuIndex+count-1)%count;
        if(k=='s'||k=='v') menuIndex=(menuIndex+1)%count;
        if(k==27||k=='p') {
            if(screen==Screen::Settings) screen=returnScreen;
            else if(screen==Screen::Paused) screen=Screen::Playing;
            menuIndex=0; return;
        }
        bool activate=k=='\r'||k=='\n'||k==' ';
        int direction=(k=='a'||k=='<')?-1:1;
        if(screen==Screen::Settings&&(activate||k=='a'||k=='d'||k=='<'||k=='>')) {
            switch(menuIndex) {
                case 0: setTesting(!hero.testing); break;
                case 1: setCompanions(!companions); break;
                case 2: {
                    const int choices[]={30,60,120}; int index=presentationFps<=30?0:presentationFps<=60?1:2;
                    presentationFps=choices[(index+direction+3)%3]; break;
                }
                case 3: detail=(detail+direction+3)%3; break;
                case 4: camera.zoom=clamp(camera.zoom+direction*.1f,.55f,2.f); break;
                case 5: showNames=!showNames; break;
                case 6: showHelp=!showHelp; break;
                case 7: {
                    int role=(int(hero.role)+direction+5)%5;
                    if(hero.testing||(hero.alive&&!fight.active&&hero.hurtTime<=0)) {
                        data.applyClass(hero,Role(role));
                        for(int item:hero.equipment) if(item>=0) {
                            hero.armor+=data.items[size_t(item)].armor;
                            hero.power+=data.items[size_t(item)].power;
                        }
                    } else message="Class changes require being alive and outside combat.";
                    break;
                }
                case 8: screen=returnScreen; menuIndex=0; return;
            }
        } else if(activate&&screen==Screen::Title) {
            if(menuIndex==0) { screen=Screen::Playing; message="Explore freely. M: world map. Esc/P: pause and settings."; }
            else if(menuIndex==1) { returnScreen=screen; screen=Screen::Settings; }
            else quit=true;
            menuIndex=0; return;
        } else if(activate&&screen==Screen::Paused) {
            if(menuIndex==0) screen=Screen::Playing;
            else if(menuIndex==1) { returnScreen=screen; screen=Screen::Settings; }
            else if(menuIndex==2) { returnScreen=screen; screen=Screen::Map; }
            else if(menuIndex==3) screen=Screen::Title;
            else quit=true;
            menuIndex=0; return;
        }
    }
}
void Game::renderMenu(Framebuffer& frame) {
    // Keep the world as a dim backdrop. No game time advances in any menu.
    for(auto& cell:frame.cells) cell.fg=cell.fg*.24f;
    int width=std::min(78,frame.width-2),height=std::min(30,frame.height-2);
    int x=(frame.width-width)/2,y=(frame.height-height)/2;
    box(frame,x,y,width,height,{120,170,173});
    auto line=[&](int row,const std::string& text,Color color=white) {
        if(row<height-1) frame.text(x+2,y+row,text.substr(0,size_t(std::max(0,width-4))),color);
    };
    auto option=[&](int index,const std::string& text) {
        int visible=std::max(1,(height-7)/2),first=std::max(0,menuIndex-visible+1);
        if(index>=first&&index<first+visible) line(5+(index-first)*2,(menuIndex==index?"> ":"  ")+text,menuIndex==index?Color{255,222,139}:white);
    };
    if(screen==Screen::Title) {
        line(2,"E T E R N A L R P G",{242,210,139});
        option(0,"Enter the open world"); option(1,"Settings"); option(2,"Exit game");
        line(13,"Beyond Ashenmere's ruins lie forests, wetlands and ember hills.");
        line(15,"Explore the land, discover landmarks and defeat the Dragon.");
        line(17,"Earn XP from battles and exploration. Choose any of five classes.");
        line(19,"Single-player adventure. No account, server or connection.");
        line(24,"Seed "+std::to_string(world.seed)+" | Active mods: "+std::to_string(activeMods.size()),muted);
        if(!activeMods.empty()) line(25,activeMods.back(),muted);
        line(21,hero.testing?"TESTING POWERS ON - change this in Settings":"NORMAL ADVENTURE - enemies can defeat you",{222,185,121});
        line(23,"WASD move | Mouse aim | 1-6 cast | M map | Esc/P pause");
    } else if(screen==Screen::Paused) {
        line(2,"PAUSED - the world is stopped",{242,210,139});
        option(0,"Resume adventure"); option(1,"Settings"); option(2,"World map");
        option(3,"Return to title (keeps this session)"); option(4,"Exit game");
        line(18,"Level "+std::to_string(hero.level)+" | XP "+std::to_string(hero.xp)+" / "+std::to_string(100+hero.level*25));
        line(20,"Progress is kept during this session, not after exiting.",muted);
    } else if(screen==Screen::Settings) {
        line(2,"SETTINGS",{242,210,139});
        option(0,std::string("Testing powers: ")+(hero.testing?"ON":"OFF"));
        option(1,std::string("AI companions: ")+(companions?"ON":"OFF (solo)"));
        option(2,"Presentation target: "+std::to_string(presentationFps)+" FPS");
        const char* details[]={"Comfort / fewer glyphs","Detailed","Dense / more GPU work"};
        option(3,std::string("Window detail: ")+details[detail]);
        option(4,"Camera zoom: "+std::to_string(int(camera.zoom*100))+"%");
        option(5,std::string("Name labels: ")+(showNames?"ON":"OFF"));
        option(6,std::string("Control hints: ")+(showHelp?"ON":"OFF"));
        option(7,std::string("Class: ")+roleName(hero.role));
        option(8,"Back");
        line(24,"A/D or Left/Right changes a setting; Enter toggles.",muted);
        line(25,"Window detail applies to Metal; terminal size sets terminal detail.",muted);
    } else if(screen==Screen::Map) {
        line(2,"PROCEDURAL WORLD - local map around you",{242,210,139});
        int mapW=std::min(36,width-6),mapH=8;
        for(int row=0;row<mapH;++row) for(int col=0;col<mapW;++col) {
            Vec2 p=hero.pos+Vec2{-256+512.f*col/mapW,-256+512.f*row/mapH};
            char glyph=world.water(p)?'~':world.road(p)?':':'.';
            frame.put(x+2+col,y+4+row,glyph,{75,115,109});
        }
        auto mark=[&](Vec2 p,char glyph,Color color) {
            if(std::abs(p.x-hero.pos.x)>255||std::abs(p.y-hero.pos.y)>255) return;
            int col=clamp(int((p.x-hero.pos.x+256)/512*mapW),0,mapW-1),row=clamp(int((p.y-hero.pos.y+256)/512*mapH),0,mapH-1);
            frame.put(x+2+col,y+4+row,glyph,color);
        };
        for(size_t i=0;i<landmarks.size();++i) {
            mark(landmarks[i].pos,char('1'+i),{225,183,109});
            line(13+int(i)*2,std::to_string(i+1)+" "+landmarks[i].name+(discovered[i]?" [discovered]":" [unexplored]"));
        }
        mark(hero.pos,'@',white);
        line(12,"Seed "+std::to_string(world.seed)+" | Chunks "+std::to_string(world.loadedChunkCount())+" | Active enemies "+std::to_string(mobs.size()),muted);
        line(23,"@ You: "+std::to_string(int(hero.pos.x))+", "+std::to_string(int(hero.pos.y))+" | "+world.region(hero.pos));
        line(24,hero.testing?"1-5: home landmarks | T: jump to new wilderness":"1-5: travel to discovered landmarks outside boss combat",muted);
        line(25,message,muted);
    }
    line(height-2,screen==Screen::Map?"M / Esc: back":"W/S: select | Enter or click: choose | Esc/P: back",{128,162,174});
}
}
