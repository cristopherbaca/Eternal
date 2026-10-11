#include "renderer/framebuffer.hpp"
#include "platform/terminal.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <cstdio>
int main(int argc,char** argv) {
    try {
        bool headless=false; int frames=0,w=200,h=60; std::string dump;
        for(int i=1;i<argc;++i) { std::string a=argv[i];
            if(a=="--headless") headless=true; else if(a=="--frames"&&i+1<argc) frames=std::stoi(argv[++i]);
            else if(a=="--dump"&&i+1<argc) dump=argv[++i]; else if(a=="--width"&&i+1<argc) w=std::stoi(argv[++i]); else if(a=="--height"&&i+1<argc) h=std::stoi(argv[++i]);
            else if(a=="--help") { std::cout<<"EternalRPG | Q quit. --headless --frames N --dump frame.json\n"; return 0; }
        }
        er::Terminal terminal(!headless); er::Framebuffer frame(w,h); using Clock=std::chrono::steady_clock; auto start=Clock::now(); float ms=16.67f; int count=0;
        while(er::Terminal::running()&&(!frames||count<frames)) {
            auto begin=Clock::now(); auto input=terminal.poll(); if(input.keys.find('q')!=std::string::npos) break;
            if(!headless) { auto [x,y]=terminal.size(); if(x!=frame.width||y!=frame.height) frame.resize(x,y); }
            frame.clear();
            for(int y=5;y<frame.height-4;++y) for(int x=3;x<frame.width-3;++x) { float wave=0.5f+0.5f*std::sin(x*0.08f+y*0.2f+count/60.f); frame.put(x,y,U" .:-=+*#%@"[int(wave*9)],er::mix({25,59,64},{144,214,187},wave)); }
            frame.text(3,1,"ETERNAL / terminal renderer"); char label[80]; std::snprintf(label,sizeof(label),"FPS %.0f - Frame %.1f ms",1000.f/std::max(ms,0.01f),ms); frame.text(frame.width-31,1,label);
            frame.text(3,frame.height-2,"24-bit color / differential ANSI / Q quit"); terminal.write(frame.diff()); ++count;
            if(!headless) std::this_thread::sleep_until(begin+std::chrono::microseconds(16667)); ms=std::chrono::duration<float,std::milli>(Clock::now()-begin).count();
        }
        if(!dump.empty()) frame.dump(dump); if(headless) std::cout<<count<<" frames in "<<std::chrono::duration<double>(Clock::now()-start).count()<<" s\n";
    } catch(const std::exception& e) { std::cerr<<"EternalRPG: "<<e.what()<<'\n'; return 1; }
}
