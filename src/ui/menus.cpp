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
        int cols=uiWidth?uiWidth:camera.width,rows=uiHeight?uiHeight:camera.height;
        int width=std::min(78,cols-2),height=std::min(30,rows-2);
        int x=(cols-width)/2,y=(rows-height)/2;
        if(input.mouseX>=x+2&&input.mouseX<x+width-2) {
            int row=input.mouseY-y;
            if(screen!=Screen::Map&&row>=5&&(row-5)%2==0) {
                int visible=std::max(1,(height-7)/2),first=std::max(0,menuIndex-visible+1);
                int selected=first+(row-5)/2,count=menuCount();
                if(selected<count&&selected<first+visible) { editingField=false; menuIndex=selected; keys+='\r'; }
            }
        }
    }
    for(char k:keys) {
        if(screen==Screen::Create&&editingField) {
            std::string& field=menuIndex==0?draftWorld:menuIndex==1?draftName:draftSeed;
            if(k=='\r'||k=='\n'||k==27) editingField=false;
            else if(k==127||k==8) { if(!field.empty()) field.pop_back(); }
            else if(k>=32&&k<=126&&field.size()<size_t(menuIndex==3?12:menuIndex==1?20:24)) {
                if(menuIndex!=3||(k>='0'&&k<='9')) field+=k;
            }
            continue;
        }
        if(k=='q') { quit=true; return; }
        if(screen==Screen::Map) {
            if(k=='w'||k=='^') mapCenter.y-=mapStep*4;
            if(k=='s'||k=='v') mapCenter.y+=mapStep*4;
            if(k=='a'||k=='<') mapCenter.x-=mapStep*4;
            if(k=='d'||k=='>') mapCenter.x+=mapStep*4;
            if(k=='c') mapCenter=hero.pos;
            if(k=='+'||k=='=') mapStep=std::max(4.f,mapStep/2);
            if(k=='-') mapStep=std::min(128.f,mapStep*2);
            mapCenter.x=clamp(mapCenter.x,-999000.f,999000.f); mapCenter.y=clamp(mapCenter.y,-999000.f,999000.f);
            if(k=='\t'&&!atlas.known.empty()) { auto it=mapSelected?atlas.known.upper_bound(*mapSelected):atlas.known.begin(); if(it==atlas.known.end()) it=atlas.known.begin(); mapSelected=*it; mapCenter=atlas.place(*it).pos; }
            if((k=='\r'||k=='\n')&&mapSelected&&atlas.known.count(*mapSelected)&&hero.alive&&(!fight.active||hero.testing)) {
                hero.pos=atlas.place(*mapSelected).pos+Vec2{0,5}; camera.center=hero.pos; world.focus(hero.pos); atlas.focus(hero.pos); velocity={}; screen=Screen::Playing; return;
            }
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
        int count=menuCount();
        if(k=='w'||k=='^') menuIndex=(menuIndex+count-1)%count;
        if(k=='s'||k=='v') menuIndex=(menuIndex+1)%count;
        if(k==27||k=='p') {
            if(screen==Screen::Create||screen==Screen::Saves) screen=Screen::Title;
            else if(screen==Screen::SaveActions||screen==Screen::DeleteSave) { screen=Screen::Saves; refreshSaves(); }
            else if(screen==Screen::Settings) screen=returnScreen;
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
        } else if(screen==Screen::Create&&(activate||k=='<'||k=='>'||k=='a'||k=='d')) {
            if(menuIndex==2) draftRole=Role((int(draftRole)+direction+5)%5);
            else if(menuIndex==4) draftGod=!draftGod;
            else if(activate) {
                if(menuIndex==0||menuIndex==1||menuIndex==3) editingField=true;
                else if(menuIndex==5) { createAdventure(); return; }
                else { screen=Screen::Title; menuIndex=0; }
            }
        } else if(activate&&screen==Screen::Title) {
            menuNotice.clear();
            if(menuIndex==0) beginCreation();
            else if(menuIndex==1) { refreshSaves(); screen=Screen::Saves; menuIndex=0; }
            else if(menuIndex==2) { returnScreen=screen; screen=Screen::Settings; menuIndex=0; }
            else quit=true;
            return;
        } else if(activate&&screen==Screen::Saves) {
            if(menuIndex==8) screen=Screen::Title;
            else { selectedSave=menuIndex; screen=Screen::SaveActions; }
            menuIndex=0; menuNotice.clear(); return;
        } else if(activate&&screen==Screen::SaveActions) {
            if(menuIndex==0) {
                if(saves[size_t(selectedSave)].exists) { if(activeSave>=0&&!saveAdventure()) return; loadAdventure(selectedSave); }
                else beginCreation(selectedSave);
            } else if(menuIndex==1&&saves[size_t(selectedSave)].exists) screen=Screen::DeleteSave;
            else { screen=Screen::Saves; refreshSaves(); }
            menuIndex=0; return;
        } else if(activate&&screen==Screen::DeleteSave) {
            if(menuIndex==1) deleteAdventure(selectedSave);
            screen=Screen::Saves; menuIndex=0; return;
        } else if(activate&&screen==Screen::Paused) {
            if(menuIndex==0) screen=Screen::Playing;
            else if(menuIndex==1) { returnScreen=screen; screen=Screen::Settings; }
            else if(menuIndex==2) { returnScreen=screen; screen=Screen::Map; mapCenter=hero.pos; }
            else if(menuIndex==3) { saveAdventure(); return; }
            else if(menuIndex==4) { if(!saveAdventure()) return; screen=Screen::Title; refreshSaves(); }
            else { if(!saveAdventure()) return; quit=true; }
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
        line(1,"A S H E N M E R E   /   A   S O L O   A D V E N T U R E",{128,174,187});
        line(2,"E T E R N A L   R P G",{242,210,139});
        option(0,"New adventure - create world & character"); option(1,"Continue / manage saved worlds");
        option(2,"Settings"); option(3,"Exit game");
        line(15,R"(                 /\          /\)",{111,166,183});
        line(16,R"(            /\ /__\   /\   /__\)",{111,166,183});
        line(17,R"(       ____/  \|[] |__/  \__|[] |____)",{148,182,190});
        line(18,R"(      /    /\ |   | /\    |   |    \)", {148,182,190});
        line(19,R"(     /____/  \|___|/  \___|___|_____\)",{111,166,183});
        line(21,"No possessions. No destiny. A world waiting for your story.");
        line(23,"Procedural worlds / 12-digit seeds / 5 classes / 8 save slots",muted);
        line(25,"Create a normal adventure, or enable God mode for testing.",muted);
    } else if(screen==Screen::Create) {
        line(2,"BEGIN YOUR STORY",{242,210,139});
        option(0,"World name: "+draftWorld+(editingField&&menuIndex==0?"_":""));
        option(1,"Player name: "+draftName+(editingField&&menuIndex==1?"_":""));
        option(2,std::string("Class: ")+roleName(draftRole)+"  < >");
        option(3,"Seed (12 digits): "+draftSeed+(editingField&&menuIndex==3?"_":""));
        option(4,std::string("God mode: ")+(draftGod?"ON - invulnerable / unlimited MP":"OFF - normal adventure"));
        option(5,"Create world and begin"); option(6,"Back");
        line(21,"Start at level 1 with no gold, items or equipped gear.",muted);
        line(22,"Weapon / clothing / charm: find loot and equip it with I.",muted);
        line(24,editingField?"Type to edit / Backspace to erase / Enter to finish":"Enter a field to edit it. Left/Right changes class or God mode.",muted);
    } else if(screen==Screen::Saves) {
        line(2,"YOUR WORLDS",{242,210,139});
        for(int i=0;i<8;++i) option(i,std::to_string(i+1)+"  "+saves[size_t(i)].label);
        option(8,"Back"); line(24,"Select a slot to continue or delete it.",muted);
    } else if(screen==Screen::SaveActions) {
        line(2,"SLOT "+std::to_string(selectedSave+1)+" / "+saves[size_t(selectedSave)].label,{242,210,139});
        option(0,saves[size_t(selectedSave)].exists?"Continue this adventure":"Create a new adventure");
        option(1,"Delete this save"); option(2,"Back");
    } else if(screen==Screen::DeleteSave) {
        line(2,"DELETE SAVED WORLD?",{244,147,119});
        option(0,"Cancel - keep this world"); option(1,"Delete permanently");
        line(12,saves[size_t(selectedSave)].label); line(15,"This removes the world and its character progress.",muted);
    } else if(screen==Screen::Paused) {
        line(2,"PAUSED / "+worldName,{242,210,139});
        option(0,"Resume adventure"); option(1,"Settings"); option(2,"World map");
        option(3,"Save now"); option(4,"Save and return to title"); option(5,"Save and exit");
        line(20,"Level "+std::to_string(hero.level)+" | XP "+std::to_string(hero.xp)+" / "+std::to_string(100+hero.level*25));
        line(22,activeSave>=0?"Autosave every 30 seconds. Progress saved on exit.":"Unsaved demo session. Create a world from the title screen.",muted);
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
        line(2,"WORLD ATLAS / "+seedText(),{242,210,139});
        drawMap(frame,x+2,y+4,width-4,std::max(1,height-12),mapCenter,mapStep,true);
        line(height-7,mapSelected?atlas.place(*mapSelected).name+" / Enter: travel":"Tab: select discovered place / Enter: fast travel",{221,199,145});
        line(height-6,"WASD/arrows pan | +/- zoom | C center | blank = unexplored",muted);
        line(height-5,"@ you V village T town C capital D crypt O cave ! camp",muted);
        line(height-4,"Discovered "+std::to_string(atlas.known.size())+" places | God mode reveals terrain",muted);
    }
    if(!menuNotice.empty()) line(height-3,menuNotice,{243,184,122});
    line(height-2,screen==Screen::Map?"M / Esc: back":"W/S: select | Enter or click: choose | Esc/P: back",{128,162,174});
}
}
