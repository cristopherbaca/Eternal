#include "renderer/framebuffer.hpp"
#include "platform/terminal.hpp"
#include "game.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <cstdio>
int main(int argc,char** argv) {
    try {
        bool headless=false,demo=false; int frames=0,w=200,h=60; uint32_t seed=42; std::string dump,scene="grass",className="Mage",dataPath;
        for(int i=1;i<argc;++i) { std::string a=argv[i];
            if(a=="--headless") headless=true; else if(a=="--frames"&&i+1<argc) frames=std::stoi(argv[++i]);
            else if(a=="--scene"&&i+1<argc) scene=argv[++i];
            else if(a=="--demo") demo=true;
            else if(a=="--seed"&&i+1<argc) seed=uint32_t(std::stoul(argv[++i]));
            else if(a=="--class"&&i+1<argc) className=argv[++i];
            else if(a=="--data"&&i+1<argc) dataPath=argv[++i];
            else if(a=="--dump"&&i+1<argc) dump=argv[++i]; else if(a=="--width"&&i+1<argc) w=std::stoi(argv[++i]); else if(a=="--height"&&i+1<argc) h=std::stoi(argv[++i]);
            else if(a=="--help") { std::cout<<"EternalRPG | Q quit. --headless --frames N --dump frame.json\n"; return 0; }
        }
        er::Terminal terminal(!headless); er::Framebuffer frame(w,h); er::Game game(seed,dataPath); game.data.applyClass(game.hero,er::parseRole(className));
        game.setScene(scene);
        using Clock=std::chrono::steady_clock; auto start=Clock::now(),last=start; float ms=16.67f,accumulator=0; int count=0;
        while(er::Terminal::running()&&(!frames||count<frames)) {
            auto begin=Clock::now(); auto input=terminal.poll(); game.input(input); if(game.quit) break;
            if(demo) { er::Input action; if(count%45==0) action.keys+='1'; if(count%300==90) action.keys+='2'; game.input(action); }
            if(!headless) { auto [x,y]=terminal.size(); if(x!=frame.width||y!=frame.height) frame.resize(x,y); }
            accumulator+=headless?1.f/60:std::min(.25f,std::chrono::duration<float>(begin-last).count()); last=begin;
            while(accumulator>=1.f/60) { game.tick(1.f/60); accumulator-=1.f/60; }
            game.render(frame,ms); terminal.write(frame.diff()); ++count;
            if(!headless) std::this_thread::sleep_until(begin+std::chrono::microseconds(16667)); ms=std::chrono::duration<float,std::milli>(Clock::now()-begin).count();
        }
        if(!dump.empty()) frame.dump(dump); if(headless) std::cout<<count<<" frames in "<<std::chrono::duration<double>(Clock::now()-start).count()<<" s\n";
    } catch(const std::exception& e) { std::cerr<<"EternalRPG: "<<e.what()<<'\n'; return 1; }
}
