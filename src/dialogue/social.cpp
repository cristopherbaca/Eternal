#include "game.hpp"
#include "ui/hud.hpp"
#include <fstream>
#include <iomanip>
namespace er {
void Game::talkNearby() {
    if(world.interior||!hero.alive) return;
    const Person* nearest=nullptr;
    for(const auto& p:population.nearby) if(distance(p.pos,hero.pos)<8&&(!nearest||distance(p.pos,hero.pos)<distance(nearest->pos,hero.pos))) nearest=&p;
    if(!nearest) { message="Approach a person and press Y to talk."; return; }
    talking=nearest->id; screen=Screen::Talk; menuIndex=0; velocity={}; casts.clear();
    dialogue=population.traits[size_t(nearest->trait)].greeting;
}
Person Game::speaker() const { return population.person(atlas.place({std::get<0>(talking),std::get<1>(talking)}),std::get<2>(talking)); }
void Game::learnLore(int index) {
    index=int(size_t(index)%chronicle.events.size()); chronicle.learned.insert(index);
    atlas.known.insert(chronicle.events[size_t(index)].place); loreIndex=index; dialogue=chronicle.account(index);
}
void Game::offerQuest(const Person& person) {
    for(auto& q:quests) if(q.giver==person.id&&!q.rewarded) {
        if(clearedPlaces.count(q.target)) { q.complete=true; q.rewarded=true; hero.gold+=90; awardXP(150); ++reputation[population.jobs[size_t(person.job)].faction]; dialogue="You brought the heirloom back. Take 90 gold and my thanks!"; }
        else { dialogue="Our agreement stands: "+q.title+". Clear its guardians and open the chest."; mapSelected=q.target; }
        return;
    }
    int serial=0; for(const auto& q:quests) if(q.giver==person.id) ++serial;
    Place target=atlas.place(person.home); bool found=false;
    // Deterministic, expanding search chooses a real, uncleared location.
    for(int radius=1;radius<=8&&!found;++radius) for(int y=-radius;y<=radius&&!found;++y) for(int x=-radius;x<=radius;++x) {
        if(std::abs(x)!=radius&&std::abs(y)!=radius) continue;
        target=atlas.place({person.home.first+x,person.home.second+y}); const auto& type=atlas.types[size_t(target.type)];
        bool reserved=std::any_of(quests.begin(),quests.end(),[&](const Quest& q){return !q.rewarded&&q.target==target.id;});
        if(type.interior&&!type.inhabited&&!clearedPlaces.count(target.id)&&!reserved) { found=true; break; }
    }
    if(!found||quests.size()>=1000) { dialogue="No new work today. Explore farther afield."; return; }
    std::string title="Recover the lost heirloom from "+target.name;
    auto path=sourceData/"quest_templates.txt"; if(!std::filesystem::exists(path)) path=std::filesystem::path(ER_DATA_DIR)/"quest_templates.txt";
    std::ifstream in(path); std::vector<std::string> templates; std::string line; while(in>>std::quoted(line)) templates.push_back(line);
    if(!templates.empty()) { title=templates[(person.key+uint32_t(serial))%templates.size()]; auto at=title.find("{place}"); if(at!=std::string::npos) title.replace(at,7,target.name); }
    quests.push_back({person.id,target.id,serial,false,false,title}); atlas.known.insert(target.id); mapSelected=target.id;
    dialogue=title+". Clear the guardians, open the chest, then return to me. Reward: 90 gold, 150 XP.";
}
void Game::socialInput(const Input& input) {
    std::string keys=input.keys;
    int count=screen==Screen::Talk?8:screen==Screen::Trade?6:screen==Screen::Journal?std::max(1,int(quests.size())+1):std::max(1,int(chronicle.events.size()));
    if(input.click) {
        int width=std::min(78,uiWidth-2),height=std::min(30,uiHeight-2),x=(uiWidth-width)/2,y=(uiHeight-height)/2;
        int row=input.mouseY-y,visible=std::max(1,(height-14)/2),first=std::max(0,menuIndex-visible+1);
        if(input.mouseX>=x+2&&input.mouseX<x+width-2&&row>=5&&(row-5)%2==0&&(row-5)/2<visible) { int selected=first+(row-5)/2; if(selected<count) { menuIndex=selected; keys+='\r'; } }
    }
    for(char k:keys) {
        if(k==27||k=='y'||k=='j'||k=='l'||k=='p') { screen=Screen::Playing; menuIndex=0; return; }
        if(k=='q') { quit=true; return; }
        if(k=='w'||k=='^') menuIndex=(menuIndex+count-1)%count;
        if(k=='s'||k=='v') menuIndex=(menuIndex+1)%count;
        if(k!='\r'&&k!='\n'&&k!=' ') continue;
        if(screen==Screen::Journal) { if(menuIndex<int(quests.size())) { if(world.interior) { dialogue="Leave the building before opening the world atlas."; return; } mapSelected=quests[size_t(menuIndex)].target; mapCenter=atlas.place(*mapSelected).pos; screen=Screen::Map; returnScreen=Screen::Playing; } else { screen=Screen::Lore; menuIndex=0; } return; }
        if(screen==Screen::Lore) { if(chronicle.learned.count(menuIndex)) dialogue=chronicle.account(menuIndex); else dialogue="Find books, inscriptions or ask townspeople about this chapter."; return; }
        Person p=speaker(); const auto& job=population.jobs[size_t(p.job)]; const auto& trait=population.traits[size_t(p.trait)];
        if(screen==Screen::Trade) { if(menuIndex==5) { screen=Screen::Talk; menuIndex=0; } else tradeWith(p,menuIndex); return; }
        switch(menuIndex) {
            case 0: dialogue=trait.greeting+" I am "+p.name+", a "+std::to_string(p.age)+"-year-old "+p.culture+" "+job.name+". I like "+trait.likes+"; I dislike "+trait.dislikes+"."; break;
            case 1: learnLore(p.job==6?3:int(p.key%chronicle.events.size())); dialogue="A rumor from our elders: "+dialogue; break;
            case 2: { auto relative=population.person(atlas.place(p.home),p.familyIndex); auto friendPerson=population.person(atlas.place(p.home),p.friendIndex); dialogue="My kin "+relative.name+" lives here. My friend "+friendPerson.name+" follows "+population.jobs[size_t(friendPerson.job)].faction+". We gather near the inn at dusk."; break; }
            case 3: offerQuest(p); break;
            case 4: if(p.job==1||p.job==3||p.job==7||p.job==8) { screen=Screen::Trade; menuIndex=0; dialogue="Buy, sell, or ask for a service."; } else dialogue="I am no trader. Look for a Smith, Innkeeper, Merchant or Alchemist."; break;
            case 5: if(hero.gold<10) dialogue="A meal and rest costs 10 gold."; else { hero.gold-=10; hero.hp=hero.maxHp; hero.mp=hero.maxMp; time+=180; dialogue="You share a meal and rest. The world moves on."; } break;
            case 6: {
                auto& state=population.changes[p.id];
                if(state.recruited) { dialogue="I am already traveling with you."; break; }
                if(hero.gold<40||party.size()>=3) { dialogue="Hiring costs 40 gold; at most three companions can travel with you."; break; }
                Entity e; e.id=2+int(party.size()); e.name=p.name; e.kind=Kind::Party; e.level=hero.level; data.applyClass(e,Role(p.job%5)); e.pos=hero.pos+Vec2{2,0}; party.push_back(e); companions=true; hero.gold-=40; state.recruited=true; dialogue="I will walk beside you."; break;
            }
            default: screen=Screen::Playing; break;
        }
    }
}
void Game::renderSocial(Framebuffer& f) const {
    int w=std::min(78,f.width-2),h=std::min(30,f.height-2),x=(f.width-w)/2,y=(f.height-h)/2;
    box(f,x,y,w,h,{120,170,173});
    auto line=[&](int row,const std::string& text,Color color=white) { if(row>0&&row<h-1) f.text(x+2,y+row,text.substr(0,size_t(w-4)),color); };
    auto option=[&](int i,const std::string& text) { int visible=std::max(1,(h-14)/2),first=std::max(0,menuIndex-visible+1); if(i>=first&&i<first+visible) line(5+(i-first)*2,(i==menuIndex?"> ":"  ")+text,i==menuIndex?Color{255,222,139}:white); };
    if(screen==Screen::Talk) {
        auto p=speaker(); line(2,p.name+" / "+population.jobs[size_t(p.job)].name+" / "+population.traits[size_t(p.trait)].name,{225,200,145});
        const char* topics[]={"About you","Rumors and history","Family and people","Work / turn in quest","Trade and services","Rest and a meal (10g)","Hire companion (40g)","Goodbye"}; for(int i=0;i<8;++i) option(i,topics[i]);
    } else if(screen==Screen::Trade) {
        auto p=speaker(); line(2,"TRADE / "+p.name+" / Your gold: "+std::to_string(hero.gold),{225,200,145});
        for(int i=0;i<3;++i) { int id=int((p.key+uint32_t(i))%data.items.size()); const auto& item=data.items[size_t(id)]; option(i,"Buy "+item.name+" ("+std::to_string(buyPrice(p,item))+"g)"); }
        option(3,hero.inventory.empty()?"Sell: inventory empty":"Sell selected: "+data.items[size_t(hero.inventory[size_t(clamp(inventoryIndex,0,int(hero.inventory.size())-1))])].name);
        option(4,"Smith/alchemist service: restore HP/MP (15g)"); option(5,"Back");
    } else if(screen==Screen::Journal) {
        line(2,"QUEST JOURNAL",{225,200,145});
        for(size_t i=0;i<quests.size();++i) { const auto& q=quests[i]; option(int(i),(q.rewarded?"[DONE] ":clearedPlaces.count(q.target)?"[RETURN] ":"[ACTIVE] ")+q.title); }
        option(int(quests.size()),"Read learned lore (L)");
        line(h-9,!chronicle.learned.count(3)?"Main story: ask a Scholar about the Dragon Age.":!clearedPlaces.count(chronicle.events[3].place)?"Main story: recover the relic at "+atlas.place(chronicle.events[3].place).name: fight.won?"Main story: the Dragon is defeated.":"Main story: the relic points to Dragon Hollow. M, then 5.",{220,200,150});
    } else {
        line(2,"LORE / WORLD CHRONICLE",{225,200,145});
        for(size_t i=0;i<chronicle.events.size();++i) option(int(i),chronicle.learned.count(int(i))?"Year "+std::to_string(chronicle.events[i].year)+" / "+atlas.place(chronicle.events[i].place).name:"[Undiscovered chapter]");
    }
    size_t offset=0; for(int row=h-8;row<h-2&&offset<dialogue.size();++row) { size_t length=std::min(size_t(w-4),dialogue.size()-offset); if(offset+length<dialogue.size()) { auto space=dialogue.rfind(' ',offset+length); if(space!=std::string::npos&&space>offset) length=space-offset; } line(row,dialogue.substr(offset,length),{160,194,204}); offset+=length; while(offset<dialogue.size()&&dialogue[offset]==' ') ++offset; }
    line(h-2,"W/S choose | Enter/click select | Esc close",muted);
}
}
