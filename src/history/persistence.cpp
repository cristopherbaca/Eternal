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
}
bool Game::loadLiving(std::istream& in) {
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
    atlas.focus(interiorSite?outsidePosition:hero.pos); population.tick(atlas,world,time,0,weatherAt(world.terrainSeed,time)!=0);
    return true;
}
}
