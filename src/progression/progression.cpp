#include "game.hpp"
#include "ui/hud.hpp"
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
namespace er {
void Game::applyPlayerClass(Role role) {
    data.applyClass(hero,role);
    int base=int(role)*3;
    hero.maxHp+=talents[size_t(base)]*8; hero.hp=hero.maxHp;
    hero.power+=talents[size_t(base+1)]*2; hero.armor+=talents[size_t(base+2)]*2;
}
int Game::perkPoints() const { return std::accumulate(mastery.begin(),mastery.end(),0)/20-std::accumulate(talents.begin(),talents.end(),0); }
int Game::carryWeight() const { int total=0; for(int id:hero.inventory) { int slot=data.items[size_t(id)].slot; total+=slot==0?3:slot==1?4:1; } return total; }
bool Game::canCarry(int id) const { return hero.inventory.size()<24&&carryWeight()+(data.items[size_t(id)].slot==0?3:data.items[size_t(id)].slot==1?4:1)<=60; }
void Game::gather() {
    if(!hero.alive) return;
    if(world.interior&&(!interiorSite||atlas.types[size_t(atlas.place(*interiorSite).type)].name!="Mine")) { message="Forage outside, or gather ore inside a mine."; return; }
    Vec2 pos=interiorSite?outsidePosition:hero.pos; auto tile=World::chunkAt(pos); int day=int(time/1080);
    auto found=harvested.find(tile); if(found!=harvested.end()&&found->second==day) { message="This area has been gathered today. Explore another chunk."; return; }
    harvested[tile]=day;
    int resource=world.interior||world.biome(pos)==5?0:world.biome(pos)==3?1:int(hash(uint32_t(tile.first)^uint32_t(tile.second)*971^world.terrainSeed)%3);
    ++materials[size_t(resource)]; const char* names[]={"ore","herbs","food"}; message=std::string("Gathered ")+names[resource]+". U: skills and crafting";
}
void Game::progressInput(const Input& input) {
    std::string keys=input.keys;
    if(input.click) {
        int w=std::min(78,uiWidth-2),h=std::min(30,uiHeight-2),x=(uiWidth-w)/2,y=(uiHeight-h)/2;
        int row=input.mouseY-y,visible=std::max(1,(h-14)/2),first=std::max(0,menuIndex-visible+1);
        if(input.mouseX>=x+2&&input.mouseX<x+w-2&&row>=5&&(row-5)%2==0&&(row-5)/2<visible) { int selected=first+(row-5)/2; if(selected<8) { menuIndex=selected; keys+='\r'; } }
    }
    for(char k:keys) {
        if(k==27||k=='u'||k=='p') { screen=Screen::Playing; return; }
        if(k=='q') { quit=true; return; }
        if(k=='w'||k=='^') menuIndex=(menuIndex+7)%8;
        if(k=='s'||k=='v') menuIndex=(menuIndex+1)%8;
        if(k!='\r'&&k!='\n'&&k!=' ') continue;
        if(menuIndex<3) {
            int slot=int(hero.role)*3+menuIndex;
            if(perkPoints()<=0||talents[size_t(slot)]>=5) { dialogue="Earn one perk point per 20 successful casts. Rank cap: 5."; continue; }
            ++talents[size_t(slot)]; if(menuIndex==0) { hero.maxHp+=8; hero.hp+=8; } else if(menuIndex==1) hero.power+=2; else hero.armor+=2;
            dialogue="Talent learned for this class.";
        } else if(menuIndex<7) craft(menuIndex-3);
        else screen=Screen::Playing;
    }
}
void Game::craft(int index) {
    auto path=sourceData/"recipes.txt"; if(!std::filesystem::exists(path)) path=std::filesystem::path(ER_DATA_DIR)/"recipes.txt";
    std::ifstream in(path); std::string row; int recipe=0;
    while(std::getline(in,row)) {
        if(row.empty()||row[0]=='#') continue;
        if(recipe++!=index) continue;
        std::istringstream line(row); std::string name,skill; std::array<int,3> cost{}; int gold,item;
        if(!(line>>std::quoted(name)>>std::quoted(skill)>>cost[0]>>cost[1]>>cost[2]>>gold>>item)||gold<0||item<-2||item>=int(data.items.size())||std::any_of(cost.begin(),cost.end(),[](int n){return n<0;})) { dialogue="Invalid recipe."; return; }
        for(size_t i=0;i<3;++i) if(materials[i]<cost[i]) { dialogue="Need ore/herbs/food: "+std::to_string(cost[0])+"/"+std::to_string(cost[1])+"/"+std::to_string(cost[2])+" and "+std::to_string(gold)+" gold. G: gather."; return; }
        if(hero.gold<gold||(item>=0&&!canCarry(item))) { dialogue="Not enough gold or carrying capacity."; return; }
        for(size_t i=0;i<3;++i) materials[i]-=cost[i]; hero.gold-=gold; ++craftPractice[size_t(index)];
        if(item>=0) hero.inventory.push_back(item);
        else if(item==-1) hero.hp=std::min(hero.maxHp,hero.hp+50);
        else { hero.hp=std::min(hero.maxHp,hero.hp+20); hero.mp=std::min(hero.maxMp,hero.mp+30); }
        dialogue="Made "+name+". "+skill+" practice increased."; return;
    }
    dialogue="Recipe unavailable.";
}
void Game::renderProgress(Framebuffer& f) const {
    int w=std::min(78,f.width-2),h=std::min(30,f.height-2),x=(f.width-w)/2,y=(f.height-h)/2;
    box(f,x,y,w,h,{120,170,173});
    auto line=[&](int row,const std::string& text,Color color=white) { if(row>0&&row<h-1) f.text(x+2,y+row,text.substr(0,size_t(w-4)),color); };
    line(2,std::string(roleName(hero.role))+" MASTERY / "+std::to_string(mastery[size_t(hero.role)])+" casts / "+std::to_string(perkPoints())+" perk points",{229,201,141});
    const char* names[]={"Vitality +8 HP","Focus +2 power","Ward +2 armor","Smith iron sword (3 ore, 10g)","Brew tonic (3 herbs)","Enchant charm (1 ore, 2 herbs, 20g)","Cook meal (2 food)","Back"};
    int visible=std::max(1,(h-14)/2),first=std::max(0,menuIndex-visible+1);
    for(int i=first;i<8&&i<first+visible;++i) line(5+(i-first)*2,(i==menuIndex?"> ":"  ")+std::string(names[i])+(i<3?" ["+std::to_string(talents[size_t(int(hero.role)*3+i)])+"/5]":""),i==menuIndex?Color{255,222,139}:white);
    line(h-8,"Ore "+std::to_string(materials[0])+" | Herbs "+std::to_string(materials[1])+" | Food "+std::to_string(materials[2]));
    line(h-7,"Craft practice S/A/E/C: "+std::to_string(craftPractice[0])+"/"+std::to_string(craftPractice[1])+"/"+std::to_string(craftPractice[2])+"/"+std::to_string(craftPractice[3]),muted);
    line(h-6,"Carry weight "+std::to_string(carryWeight())+" / 60 | G: gather outside",muted);
    line(h-4,dialogue,{160,194,204}); line(h-2,"W/S select | Enter/click choose | Esc back",muted);
}
}
