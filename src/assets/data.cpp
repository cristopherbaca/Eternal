#include "assets/data.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
namespace er {
Role parseRole(const std::string& name) { for(int i=0;i<5;++i) if(name==roleName(Role(i))) return Role(i); throw std::runtime_error("Unknown class: "+name); }
Data::Data(const std::filesystem::path& requested) {
    auto root=requested.empty()?std::filesystem::path(ER_DATA_DIR):requested;
    auto read=[&](const char* file,auto consume) { std::ifstream f(root/file); if(!f) throw std::runtime_error("Cannot read game data: "+(root/file).string()); std::string line; int row=0;
        while(std::getline(f,line)) { ++row; if(line.empty()||line[0]=='#') continue; std::istringstream in(line); consume(in); if(in.fail()) throw std::runtime_error(std::string(file)+":"+std::to_string(row)+": malformed row"); }
    };
    read("classes.txt",[&](std::istream& in) { std::string role; in>>role; auto& c=classes[size_t(parseRole(role))]; int r=0,g=0,b=0; in>>c.hp>>c.mp>>c.armor>>c.power>>c.speed>>r>>g>>b; c.color=rgb(float(r),float(g),float(b)); if(c.hp<=0||c.mp<0||c.speed<=0) throw std::runtime_error("Invalid class stats"); });
    read("abilities.txt",[&](std::istream& in) { std::string role; int slot=0; in>>role>>slot; if(slot<1||slot>6) throw std::runtime_error("Ability slot outside 1..6"); auto& a=abilities[size_t(parseRole(role))][size_t(slot-1)]; in>>std::quoted(a.name)>>a.effect>>a.cost>>a.cooldown>>a.power>>a.radius>>a.speed>>a.range>>a.duration; if(a.cost<0||a.cooldown<0||a.range<0) throw std::runtime_error("Invalid ability stats"); });
    for(int i=0;i<5;++i) { if(classes[size_t(i)].hp<=0) throw std::runtime_error("Missing class"); for(const auto& a:abilities[size_t(i)]) if(a.name.empty()) throw std::runtime_error("Missing ability"); }
}
void Data::applyClass(Entity& e,Role role) const { const auto& c=classes[size_t(role)]; e.role=role; e.hp=e.maxHp=c.hp; e.mp=e.maxMp=c.mp; e.armor=c.armor; e.power=c.power; e.speed=c.speed; e.color=c.color; e.cooldown.fill(0); }
}
