#include "game.hpp"
#include <iomanip>
namespace er {
void Game::saveLiving(std::ostream& out) const {
    out<<"LIVING 1\n"; population.save(out);
    out<<chronicle.events.size()<<'\n';
    for(const auto& event:chronicle.events) out<<event.year<<' '<<event.place.first<<' '<<event.place.second<<' '<<std::quoted(event.person)<<' '<<std::quoted(event.faction)<<' '<<std::quoted(event.text)<<'\n';
    out<<chronicle.learned.size()<<' '; for(int id:chronicle.learned) out<<id<<' '; out<<'\n';
    out<<quests.size()<<'\n';
    for(const auto& q:quests) out<<std::get<0>(q.giver)<<' '<<std::get<1>(q.giver)<<' '<<std::get<2>(q.giver)<<' '<<q.target.first<<' '<<q.target.second<<' '<<q.serial<<' '<<q.complete<<' '<<q.rewarded<<' '<<std::quoted(q.title)<<'\n';
    out<<reputation.size()<<'\n'; for(const auto& entry:reputation) out<<std::quoted(entry.first)<<' '<<entry.second<<'\n';
    out<<"PROGRESSION 1\n";
    for(int n:mastery) out<<n<<' '; out<<'\n';
    for(int n:talents) out<<n<<' '; out<<'\n';
    for(int n:materials) out<<n<<' '; out<<'\n';
    for(int n:craftPractice) out<<n<<' '; out<<'\n';
    auto writeMap=[&](const std::map<PlaceId,int>& values) { out<<values.size()<<'\n'; for(const auto& e:values) out<<e.first.first<<' '<<e.first.second<<' '<<e.second<<'\n'; };
    writeMap(harvested); writeMap(bounties);
    out<<eventLog.size()<<'\n'; for(const auto& entry:eventLog) out<<std::quoted(entry.substr(0,1000))<<'\n';
}
bool Game::loadLiving(std::istream& in,int saveVersion) {
    std::string magic; int version; if(!(in>>magic>>version)||magic!="LIVING"||version!=1||!population.load(in)) return false;
    size_t count; if(!(in>>count)||count!=24) return false; chronicle.events.resize(count);
    for(auto& event:chronicle.events) if(!(in>>event.year>>event.place.first>>event.place.second>>std::quoted(event.person)>>std::quoted(event.faction)>>std::quoted(event.text))||event.person.size()>80||event.faction.size()>80||event.text.size()>1000||std::abs(int64_t(event.place.first))>=4000||std::abs(int64_t(event.place.second))>=4000) return false;
    if(!(in>>count)||count>24) return false;
    for(size_t i=0;i<count;++i) { int id; if(!(in>>id)||id<0||id>=24) return false; chronicle.learned.insert(id); }
    if(!(in>>count)||count>1000) return false; quests.resize(count);
    for(auto& q:quests) {
        int x,y,n; if(!(in>>x>>y>>n>>q.target.first>>q.target.second>>q.serial>>q.complete>>q.rewarded>>std::quoted(q.title))||n<0||n>=12||q.title.size()>256||q.serial<0||std::abs(int64_t(x))>=4000||std::abs(int64_t(y))>=4000||std::abs(int64_t(q.target.first))>=4000||std::abs(int64_t(q.target.second))>=4000) return false;
        q.giver={x,y,n};
    }
    if(!(in>>count)||count>64) return false;
    for(size_t i=0;i<count;++i) { std::string name; int value; if(!(in>>std::quoted(name)>>value)||name.size()>80) return false; reputation[name]=value; }
    if(saveVersion>=5) {
        std::string tag; int format; if(!(in>>tag>>format)||tag!="PROGRESSION"||format!=1) return false;
        auto numbers=[&](auto& values,int limit) { for(int& n:values) if(!(in>>n)||n<0||n>limit) return false; return true; };
        if(!numbers(mastery,100000)||!numbers(talents,5)||!numbers(materials,1000000)||!numbers(craftPractice,1000000)||perkPoints()<0) return false;
        auto readMap=[&](std::map<PlaceId,int>& values,int bound) { size_t size; if(!(in>>size)||size>1000000) return false; for(size_t i=0;i<size;++i) { int x,y,n; if(!(in>>x>>y>>n)||std::abs(int64_t(x))>bound||std::abs(int64_t(y))>bound||n<0||n>1000000) return false; values[{x,y}]=n; } return true; };
        if(!readMap(harvested,16000)||!readMap(bounties,4000)) return false;
        if(!(in>>count)||count>5) return false; for(size_t i=0;i<count;++i) { std::string entry; if(!(in>>std::quoted(entry))||entry.size()>1000) return false; eventLog.push_back(entry); }
        if(!eventLog.empty()) lastEvent=eventLog.front();
    }
    atlas.focus(interiorSite?outsidePosition:hero.pos); population.tick(atlas,world,time,0,weatherAt(world.terrainSeed,time)!=0);
    return true;
}
}
