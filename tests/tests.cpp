#include "ascii_conversion/ascii.hpp"
#include "platform/terminal.hpp"
#include "world/world.hpp"
#include "combat/combat.hpp"
#include "game.hpp"
#include <iostream>
#include <stdexcept>
void check(bool ok,const char* name) { if(!ok) throw std::runtime_error(name); }
int main() { try {
    using namespace er;
    check(std::abs(length(normalized({3,4}))-1)<.0001f,"vector normalization");
    check(length(normalized({0,0}))==0,"zero normalization");
    Camera c; c.zoom=1.4f; Vec2 p{12,-6}; check(distance(p,c.unproject(c.project(p)))<.0001f,"projection inverse");
    check(noise(-4,13,42)==noise(-4,13,42),"seed deterministic");
    check(brightnessGlyph(-1)==U' '&&brightnessGlyph(2)==U'@',"ramp clamps");
    check(edgeGlyph(10,0)==U'|'&&edgeGlyph(0,10)==U'-',"edge direction");
    Pixel px; px.color={90,90,90}; auto dark=shade(px,{},1),lit=shade(px,{{{0,0},0,{255,120,20},10,1}},1);
    check(lit.r>dark.r&&int(lit.r)-dark.r>int(lit.b)-dark.b,"warm radial light");
    check(luminance(shade(px,{},.5f))<luminance(dark),"vignette");
    Framebuffer f(20,10); check(!f.diff().empty()&&f.diff().empty(),"diff only changes"); f.put(1,1,U'^'); check(!f.diff().empty(),"changed cell emitted");
    InputParser parser; check(parser.feed("\x1b[<0;12;").keys.empty(),"split mouse input"); auto in=parser.feed("8Mw"); check(in.click&&in.mouseX==11&&in.mouseY==7&&in.keys=="w","mouse continuation");
    World a(42),b(42); check(a.trees.size()==b.trees.size(),"world repeatability"); check(a.blocked({90,0}),"world boundary");
    a.pushGrass({0,12},3,10); a.tick(1.f/60); check(length(a.grassBend({1,12}))>0,"grass deflects");
    for(int i=0;i<600;++i) a.tick(1.f/60); check(length(a.grassBend({1,12}))<.001f,"grass spring recovers");
    check(std::abs(damageAfterArmor(100,100)-50)<.001f,"armor damage formula");
    Data data; Entity mage,mob; data.applyClass(mage,Role::Mage); mob.id=100; mob.kind=Kind::Mob; mob.pos={5,12}; mob.hp=mob.maxHp=100; mob.armor=0;
    Combat combat; std::vector<Entity*> es{&mage,&mob}; std::vector<FloatingText> texts;
    float mp=mage.mp; check(combat.cast(mage,0,mob.pos,mob.id,es,data,a,texts).empty(),"fire orb cast");
    check(mage.mp<mp&&!combat.cast(mage,0,mob.pos,mob.id,es,data,a,texts).empty(),"cost and cooldown");
    for(int i=0;i<40;++i) combat.tick(1.f/60,es,a,texts); check(mob.hp<100,"projectile swept hit");
    mob.hp=90; heal(mob,200,texts); check(mob.hp==100,"heal clamps maximum"); dealDamage(mob,1000,texts); check(!mob.alive&&mob.hp==0,"death clamps zero");
    for(int r=0;r<5;++r) for(int slot=0;slot<6;++slot) {
        Entity e,ally; data.applyClass(e,Role(r)); e.mp=1000; ally.id=2; ally.kind=Kind::Party; ally.alive=!(r==3&&slot==4); ally.hp=50; ally.pos=e.pos;
        std::vector<Entity*> targets{&e,&ally,&mob}; check(combat.cast(e,slot,{5,12},ally.id,targets,data,a,texts).empty(),"all class abilities cast");
    }
    Game game; auto& victim=game.mobs.front(); victim.hp=1; dealDamage(victim,10,game.texts); game.rewards(); check(!game.loot.empty()&&game.hero.xp>0,"kill yields loot and XP");
    int xp=game.hero.xp; game.rewards(); check(game.hero.xp==xp,"kill rewarded once");
    game.hero.pos=game.loot.front().pos; game.collectLoot(); check(!game.hero.inventory.empty(),"nearby loot collected");
    game.equipSelected(); float power=game.hero.power,armor=game.hero.armor; game.equipSelected(); check(power==game.hero.power&&armor==game.hero.armor,"equipment cannot stack repeatedly");
    dealDamage(game.hero,10000,game.texts); game.respawn(); check(game.hero.alive&&game.hero.hp==game.hero.maxHp,"death and respawn");
    std::cout<<"Math, projection, ASCII, lighting, framebuffer, input and world checks passed\n";
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; } }
