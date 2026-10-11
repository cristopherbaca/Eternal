#include "net/protocol.hpp"
#include "net/socket.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>
namespace er::net {
std::string snapshot(Game& g,uint64_t tick) {
    std::ostringstream out; out<<std::setprecision(7)<<"SNAP "<<tick<<' '<<g.time<<' '<<g.fight.active<<' '<<g.fight.phase<<'\n';
    for(auto* e:g.entities()) {
        out<<"E "<<e->id<<' '<<std::quoted(safeText(e->name,24))<<' '<<int(e->kind)<<' '<<int(e->role)<<' '<<e->pos.x<<' '<<e->pos.y<<' '<<e->velocity.x<<' '<<e->velocity.y<<' '<<e->hp<<' '<<e->maxHp<<' '<<e->mp<<' '<<e->maxMp<<' '<<e->armor<<' '<<e->power<<' '<<e->speed<<' '<<e->alive<<' '<<e->level<<' '<<e->xp<<' '<<e->gold<<' '<<e->target<<' '<<e->group<<' '<<int(e->color.r)<<' '<<int(e->color.g)<<' '<<int(e->color.b);
        for(float cd:e->cooldown) out<<' '<<cd;
        out<<' '<<e->inventory.size(); for(int item:e->inventory) out<<' '<<item; for(int item:e->equipment) out<<' '<<item; out<<' '<<e->archetype<<'\n';
    }
    for(const auto& a:g.combat.areas) out<<"A "<<a.pos.x<<' '<<a.pos.y<<' '<<a.radius<<' '<<a.life<<' '<<a.delay<<' '<<a.owner<<' '<<a.effect<<' '<<a.direction.x<<' '<<a.direction.y<<'\n';
    for(const auto& p:g.combat.projectiles) out<<"P "<<p.pos.x<<' '<<p.pos.y<<' '<<p.velocity.x<<' '<<p.velocity.y<<' '<<p.life<<' '<<p.owner<<' '<<p.effect<<' '<<int(p.color.r)<<' '<<int(p.color.g)<<' '<<int(p.color.b)<<'\n';
    size_t stride=std::max(size_t(1),g.combat.particles.size()/180);
    for(size_t i=0;i<g.combat.particles.size();i+=stride) { const auto& p=g.combat.particles[i]; out<<"V "<<p.pos.x<<' '<<p.pos.y<<' '<<p.height<<' '<<p.life<<' '<<p.maxLife<<' '<<uint32_t(p.glyph)<<' '<<int(p.color.r)<<' '<<int(p.color.g)<<' '<<int(p.color.b)<<'\n'; }
    for(const auto& t:g.texts) out<<"T "<<t.pos.x<<' '<<t.pos.y<<' '<<std::quoted(t.text)<<' '<<t.life<<' '<<t.maxLife<<' '<<int(t.color.r)<<' '<<int(t.color.g)<<' '<<int(t.color.b)<<'\n';
    for(const auto& l:g.loot) out<<"L "<<l.pos.x<<' '<<l.pos.y<<' '<<l.item<<' '<<l.gold<<'\n';
    return out.str();
}
Snapshot parseSnapshot(const std::string& message) {
    Snapshot s; std::istringstream in(message); std::string tag; in>>tag>>s.tick>>s.time>>s.active>>s.phase;
    if(tag!="SNAP"||!in||!std::isfinite(s.time)) throw std::runtime_error("Invalid snapshot header");
    int records=0;
    while(in>>tag) {
        if(++records>1200) throw std::runtime_error("Too many snapshot records");
        if(tag=="E") {
            Entity e; int kind=0,role=0,r=0,g=0,b=0,count=0;
            in>>e.id>>std::quoted(e.name)>>kind>>role>>e.pos.x>>e.pos.y>>e.velocity.x>>e.velocity.y>>e.hp>>e.maxHp>>e.mp>>e.maxMp>>e.armor>>e.power>>e.speed>>e.alive>>e.level>>e.xp>>e.gold>>e.target>>e.group>>r>>g>>b;
            if(kind<0||kind>4||role<0||role>4||!std::isfinite(e.pos.x)||!std::isfinite(e.pos.y)||std::abs(e.pos.x)>200||std::abs(e.pos.y)>200||e.hp<0||e.maxHp<=0||!std::isfinite(e.maxHp)) throw std::runtime_error("Invalid network entity");
            e.kind=Kind(kind); e.role=Role(role); e.color=rgb(float(r),float(g),float(b)); e.name=safeText(e.name,24);
            for(float& cd:e.cooldown) in>>cd;
            in>>count; if(count<0||count>24) throw std::runtime_error("Invalid network inventory");
            for(int i=0;i<count;++i) { int id=0; in>>id; if(id<0||id>4) throw std::runtime_error("Invalid network item"); e.inventory.push_back(id); }
            for(int& id:e.equipment) { in>>id; if(id < -1||id>4) throw std::runtime_error("Invalid equipment"); }
            in>>e.archetype; if(e.archetype<0||e.archetype>2) throw std::runtime_error("Invalid enemy archetype");
            s.entities.push_back(e);
        } else if(tag=="A") { Area a; in>>a.pos.x>>a.pos.y>>a.radius>>a.life>>a.delay>>a.owner>>a.effect>>a.direction.x>>a.direction.y; if(!std::isfinite(a.radius)||a.radius<0||a.radius>50) throw std::runtime_error("Invalid area"); s.areas.push_back(a); }
        else if(tag=="P") { Projectile p; int r=0,g=0,b=0; in>>p.pos.x>>p.pos.y>>p.velocity.x>>p.velocity.y>>p.life>>p.owner>>p.effect>>r>>g>>b; p.color=rgb(float(r),float(g),float(b)); s.projectiles.push_back(p); }
        else if(tag=="V") { Particle p; int r=0,g=0,b=0; uint32_t glyph=0; in>>p.pos.x>>p.pos.y>>p.height>>p.life>>p.maxLife>>glyph>>r>>g>>b; p.glyph=glyph>=32&&glyph<127?char32_t(glyph):U'.'; p.color=rgb(float(r),float(g),float(b)); s.particles.push_back(p); }
        else if(tag=="T") { FloatingText t; int r=0,g=0,b=0; in>>t.pos.x>>t.pos.y>>std::quoted(t.text)>>t.life>>t.maxLife>>r>>g>>b; t.text=safeText(t.text,32); t.color=rgb(float(r),float(g),float(b)); s.texts.push_back(t); }
        else if(tag=="L") { Loot l; in>>l.pos.x>>l.pos.y>>l.item>>l.gold; if(l.item<0||l.item>4) throw std::runtime_error("Invalid loot"); s.loot.push_back(l); }
        else throw std::runtime_error("Unknown snapshot record");
        if(!in) throw std::runtime_error("Truncated snapshot");
    } return s;
}
}
