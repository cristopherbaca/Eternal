#include "platform/window.hpp"
#include "game.hpp"
#import <AppKit/AppKit.h>
#include <iostream>
#include <stdexcept>
static void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int main() { @autoreleasepool { try {
    er::Window window;
    er::Game game; game.screen=er::Screen::Title;
    er::Framebuffer world(320,100),ui(160,50);
    game.render(world,16,&ui); window.present(world,ui);
    for(int i=0;i<30;++i) { window.poll(true); [NSThread sleepForTimeInterval:.01]; }
    NSWindow* native=[NSApp mainWindow];
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
    std::cout<<"PASS: native focus, Enter, diagonal holds, key release, pause, menu click, mouse aim and cast\n";
    return 0;
} catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; } } }
