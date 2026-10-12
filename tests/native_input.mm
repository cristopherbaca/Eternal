#include "platform/window.hpp"
#include "game.hpp"
#import <AppKit/AppKit.h>
#include <iostream>
#include <cstring>
#include <stdexcept>
static void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int main(int argc,char** argv) { @autoreleasepool { try {
    er::Window window;
    er::Game game; game.screen=er::Screen::Title;
    er::Framebuffer world(320,100),ui(160,50);
    game.render(world,16,&ui); window.present(world,ui);
    // Exercise GPU conversion and reallocations without depending on desktop focus.
    if(argc>1&&std::strcmp(argv[1],"--renderer-only")==0) {
        er::Terrain3D mesh; mesh.update(game.world,game.hero.pos);
        er::Camera camera; camera.perspective=true; camera.firstPerson=true;
        camera.terrain=&game.world; camera.center=game.hero.pos;
        auto hud=window.interfaceSize();
        check(window.size(1).first>hud.first,"world detail is finer than HUD text");
        for(int detail:{0,2,1,0}) {
            auto dimensions=window.size(detail); world.resize(dimensions.first,dimensions.second);
            camera.width=world.width; camera.height=world.height;
            while(!window.ready()) [NSThread sleepForTimeInterval:.001];
            check(window.present(world,ui,&mesh,&camera),"density change submits GPU conversion");
            while(!window.ready()) [NSThread sleepForTimeInterval:.001];
            check(window.gpuMilliseconds()>0,"ASCII conversion completes successfully");
            check(window.interfaceSize()==hud,"world density preserves HUD size");
        }
        window.raw3D(true);
        check(window.present(world,ui,&mesh,&camera),"raw preview after density changes");
        while(!window.ready()) [NSThread sleepForTimeInterval:.001];
        check(window.present(world,ui),"2D fallback after GPU conversion");
        while(!window.ready()) [NSThread sleepForTimeInterval:.001];
        check(window.gpuMilliseconds()>0,"fallback completes successfully");
        std::cout<<"PASS: all ASCII densities, buffer resize, independent HUD, raw view and 2D fallback\n";
        return 0;
    }
    for(int i=0;i<30;++i) { window.poll(true); [NSThread sleepForTimeInterval:.01]; }
    NSWindow* native=[NSApp mainWindow];
    if(!native) for(NSWindow* candidate in [NSApp windows]) if([candidate.title hasPrefix:@"EternalRPG"]) { native=candidate; break; }
    [native makeMainWindow]; [native makeKeyAndOrderFront:nil];
    check(native!=nil,"game has a main window");
    check(native.firstResponder==native.contentView,"game view owns keyboard focus");
    auto key=[&](NSString* text,unsigned short code,bool down) {
        NSEvent* event=[NSEvent keyEventWithType:down?NSEventTypeKeyDown:NSEventTypeKeyUp
            location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:native.windowNumber
            context:nil characters:text charactersIgnoringModifiers:text isARepeat:NO keyCode:code];
        [NSApp postEvent:event atStart:NO];
    };
    key(@"\r",36,true); auto input=window.poll(true); game.input(input);
    check(game.screen==er::Screen::Create,"native Enter opens world creation");
    if(const char* path=std::getenv("ER_CREATE_CAPTURE")) {
        while(!window.ready()) [NSThread sleepForTimeInterval:.001];
        setenv("ER_CAPTURE",path,1); game.render(world,16,&ui); window.present(world,ui);
    }
    game.menuIndex=1; game.editingField=true;
    key(@"Q",12,true); game.input(window.poll(true,true));
    key(@"w",13,true); game.input(window.poll(true,true));
    key(@"p",35,true); game.input(window.poll(true,true));
    check(game.draftName=="Qwp"&&!game.quit,"name entry preserves case and does not trigger controls");
    key(@"w",13,false); window.poll(true,true);
    game.editingField=false; game.screen=er::Screen::Playing;
    key(@"w",13,true); key(@"d",2,true); input=window.poll(); game.input(input);
    check(game.velocity.x>0&&game.velocity.y<0,"native held WD moves diagonally");
    key(@"w",13,false); input=window.poll(); game.input(input);
    check(game.velocity.x>0&&game.velocity.y==0,"native key release keeps the remaining direction");
    key(@"d",2,false); game.input(window.poll());
    check(er::length(game.velocity)==0,"native release stops movement");
    key(@"p",35,true); game.input(window.poll());
    check(game.screen==er::Screen::Paused,"native P opens pause menu");
    game.render(world,16,&ui);
    NSSize size=native.contentView.bounds.size;
    NSPoint point=NSMakePoint(44.5/160*size.width,(1-15.5/50)*size.height);
    NSEvent* click=[NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:point
        modifierFlags:0 timestamp:0 windowNumber:native.windowNumber context:nil eventNumber:1 clickCount:1 pressure:1];
    [NSApp postEvent:click atStart:NO];
    input=window.poll(true);
    check(input.click&&input.mouseX==44&&input.mouseY==15,"native click uses event position in UI coordinates");
    game.input(input); check(game.screen==er::Screen::Playing,"native click resumes from scaled pause menu");
    [NSApp postEvent:click atStart:NO]; input=window.poll(false);
    check(input.click&&input.mouseX==89&&input.mouseY==31,"gameplay mouse uses world coordinates");
    game.input(input); game.tick(1.f/60);
    check(game.hero.cooldown[0]>0,"native mouse click casts an ability");
    key(@"w",13,true); window.poll(); [native resignKeyWindow];
    check(window.poll().movementKeys.empty(),"losing focus clears held movement");
    [native makeKeyWindow];
    game.atlas.focus(game.hero.pos); game.population.tick(game.atlas,game.world,game.time,0,false);
    check(!game.population.nearby.empty(),"native scene has residents");
    game.hero.pos=game.population.nearby.front().pos; game.camera.center=game.hero.pos; game.tick(1.f/60);
    auto capture=[&](const char* variable) {
        if(const char* path=std::getenv(variable)) {
            while(!window.ready()) [NSThread sleepForTimeInterval:.001];
            setenv("ER_CAPTURE",path,1); game.render(world,16,&ui); window.present(world,ui);
        }
    };
    capture("ER_LIVING_CAPTURE");
    key(@"y",16,true); game.input(window.poll());
    check(game.screen==er::Screen::Talk,"native Y opens resident dialogue");
    game.menuIndex=3; key(@"\r",36,true); game.input(window.poll(true));
    check(!game.quests.empty(),"native dialogue accepts a generated quest");
    capture("ER_TALK_CAPTURE");
    key(@"\x1b",53,true); game.input(window.poll(true));
    key(@"u",32,true); game.input(window.poll());
    check(game.screen==er::Screen::Progress,"native U opens mastery and crafting");
    capture("ER_PROGRESS_CAPTURE");
    game.screen=er::Screen::Playing; game.camera.perspective=true; game.camera.terrain=&game.world;
    world.resize(160,50); game.render(world,16,&ui);
    er::Terrain3D mesh; mesh.update(game.world,game.hero.pos);
    while(!window.ready()) [NSThread sleepForTimeInterval:.001];
    check(window.present(world,ui,&mesh,&game.camera),"native 3D terrain submits");
    while(!window.ready()) [NSThread sleepForTimeInterval:.001];
    check(window.terrainMilliseconds()>0,"offscreen terrain pass completes on GPU");
    key([NSString stringWithFormat:@"%C",(unichar)NSF8FunctionKey],100,true); window.poll();
    check(window.raw3D(),"F8 toggles raw 3D preview");
    key([NSString stringWithFormat:@"%C",(unichar)NSF9FunctionKey],101,true); window.poll();
    check(window.debug3D(),"F9 toggles renderer diagnostics");
    auto p3=game.camera.project(game.hero.pos+er::Vec2{3,1});
    NSPoint aimPoint=NSMakePoint((p3.x+.5f)/world.width*size.width,(1-(p3.y+.5f)/world.height)*size.height);
    NSEvent* aimClick=[NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:aimPoint modifierFlags:0 timestamp:0 windowNumber:native.windowNumber context:nil eventNumber:2 clickCount:1 pressure:1];
    [NSApp postEvent:aimClick atStart:NO]; game.hero.cooldown[0]=0; game.hero.mp=game.hero.maxMp;
    game.input(window.poll()); game.tick(1.f/60);
    check(game.hero.cooldown[0]>0&&er::distance(game.aim,game.hero.pos)<10,"native perspective ray aiming still casts");
    game.camera.firstPerson=true; game.camera.center=game.hero.pos;
    auto eye=game.camera.view().eye;
    check(std::abs(eye[0]-game.hero.pos.x)<.001f&&std::abs(eye[1]-game.hero.pos.y)<.001f,"first-person eye tracks the player");
    [NSApp activateIgnoringOtherApps:YES]; [native makeKeyAndOrderFront:nil];
    [[NSRunningApplication currentApplication] activateWithOptions:0];
    for(int i=0;i<100&&!native.keyWindow;++i) { window.poll(true); [native makeKeyWindow]; [NSThread sleepForTimeInterval:.01]; }
    auto locked=window.poll(false,false,true);

    if(native.keyWindow) check(window.mouseCaptured()&&locked.mouseX==world.width/2&&locked.mouseY==world.height/2,"captured first-person mouse aims through center crosshair");
    else check(!window.mouseCaptured(),"inactive game never captures the desktop mouse");
    auto beforeLook=game.camera.lookPitch; locked.orbitX=60; locked.orbitY=-30; game.input(locked);
    check(game.camera.lookPitch<beforeLook&&game.camera.yaw>0,"first-person mouse look changes yaw and pitch");
    key(@"\x1b",53,true); game.input(window.poll(false,false,true)); window.poll(true,false,true);
    check(game.screen==er::Screen::Paused&&!window.mouseCaptured(),"Esc pauses and releases first-person mouse capture");
    std::cout<<"PASS: native focus, Enter, diagonal holds, key release, pause, menu click, mouse aim/cast, Metal 3D, raw toggle and diagnostics\n";
    return 0;
} catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; } } }
