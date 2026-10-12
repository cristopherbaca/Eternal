#include "npc/population.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace er {
Population::Population(const std::filesystem::path& data) {
    auto rows=[&](const char* name,auto read) { auto path=data/name; if(!std::filesystem::exists(path)) path=std::filesystem::path(ER_DATA_DIR)/name; std::ifstream in(path); std::string text; while(std::getline(in,text)) if(!text.empty()&&text[0]!='#') { std::istringstream line(text); read(line); if(!line) throw std::runtime_error(std::string("Invalid ")+name); } };
    rows("npc_names.txt",[&](std::istream& in){Name n; in>>std::quoted(n.culture)>>std::quoted(n.given)>>std::quoted(n.family); names.push_back(n);});
    rows("jobs.txt",[&](std::istream& in){Job j; in>>std::quoted(j.name)>>j.glyph>>std::quoted(j.faction); jobs.push_back(j);});
    rows("dialogue.txt",[&](std::istream& in){Trait t; in>>std::quoted(t.name)>>std::quoted(t.greeting)>>std::quoted(t.likes)>>std::quoted(t.dislikes); traits.push_back(t);});
    if(names.empty()||jobs.empty()||traits.empty()) throw std::runtime_error("NPC tables are empty");
}
Person Population::person(const Place& home,int index) const {
    uint32_t key=hash(home.key+uint32_t(index)*7187); const auto& n=names[key%names.size()];
    std::vector<std::string> families; for(const auto& name:names) if(name.culture==n.culture) families.push_back(name.family);
    Person p; p.id={home.id.first,home.id.second,index}; p.home=home.id; p.key=key; p.age=18+int((key>>12)%63);
    p.job=index%int(jobs.size()); p.trait=int((key>>8)%traits.size()); p.gold=25+int(key%100);
    p.name=n.given+" "+families[(key>>5)%families.size()]+" "+std::to_string(index+1); p.culture=n.culture;
    p.friendIndex=(index+1)%12; p.rivalIndex=(index+7)%12; p.familyIndex=(index/2)*2+(1-index%2);
    return p;
}
Vec2 Population::scheduled(const Person& p,const Atlas& atlas,float hour,bool shelter) const {
    Vec2 home=atlas.place(p.home).pos; int index=std::get<2>(p.id);
    Vec2 bed=home+Vec2{float(index%4)*4-6,-8+float(index/4)*3};
    Vec2 work=home+Vec2{float(index%4)*5-7,3+float(index/4)*3};
    Vec2 inn=home+Vec2{float(index%3)*3-3,float(index/3)*2};
    if(shelter||hour<6||hour>=22) return bed;
    if(hour>=18) return inn;
    return work;
}
void Population::tick(const Atlas& atlas,const World& world,float time,float dt,bool shelter) {
    auto id=Atlas::tile(atlas.nearby[4].pos);
    float hour=worldHour(time);
    if(center!=id) {
        center=id; std::map<NpcId,Vec2> old; for(const auto& p:nearby) old[p.id]=p.pos; nearby.clear();
        for(const auto& site:atlas.nearby) if(atlas.types[size_t(site.type)].inhabited) for(int i=0;i<12;++i) {
            Person p=person(site,i); p.pos=old.count(p.id)?old[p.id]:scheduled(p,atlas,hour,shelter); nearby.push_back(p);
        }
    }
    for(auto& p:nearby) { Vec2 d=scheduled(p,atlas,hour,shelter)-p.pos; float len=length(d); if(len>.1f) p.pos=world.move(p.pos,normalized(d)*std::min(len,dt*2.3f)); }
}
void Population::render(Scene& scene,float time) const {
    for(const auto& p:nearby) {
        auto found=changes.find(p.id); if(found!=changes.end()&&found->second.recruited) continue;
        Vec2 screen=scene.camera.project(p.pos); if(screen.x<0||screen.y<0||screen.x>scene.camera.width||screen.y>scene.camera.height) continue;
        Color color=p.job==2?Color{140,181,225}:p.job==1?Color{228,159,109}:Color{151,213,181};
        scene.point(p.pos,3,color,U'o',{},.4f); scene.point(p.pos,2,color,jobs[size_t(p.job)].glyph,{},.4f);
        scene.point(p.pos,1,color,int(time*3+p.key)%2?U'/':U'\\',{},.3f);
    }
}
void Population::labels(Framebuffer& f,const Camera& camera,Vec2 hero,Vec2 aim) const {
    const Person* nearest=nullptr;
    for(const auto& p:nearby) if((!changes.count(p.id)||!changes.at(p.id).recruited)&&distance(p.pos,hero)<10&&(!nearest||distance(p.pos,hero)<distance(nearest->pos,hero))) nearest=&p;
    for(const auto& p:nearby) if((!changes.count(p.id)||!changes.at(p.id).recruited)&&(&p==nearest||distance(p.pos,aim)<1.2f)) {
        Vec2 screen=camera.project(p.pos,5); std::string label=p.name+" / "+jobs[size_t(p.job)].name;
        f.text(int(screen.x)-int(label.size()/2),int(screen.y),label,{164,217,189});
    }
}
void Population::save(std::ostream& out) const {
    out<<changes.size()<<'\n'; for(const auto& entry:changes) out<<std::get<0>(entry.first)<<' '<<std::get<1>(entry.first)<<' '<<std::get<2>(entry.first)<<' '<<entry.second.gold<<' '<<entry.second.affinity<<' '<<entry.second.recruited<<'\n';
    out<<nearby.size()<<'\n'; for(const auto& p:nearby) out<<std::get<0>(p.id)<<' '<<std::get<1>(p.id)<<' '<<std::get<2>(p.id)<<' '<<p.pos.x<<' '<<p.pos.y<<'\n';
}
bool Population::load(std::istream& in) {
    size_t count; if(!(in>>count)||count>100000) return false;
    for(size_t i=0;i<count;++i) { int x,y,n; PersonState state; if(!(in>>x>>y>>n>>state.gold>>state.affinity>>state.recruited)||n<0||n>=12||state.gold<-1||std::abs(int64_t(x))>=4000||std::abs(int64_t(y))>=4000) return false; changes[{x,y,n}]=state; }
    if(!(in>>count)||count>108) return false;
    // These positions are reconciled against generated identities after the first population focus.
    nearby.clear();
    for(size_t i=0;i<count;++i) { int x,y,n; Person p; if(!(in>>x>>y>>n>>p.pos.x>>p.pos.y)||n<0||n>=12||!std::isfinite(p.pos.x)||!std::isfinite(p.pos.y)) return false; p.id={x,y,n}; nearby.push_back(p); }
    return true;
}
}
