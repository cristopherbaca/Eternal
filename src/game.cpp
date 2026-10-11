#include "game.hpp"
#include "ui/hud.hpp"
#include <cstdio>
namespace er {
Game::Game(uint32_t seed,const std::filesystem::path& dataPath):data(dataPath),world(seed) {
    data.applyClass(hero,Role::Mage);
    Entity e; e.kind=Kind::Party; e.id=2; e.name="Stasia_apk"; e.role=Role::Cleric; e.pos={-4,14}; e.hp=e.maxHp=244; e.mp=e.maxMp=556; e.level=5; e.color={148,186,137}; party.push_back(e);
    e.id=3; e.name="Jade"; e.role=Role::Warrior; e.pos={5,16}; e.hp=e.maxHp=786; e.mp=e.maxMp=53; e.level=1; e.color={151,151,171}; party.push_back(e);
    e.id=4; e.name="Elowen"; e.role=Role::Mage; e.pos={-6,9}; e.hp=e.maxHp=183; e.mp=e.maxMp=350; e.level=7; e.color={132,137,196}; party.push_back(e);
    dragon.alive=false;
}
std::vector<Entity*> Game::entities() { std::vector<Entity*> es{&hero}; for(auto& e:party) es.push_back(&e); for(auto& e:mobs) es.push_back(&e); if(dragon.id==500) es.push_back(&dragon); return es; }
void Game::setScene(const std::string& name) {
    if(name=="water") hero.pos={0,34}; else if(name=="forest") hero.pos={-33,3}; else if(name=="ruins") hero.pos={0,-2}; else if(name=="boss") hero.pos={30,-17};
    camera.center=hero.pos; aim=hero.pos+Vec2{7,-3};
    for(size_t i=0;i<party.size();++i) party[i].pos=hero.pos+Vec2{float(i)*5-5,3};
}
void Game::input(const Input& in) {
    Vec2 direction{};
    for(char k:in.keys) {
        if(k=='q') quit=true;
        if(k=='c') classMenu=!classMenu;
        if(k=='h') helpMenu=!helpMenu;
        if(k>='1'&&k<='6') {
            if(classMenu&&k<='5') { data.applyClass(hero,Role(k-'1')); classMenu=false; message=std::string("Class: ")+roleName(hero.role); }
            else if(!classMenu) casts.push_back(k-'1');
        }
        if(k==' ') casts.push_back(0);
        if(k=='\t') {
            auto es=entities(); std::vector<int> ids;
            for(auto* e:es) if(e->id!=hero.id&&(e->alive||e->kind==Kind::Party)&&distance(e->pos,hero.pos)<35) ids.push_back(e->id);
            auto current=std::find(ids.begin(),ids.end(),hero.target);
            if(!ids.empty()) hero.target=(current==ids.end()||++current==ids.end())?ids.front():*current;
            for(auto* e:es) if(e->id==hero.target) aim=e->pos;
        }
        if(k=='^') aim.y-=2; if(k=='v') aim.y+=2; if(k=='<') aim.x-=2; if(k=='>') aim.x+=2;
        if(k=='w') direction.y-=1; if(k=='s') direction.y+=1;
        if(k=='a') direction.x-=1; if(k=='d') direction.x+=1;
        if(k=='='||k=='+') camera.zoom=clamp(camera.zoom+.1f,.55f,2.f); if(k=='-') camera.zoom=clamp(camera.zoom-.1f,.55f,2.f);
    }
    if(length(direction)>0) { velocity=normalized(direction)*hero.speed; movementTime=.16f; }
    if(in.scroll) camera.zoom=clamp(camera.zoom+in.scroll*.1f,.55f,2.f);
    if(in.mouseX>=0) aim=camera.unproject({float(in.mouseX),float(in.mouseY)});
    if(in.click) casts.push_back(0);
}
void Game::tick(float dt) {
    time+=dt; movementTime-=dt; if(movementTime<=0) velocity={};
    if(hero.alive) hero.pos=world.move(hero.pos,velocity*dt*(world.water(hero.pos)?.55f:1.f)*(hero.slow>0?.35f:1.f)); hero.velocity=velocity;
    auto es=entities(); combat.tick(dt,es,world,texts);
    for(int slot:casts) { auto error=combat.cast(hero,slot,aim,hero.target,es,data,world,texts); if(!error.empty()) message=error; }
    casts.clear();
    world.tick(dt); world.pushGrass(hero.pos,2.8f,dt*30);
    for(const auto& e:party) world.pushGrass(e.pos,2.5f,dt*25);
    for(auto& t:texts) t.life-=dt;
    texts.erase(std::remove_if(texts.begin(),texts.end(),[](const auto& t){return t.life<=0;}),texts.end());
    camera.center=lerp(camera.center,hero.pos,1-std::exp(-dt*6));
}
void Game::render(Framebuffer& f,float ms) {
    camera.width=f.width; camera.height=f.height; scene.begin(camera); world.render(scene,time);
    if(hero.alive) drawCharacter(scene,hero.pos,hero.color,time,length(velocity)>0,int(hero.role));
    for(const auto& e:party) if(e.alive) drawCharacter(scene,e.pos,e.color,time,length(e.velocity)>0,int(e.role));
    combat.render(scene,time);
    world.reflections(scene,time); scene.convert(f);
    nameLabel(f,camera,hero); for(const auto& e:party) nameLabel(f,camera,e,e.id==hero.target);
    floatingLabels(f,camera,texts); hud(f,hero,party,nullptr,ms);
    Vec2 cursor=camera.project(aim); f.put(int(cursor.x),int(cursor.y),U'+',white);
    for(int y=f.height-4;y<f.height;++y) for(int x=0;x<f.width;++x) f.put(x,y,U' ');
    int slotWidth=std::max(1,(f.width-4)/6);
    for(int i=0;i<6;++i) {
        const auto& a=data.abilities[size_t(hero.role)][size_t(i)]; std::string label=std::to_string(i+1)+" "+a.name;
        if(hero.cooldown[size_t(i)]>0) label="["+std::to_string(i+1)+"] "+std::to_string(int(std::ceil(hero.cooldown[size_t(i)])))+"s";
        f.text(2+i*slotWidth,f.height-4,label.substr(0,size_t(std::max(1,slotWidth-1))),hero.cooldown[size_t(i)]>0?muted:Color{215,202,161});
        if(f.width>90) f.text(2+i*slotWidth,f.height-3,std::to_string(int(a.cost))+" MP",muted);
    }
    f.text(2,f.height-2,message.substr(0,size_t(std::max(0,f.width-4))),muted);
    f.text(2,f.height-1,"WASD move  Mouse/arrows aim  1-6 cast  Tab target  C class  H help  Q quit",{82,104,117});
    if(classMenu||helpMenu) {
        int w=std::min(66,f.width-4),x=(f.width-w)/2,y=std::max(2,f.height/2-7); box(f,x,y,w,14);
        f.text(x+2,y+1,classMenu?"CHOOSE YOUR CLASS":"ASHENMERE / controls",{222,201,148});
        if(classMenu) for(int i=0;i<5;++i) f.text(x+3,y+3+i*2,std::to_string(i+1)+"  "+roleName(Role(i)));
        else { const char* lines[]={"WASD: move / hold to repeat","Mouse or arrows: aim / click or Space: primary","1-6: ability / Tab: cycle nearby targets","C: class / +/- or wheel: zoom","I: inventory / E: collect loot / R: respawn","B: travel to Dragon arena / H: close help","Q or Ctrl-C: restore terminal and quit"}; for(int i=0;i<7;++i) f.text(x+2,y+3+i,lines[i]); }
    }
}
}
