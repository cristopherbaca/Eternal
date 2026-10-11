#include "game.hpp"
#ifdef __APPLE__
#include "platform/window.hpp"
#endif
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <thread>

int main(int argc,char** argv) {
    try {
        bool headless=false,demo=false,native=false,normal=false,fpsExplicit=false,classExplicit=false,noMods=false,listMods=false,positionSet=false;
        er::Vec2 startPosition{};
        int frames=0,w=200,h=60,fps=60;
        uint64_t seed=42;
        std::string dump,stats,scene="grass",className="Mage",dataPath,modsPath,name="Senje";
        std::vector<std::string> modIds;
        for(int i=1;i<argc;++i) {
            std::string arg=argv[i];
            auto value=[&]() -> std::string { if(i+1>=argc) throw std::runtime_error("Missing value for "+arg); return argv[++i]; };
            if(arg=="--headless"||arg=="--benchmark") headless=true;
            else if(arg=="--window") native=true;
            else if(arg=="--demo") demo=true;
            else if(arg=="--frames") frames=std::stoi(value());
            else if(arg=="--fps") { fps=std::stoi(value()); fpsExplicit=true; }
            else if(arg=="--scene") scene=value();
            else if(arg=="--normal") normal=true;
            else if(arg=="--name") name=value().substr(0,20);
            else if(arg=="--seed") seed=std::stoull(value());
            else if(arg=="--class") { className=value(); classExplicit=true; }
            else if(arg=="--position") { startPosition.x=std::stof(value()); startPosition.y=std::stof(value()); positionSet=true; }
            else if(arg=="--mod") modIds.push_back(value());
            else if(arg=="--mods-dir") modsPath=value();
            else if(arg=="--no-mods") noMods=true;
            else if(arg=="--list-mods") listMods=true;
            else if(arg=="--data") dataPath=value();
            else if(arg=="--dump") dump=value();
            else if(arg=="--stats") stats=value();
            else if(arg=="--width") w=std::stoi(value());
            else if(arg=="--height") h=std::stoi(value());
            else if(arg=="--help") {
                std::cout<<"EternalRPG - truecolor terminal RPG\n"
                    "WASD move, mouse/arrows aim, click/Space primary, 1-6 abilities\n"
                    "Tab target, C class, I inventory, E loot, B boss, R respawn, M map, Esc/P pause, Q quit\n"
                    "--scene grass|forest|ruins|water|boss  --class Warrior|Mage|Archer|Cleric|Rogue\n"
                    "--window (macOS Metal ASCII window)\n"
                    "--seed N  --name NAME  --fps 60  --data DIR  --demo\n"
                    "--mod ID --mods-dir DIR --no-mods --list-mods --position X Y\n"
                    "--normal disables powerful testing mode\n"
                    "--headless --frames N --width 200 --height 60 --dump frame.json --stats metrics.json\n"
                    "--benchmark implies --headless; headless defaults to 600 deterministic ticks.\n";
                return 0;
            } else throw std::runtime_error("Unknown option: "+arg+" (see --help)");
        }
        if(frames<0||frames>10000000||w<1||w>500||h<1||h>180||fps<20||fps>240)
            throw std::runtime_error("Invalid dimensions/frame limit/FPS (width 1..500, height 1..180, FPS 20..240)");
        if(scene!="grass"&&scene!="forest"&&scene!="ruins"&&scene!="water"&&scene!="boss")
            throw std::runtime_error("Unknown scene: "+scene);
        if(positionSet&&(!std::isfinite(startPosition.x)||!std::isfinite(startPosition.y)||std::abs(startPosition.x)>999000||std::abs(startPosition.y)>999000))
            throw std::runtime_error("--position must be finite and within +/-999000 world units");
        auto mods=er::loadMods(modsPath,modIds,noMods);
        if(listMods) {
            if(mods.names.empty()) std::cout<<"No active mods\n";
            for(const auto& name:mods.names) std::cout<<name<<'\n';
            return 0;
        }
        if(headless&&!frames) frames=600;
        if(native&&headless) throw std::runtime_error("--window and --headless cannot be combined");
#ifdef __APPLE__
        std::unique_ptr<er::Window> window;
        if(native) window=std::make_unique<er::Window>();
#else
        if(native) throw std::runtime_error("--window currently requires macOS");
#endif
        er::Terminal terminal(!headless&&!native);
        er::Framebuffer frame(w,h),interface(160,50);
        er::Game game(seed,dataPath,mods);
        game.data.applyClass(game.hero,er::parseRole(className));
        game.setScene(scene);
        if(positionSet) {
            game.hero.pos=startPosition; game.camera.center=startPosition; game.aim=startPosition+er::Vec2{5,0};
            game.world.focus(startPosition); game.streamEnemies();
        }
        name.erase(std::remove_if(name.begin(),name.end(),[](unsigned char c){return c<32||c>126;}),name.end());
        game.hero.name=name.empty()?"Senje":name;
        game.presentationFps=fps;
        game.setTesting(!normal);
        if(!headless&&!demo) game.loadSettings();
        if(normal) game.setTesting(false);
        if(fpsExplicit) game.presentationFps=fps;
        if(classExplicit) game.data.applyClass(game.hero,er::parseRole(className));
        game.refreshSaves();
        fps=game.presentationFps;
        game.screen=(headless||demo)?er::Screen::Playing:er::Screen::Title;
        using Clock=std::chrono::steady_clock;
        auto start=Clock::now(),last=start,lastPresented=start,nextPresentation=start,nextInput=start;
        float frameMs=1000.f/fps,accumulator=0,nextShot=0,nextArea=1.5f;
        int count=0;
        uint64_t bytes=0;
        std::vector<double> workTimes;
        auto period=std::chrono::microseconds(1000000/fps);
        auto inputPeriod=std::chrono::microseconds(1000000/std::max(60,fps));
        while(er::Terminal::running()&&(!frames||count<frames)) {
            auto begin=Clock::now();
            auto input=terminal.poll();
#ifdef __APPLE__
            if(window) input=window->poll(game.screen!=er::Screen::Playing,game.editingField);
#endif
            game.input(input);
            if(game.quit) break;
            fps=game.presentationFps;
            period=std::chrono::microseconds(1000000/fps);
            inputPeriod=std::chrono::microseconds(1000000/std::max(60,fps));
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
                game.input(action);
            }
            if(!headless) {
                auto dimensions=terminal.size();
#ifdef __APPLE__
                if(window) {
                    dimensions=window->size(game.detail);
                    auto [uw,uh]=window->interfaceSize();
                    if(interface.width!=uw||interface.height!=uh) interface.resize(uw,uh);
                }
#endif
                auto [x,y]=dimensions; x=er::clamp(x,1,500); y=er::clamp(y,1,180);
                if(x!=frame.width||y!=frame.height) frame.resize(x,y);
            }
            float elapsed=std::min(.25f,std::chrono::duration<float>(begin-last).count());
            last=begin;
            {
                accumulator+=headless?1.f/60:elapsed;
                while(accumulator>=1.f/60) { game.tick(1.f/60); accumulator-=1.f/60; }
            }
            bool ready=terminal.frameReady();
#ifdef __APPLE__
            if(window) ready=window->ready();
#endif
            if(headless||(begin>=nextPresentation&&ready)) {
#ifdef __APPLE__
                game.render(frame,frameMs,window?&interface:nullptr);
#else
                game.render(frame,frameMs);
#endif
                bool submitted=true;
#ifdef __APPLE__
                if(window) submitted=window->present(frame,interface);
                else
#endif
                {
                    auto output=frame.diff();
                    bytes+=output.size();
                    terminal.present(output);
                }
                if(submitted) {
                    if(count>0&&!headless) {
                        float interval=std::chrono::duration<float,std::milli>(begin-lastPresented).count();
                        frameMs=frameMs*.9f+interval*.1f;
                    }
                    lastPresented=begin;
                }
                // Keep the target cadence, but never burst queued old frames.
                nextPresentation=std::max(nextPresentation+period,Clock::now());
                if(submitted) ++count;
                workTimes.push_back(std::chrono::duration<double,std::milli>(Clock::now()-begin).count());
            }
            if(!headless) {
                nextInput+=inputPeriod;
                if(nextInput<Clock::now()) nextInput=Clock::now();
                auto deadline=nextInput;
                std::this_thread::sleep_until(deadline-std::chrono::microseconds(300));
                while(Clock::now()<deadline) std::this_thread::yield();
            }
        }
        if(!headless&&!demo&&game.activeSave>=0&&!game.saveAdventure()) std::cerr<<"Could not save adventure: "<<game.menuNotice<<"\n";
        if(!headless&&!demo&&!game.saveSettings()) std::cerr<<"Could not save settings in the working directory\n";
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
