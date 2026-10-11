#include "net/client.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>
namespace er::net {
Client::Client(const std::string& address,const std::string& name,Role role) {
    auto colon=address.find(':'); std::string host=address.substr(0,colon); int port=colon==std::string::npos?7777:std::stoi(address.substr(colon+1));
    if(port<1||port>65535) throw std::runtime_error("Port outside 1..65535");
    connection=Connection(connectTo(host,uint16_t(port))); std::ostringstream hello; hello<<"HELLO 1 "<<std::quoted(safeText(name,20))<<' '<<int(role); connection.send(hello.str());
}
void Client::chat(const std::string& line,Game& g) {
    if(line.rfind("/invite ",0)==0) connection.send("INVITE "+line.substr(8));
    else if(line=="/accept") connection.send("ACCEPT");
    else if(line=="/who") { for(const auto& e:g.guests) g.chatLog.push_back(std::to_string(e.id)+" "+e.name); g.chatLog.push_back(std::to_string(g.hero.id)+" "+g.hero.name+" (you)"); }
    else { std::ostringstream out; out<<"CHAT "<<std::quoted(safeText(line,180)); connection.send(out.str()); }
}
void Client::input(const Input& in,Game& g) {
    Input local=in; local.keys.clear(); local.click=false; Vec2 direction{};
    std::vector<char> actions;
    for(char k:in.keys) {
        if(g.chatEditing) {
            if(k=='\r'||k=='\n') { if(!g.chatDraft.empty()) chat(g.chatDraft,g); g.chatDraft.clear(); g.chatEditing=false; }
            else if(k==7) { g.chatDraft.clear(); g.chatEditing=false; }
            else if(k==127||k==8) { if(!g.chatDraft.empty()) g.chatDraft.pop_back(); }
            else if(k>=32&&k<127&&g.chatDraft.size()<180) g.chatDraft+=k;
            continue;
        }
        if((k=='\r'||k=='\n')&&!g.inventoryMenu) { g.chatEditing=true; continue; }
        if(k=='w') direction.y-=1; else if(k=='s') direction.y+=1; else if(k=='a') direction.x-=1; else if(k=='d') direction.x+=1;
        else if((k>='1'&&k<='6')||k==' '||k=='e'||k=='r'||k=='b'||k=='\r'||k=='\n') actions.push_back(k);
        else local.keys+=k;
    }
    g.input(local);
    if(length(direction)>0) { direction=normalized(direction); connection.send("MOVE "+std::to_string(direction.x)+" "+std::to_string(direction.y)); }
    if(in.click&&!g.chatEditing) actions.push_back(' ');
    for(char k:actions) {
        if(g.inventoryMenu) { if(k=='\r'||k=='\n') connection.send("EQUIP "+std::to_string(g.inventoryIndex)); continue; }
        if(k=='e') connection.send("LOOT"); else if(k=='r') connection.send("RESPAWN"); else if(k=='b') connection.send("TRAVEL boss");
        else if(g.classMenu&&k>='1'&&k<='5') { connection.send("CLASS "+std::to_string(k-'1')); g.classMenu=false; }
        else if(!g.classMenu) { int slot=k==' '?0:k-'1'; connection.send("CAST "+std::to_string(slot)+" "+std::to_string(g.aim.x)+" "+std::to_string(g.aim.y)+" "+std::to_string(g.hero.target)); }
    }
}
void Client::update(float dt,Game& g) {
    age+=dt; silence+=dt; connection.flush();
    for(const auto& message:connection.receive()) {
        silence=0;
        if(message.rfind("WELCOME ",0)==0) { std::istringstream in(message); std::string tag; uint32_t seed=0; in>>tag>>id>>seed; g.world=World(seed); g.message="Connected. Enter: chat  /who  /invite ID  /accept"; }
        else if(message.rfind("SNAP ",0)==0) { auto s=parseSnapshot(message); if(history.empty()||s.tick>history.back().tick) { history.push_back(std::move(s)); age=0; while(history.size()>8) history.pop_front(); } }
        else if(message.rfind("NOTICE ",0)==0) { std::istringstream in(message.substr(7)); std::string line; in>>std::quoted(line); g.chatLog.push_back(safeText(line,200)); }
    }
    if(connection.closed) throw std::runtime_error("Server disconnected");
    if(silence>10) throw std::runtime_error("Server timed out");
    if(history.empty()||!id) return;
    const auto& latest=history.back(); float targetTime=latest.time+std::min(age,.2f)-.1f;
    const Snapshot* a=&history.front(),*b=a;
    for(const auto& s:history) { if(s.time<=targetTime) a=&s; if(s.time>=targetTime) { b=&s; break; } b=&s; }
    float blend=b->time>a->time?clamp((targetTime-a->time)/(b->time-a->time),0.f,1.f):1;
    g.party.clear(); g.mobs.clear(); g.guests.clear(); g.dragon.alive=false;
    int selected=g.hero.target;
    for(auto e:latest.entities) {
        Vec2 from=e.pos,to=e.pos;
        for(const auto& v:a->entities) if(v.id==e.id) from=v.pos;
        for(const auto& v:b->entities) if(v.id==e.id) to=v.pos;
        e.pos=lerp(from,to,blend);
        if(e.id==id) { e.kind=Kind::Player; g.hero=e; g.hero.target=selected; if(!ready) { g.camera.center=e.pos; g.aim=e.pos+Vec2{7,-3}; ready=true; } }
        else if(e.kind==Kind::Party) g.party.push_back(e);
        else if(e.kind==Kind::Boss) g.dragon=e;
        else if(e.kind==Kind::Mob) g.mobs.push_back(e);
        else g.guests.push_back(e);
    }
    g.time=targetTime; g.fight.active=latest.active; g.fight.phase=latest.phase;
    g.combat.areas=latest.areas; g.combat.projectiles=latest.projectiles; g.combat.particles=latest.particles; g.texts=latest.texts; g.loot=latest.loot;
    g.world.tick(dt); for(auto* e:g.entities()) if(e->alive) g.world.pushGrass(e->pos,2.5f,dt*25);
    g.camera.center=lerp(g.camera.center,g.hero.pos,1-std::exp(-dt*10));
    if(g.chatLog.size()>6) g.chatLog.erase(g.chatLog.begin(),g.chatLog.end()-6);
}
}
