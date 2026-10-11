#include "net/socket.hpp"
#include "net/protocol.hpp"
#include <chrono>
#include <thread>
#include <csignal>
#include <iostream>
#include <sstream>
#include <iomanip>
namespace {
volatile std::sig_atomic_t running=1;
void stop(int) { running=0; }
struct Peer {
    er::net::Connection connection; int id=0,invitedBy=0; er::Vec2 move{}; float moveTTL=0,age=0,credits=120;
    explicit Peer(er::net::Socket s):connection(s) {}
};
void notice(Peer& p,const std::string& text) { std::ostringstream out; out<<"NOTICE "<<std::quoted(text); p.connection.send(out.str()); }
}
int main(int argc,char** argv) { try {
    using namespace er; using namespace er::net;
    int port=7777,ticks=0; uint32_t seed=42; std::string address="127.0.0.1",dataPath;
    for(int i=1;i<argc;++i) { std::string arg=argv[i];
        if(arg=="--port"&&i+1<argc) port=std::stoi(argv[++i]); else if(arg=="--bind"&&i+1<argc) address=argv[++i];
        else if(arg=="--ticks"&&i+1<argc) ticks=std::stoi(argv[++i]); else if(arg=="--seed"&&i+1<argc) seed=uint32_t(std::stoul(argv[++i]));
        else if(arg=="--data"&&i+1<argc) dataPath=argv[++i]; else if(arg=="--help") { std::cout<<"game_server [--bind 127.0.0.1] [--port 7777] [--seed 42] [--ticks N]\n"; return 0; } else throw std::runtime_error("Unknown server option: "+arg);
    }
    if(port<1||port>65535) throw std::runtime_error("Port outside 1..65535");
    Runtime runtime; Connection listener(listenOn(address,uint16_t(port))); Game game(seed,dataPath); game.serverMode=true; game.hero.alive=false; game.guests.reserve(8);
    std::vector<Peer> peers; peers.reserve(8); int nextId=1000; uint64_t tick=0; std::signal(SIGINT,stop); std::signal(SIGTERM,stop);
    auto entity=[&](int id)->Entity* { for(auto& e:game.guests) if(e.id==id) return &e; return nullptr; };
    auto broadcast=[&](const std::string& text) { for(auto& peer:peers) if(peer.id) notice(peer,text); };
    std::cout<<"EternalRPG server "<<address<<":"<<port<<" | simulation 60 Hz | snapshots 20 Hz | seed "<<seed<<std::endl;
    auto next=std::chrono::steady_clock::now();
    while(running&&(!ticks||tick<uint64_t(ticks))) {
        for(int i=0;i<8;++i) { Socket s=::accept(listener.socket,nullptr,nullptr); if(s==invalid) break; nonblocking(s); if(peers.size()>=8) closeSocket(s); else peers.emplace_back(s); }
        for(auto& peer:peers) {
            peer.age+=1.f/60; peer.credits=std::min(120.f,peer.credits+2); peer.moveTTL-=1.f/60;
            for(const auto& command:peer.connection.receive()) {
                if(command.size()>512||peer.credits<1) { peer.connection.close(); break; } peer.credits-=1;
                std::istringstream in(command); std::string op; in>>op;
                if(op=="HELLO"&&!peer.id) {
                    int version=0,role=0; std::string name; in>>version>>std::quoted(name)>>role;
                    if(!in||version!=1||role<0||role>4) { peer.connection.close(); break; }
                    Entity e; e.id=++nextId; e.group=e.id; e.kind=Kind::Remote; e.name=safeText(name,20); if(e.name.empty()) e.name="Traveler"; game.data.applyClass(e,Role(role)); e.pos={float(peers.size()-1)*2,12}; e.home=e.pos;
                    peer.id=e.id; game.guests.push_back(e); peer.connection.send("WELCOME "+std::to_string(e.id)+" "+std::to_string(seed)); broadcast(e.name+" joined ["+std::to_string(e.id)+"]"); continue;
                }
                Entity* e=entity(peer.id); if(!e) continue;
                if(op=="MOVE") { float x=0,y=0; in>>x>>y; if(in&&std::isfinite(x)&&std::isfinite(y)&&std::abs(x)<100&&std::abs(y)<100) { peer.move=normalized({x,y}); peer.moveTTL=.18f; } }
                else if(op=="CAST") { int slot=0,target=0; Vec2 aim; in>>slot>>aim.x>>aim.y>>target; if(in&&slot>=0&&slot<6&&std::isfinite(aim.x)&&std::isfinite(aim.y)&&std::abs(aim.x)<200&&std::abs(aim.y)<200) { auto error=game.combat.cast(*e,slot,aim,target,game.entities(),game.data,game.world,game.texts); if(!error.empty()) notice(peer,error); } }
                else if(op=="CLASS") { int role=-1; in>>role; if(in&&role>=0&&role<5&&e->alive&&e->hurtTime<=0&&!game.fight.active) { game.data.applyClass(*e,Role(role)); for(int id:e->equipment) if(id>=0) { e->power+=game.data.items[size_t(id)].power; e->armor+=game.data.items[size_t(id)].armor; } } }
                else if(op=="LOOT"&&e->alive) { std::swap(game.hero,*e); game.collectLoot(); std::swap(game.hero,*e); }
                else if(op=="EQUIP") { int index=0; in>>index; if(in&&index>=0&&index<int(e->inventory.size())) { std::swap(game.hero,*e); game.inventoryIndex=index; game.equipSelected(); std::swap(game.hero,*e); } }
                else if(op=="RESPAWN"&&!e->alive) { e->alive=true; e->hp=e->maxHp; e->mp=e->maxMp; e->pos={0,12}; e->poison=e->slow=0; e->gold=std::max(0,e->gold-25); e->cooldown.fill(0); }
                else if(op=="TRAVEL"&&e->alive) { std::string where; in>>where; if(where=="boss"&&!game.fight.active) { e->pos={30,-17}; for(size_t i=0;i<game.party.size();++i) game.party[i].pos=e->pos+Vec2{float(i)*4-4,3}; } }
                else if(op=="CHAT") { std::string line; in>>std::quoted(line); if(in) broadcast(e->name+": "+safeText(line,180)); }
                else if(op=="INVITE") { int target=0; in>>target; if(in&&target!=peer.id) for(auto& other:peers) if(other.id==target) { other.invitedBy=peer.id; notice(other,e->name+" invited you. Enter /accept to join."); notice(peer,"Invitation sent."); } }
                else if(op=="ACCEPT"&&peer.invitedBy) { if(auto* inviter=entity(peer.invitedBy)) { e->group=inviter->group; broadcast(e->name+" joined "+inviter->name+"'s party."); } peer.invitedBy=0; }
            }
            if(!peer.id&&peer.age>5) peer.connection.close(); peer.connection.flush();
        }
        for(auto it=peers.begin();it!=peers.end();) {
            if(it->connection.closed) { int id=it->id; game.guests.erase(std::remove_if(game.guests.begin(),game.guests.end(),[&](const auto& e){return e.id==id;}),game.guests.end()); it=peers.erase(it); }
            else ++it;
        }
        if(!game.guests.empty()) {
            for(auto& peer:peers) if(auto* e=entity(peer.id)) { e->velocity=peer.moveTTL>0?peer.move*e->speed:Vec2{}; if(e->alive) e->pos=game.world.move(e->pos,e->velocity*(1.f/60)*(e->slow>0?.35f:1.f)*(game.world.water(e->pos)?.55f:1.f)); }
            game.hero.pos=game.guests.front().pos; game.tick(1.f/60);
            if(tick%3==0) { auto state=snapshot(game,tick); for(auto& peer:peers) if(peer.id) peer.connection.send(state); }
        }
        ++tick; next+=std::chrono::microseconds(16667); auto now=std::chrono::steady_clock::now(); if(next<now-std::chrono::milliseconds(250)) next=now; std::this_thread::sleep_until(next);
    }
    return 0;
} catch(const std::exception& e) { std::cerr<<"game_server: "<<e.what()<<'\n'; return 1; } }
