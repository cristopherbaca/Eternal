#include "game.hpp"
#include "net/client.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <thread>

int main(int argc,char** argv) {
    try {
        bool headless=false,demo=false;
        int frames=0,w=200,h=60,fps=60;
        uint32_t seed=42;
        std::string dump,stats,scene="grass",className="Mage",dataPath,connect,name="Senje";
        for(int i=1;i<argc;++i) {
            std::string arg=argv[i];
            auto value=[&]() -> std::string { if(i+1>=argc) throw std::runtime_error("Missing value for "+arg); return argv[++i]; };
            if(arg=="--headless"||arg=="--benchmark") headless=true;
            else if(arg=="--demo") demo=true;
            else if(arg=="--frames") frames=std::stoi(value());
            else if(arg=="--fps") fps=std::stoi(value());
            else if(arg=="--scene") scene=value();
            else if(arg=="--connect") connect=value();
            else if(arg=="--name") name=er::net::safeText(value(),20);
            else if(arg=="--seed") seed=uint32_t(std::stoul(value()));
            else if(arg=="--class") className=value();
            else if(arg=="--data") dataPath=value();
            else if(arg=="--dump") dump=value();
            else if(arg=="--stats") stats=value();
            else if(arg=="--width") w=std::stoi(value());
            else if(arg=="--height") h=std::stoi(value());
            else if(arg=="--help") {
                std::cout<<"EternalRPG - truecolor terminal RPG\n"
                    "WASD move, mouse/arrows aim, click/Space primary, 1-6 abilities\n"
                    "Tab target, C class, I inventory, E loot, B boss, R respawn, H help, Q quit\n"
                    "--scene grass|forest|ruins|water|boss  --class Warrior|Mage|Archer|Cleric|Rogue\n"
                    "--seed N  --name NAME  --fps 60  --data DIR  --demo\n"
                    "--connect HOST:PORT (Enter: chat, /who, /invite ID, /accept)\n"
                    "--headless --frames N --width 200 --height 60 --dump frame.json --stats metrics.json\n"
                    "--benchmark implies --headless; headless defaults to 600 deterministic ticks.\n";
                return 0;
            } else throw std::runtime_error("Unknown option: "+arg+" (see --help)");
        }
        if(frames<0||frames>10000000||w<1||w>500||h<1||h>180||fps<20||fps>240)
            throw std::runtime_error("Invalid dimensions/frame limit/FPS (width 1..500, height 1..180, FPS 20..240)");
        if(scene!="grass"&&scene!="forest"&&scene!="ruins"&&scene!="water"&&scene!="boss")
            throw std::runtime_error("Unknown scene: "+scene);
        if(headless&&!frames) frames=600;
        er::Terminal terminal(!headless);
        er::Framebuffer frame(w,h);
        er::Game game(seed,dataPath);
        game.data.applyClass(game.hero,er::parseRole(className));
        game.setScene(scene);
        game.hero.name=name;
        std::unique_ptr<er::net::Client> client;
        if(!connect.empty()) client=std::make_unique<er::net::Client>(connect,name,game.hero.role);
        using Clock=std::chrono::steady_clock;
        auto start=Clock::now(),last=start,lastPresented=start,nextPresentation=start;
        float frameMs=1000.f/fps,accumulator=0,nextShot=0,nextArea=1.5f;
        int count=0;
        uint64_t bytes=0;
        std::vector<double> workTimes;
        auto period=std::chrono::microseconds(1000000/fps);
        auto inputPeriod=std::chrono::microseconds(1000000/std::max(60,fps));
        while(er::Terminal::running()&&(!frames||count<frames)) {
            auto begin=Clock::now();
            auto input=terminal.poll();
            if(client) client->input(input,game); else game.input(input);
            if(game.quit) break;
            if(demo) {
                er::Input action;
                if(game.time>=nextShot) { action.keys+='1'; nextShot=game.time+.75f; }
                if(game.time>=nextArea) { action.keys+='2'; nextArea=game.time+6; }
                if(game.hero.mp<30) action.keys+='6';
                if(game.fight.active) {
                    game.aim=game.dragon.pos;
                    for(const auto& area:game.combat.areas) if(area.owner>=100&&er::distance(game.hero.pos,area.pos)<area.radius+1) {
                        auto d=er::normalized(game.hero.pos-area.pos);
                        if(er::length(d)<.1f) d={1,0};
                        if(std::abs(d.x)>.4f) action.keys+=d.x>0?'d':'a';
                        if(std::abs(d.y)>.4f) action.keys+=d.y>0?'s':'w';
                        break;
                    }
                } else {
                    float best=25;
                    for(const auto& e:game.mobs) if(e.alive&&er::distance(e.pos,game.hero.pos)<best) {
                        best=er::distance(e.pos,game.hero.pos); game.aim=e.pos; game.hero.target=e.id;
                    }
                }
                if(client) client->input(action,game); else game.input(action);
            }
            if(!headless) {
                auto [x,y]=terminal.size(); x=er::clamp(x,1,500); y=er::clamp(y,1,180);
                if(x!=frame.width||y!=frame.height) frame.resize(x,y);
            }
            float elapsed=std::min(.25f,std::chrono::duration<float>(begin-last).count());
            last=begin;
            if(client) client->update(elapsed,game);
            else {
                accumulator+=headless?1.f/60:elapsed;
                while(accumulator>=1.f/60) { game.tick(1.f/60); accumulator-=1.f/60; }
            }
            if(headless||(begin>=nextPresentation&&terminal.frameReady())) {
                if(count>0&&!headless) {
                    float interval=std::chrono::duration<float,std::milli>(begin-lastPresented).count();
                    frameMs=frameMs*.9f+interval*.1f;
                }
                game.render(frame,frameMs);
                auto output=frame.diff();
                bytes+=output.size();
                terminal.present(output);
                lastPresented=begin;
                // Keep the target cadence, but never burst queued old frames.
                nextPresentation=std::max(nextPresentation+period,Clock::now());
                ++count;
                workTimes.push_back(std::chrono::duration<double,std::milli>(Clock::now()-begin).count());
            }
            if(!headless||client) {
                auto deadline=begin+inputPeriod;
                std::this_thread::sleep_until(deadline-std::chrono::microseconds(300));
                while(Clock::now()<deadline) std::this_thread::yield();
            }
        }
        double seconds=std::chrono::duration<double>(Clock::now()-start).count();
        double mean=workTimes.empty()?0:std::accumulate(workTimes.begin(),workTimes.end(),0.0)/workTimes.size();
        std::sort(workTimes.begin(),workTimes.end());
        double p95=workTimes.empty()?0:workTimes[std::min(workTimes.size()-1,size_t(workTimes.size()*.95))];
        if(!dump.empty()) frame.dump(dump);
        if(!stats.empty()) {
            std::ofstream out(stats);
            if(!out) throw std::runtime_error("Cannot write metrics: "+stats);
            out<<std::fixed<<std::setprecision(3)<<"{\"frames\":"<<count<<",\"seconds\":"<<seconds
                <<",\"fps\":"<<(seconds>0?count/seconds:0)<<",\"work_mean_ms\":"<<mean<<",\"work_p95_ms\":"<<p95
                <<",\"ansi_bytes\":"<<bytes<<",\"width\":"<<frame.width<<",\"height\":"<<frame.height
                <<",\"hero_hp\":"<<game.hero.hp<<",\"boss_hp\":"<<game.dragon.hp<<",\"boss_won\":"<<(game.fight.won?"true":"false")<<"}\n";
        }
        if(headless) std::cout<<count<<" frames in "<<seconds<<" s | work mean "<<mean<<" ms, p95 "<<p95<<" ms | ANSI "<<bytes<<" bytes\n";
    } catch(const std::exception& e) {
        std::cerr<<"EternalRPG: "<<e.what()<<'\n';
        return 1;
    }
}
