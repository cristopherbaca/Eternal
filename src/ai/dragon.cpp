#include "ai/ai.hpp"
#include "game.hpp"
namespace er {
void Game::resetBoss() {
    fight={}; dragon.hp=dragon.maxHp; dragon.pos=dragon.home; dragon.alive=true; dragon.poison=dragon.slow=dragon.taunt=0;
    combat.areas.clear(); combat.projectiles.clear();
    mobs.erase(std::remove_if(mobs.begin(),mobs.end(),[](const Entity& e){return e.summoned;}),mobs.end());
}
void tickDragon(Game& g,float dt) {
    auto& b=g.dragon; auto& fight=g.fight; if(fight.won) return;
    if(!b.alive) {
        fight.won=true; fight.active=false; g.hero.gold+=g.data.boss.gold; g.hero.xp+=1200;
        for(auto& e:g.guests) if(distance(e.pos,b.pos)<40) { e.gold+=g.data.boss.gold; e.xp+=1200; }
        g.loot.push_back({b.pos,3,200}); g.loot.push_back({b.pos+Vec2{2,0},4,150}); g.combat.areas.clear();
        g.message="Dragon defeated! +1200 XP, +"+std::to_string(g.data.boss.gold)+"g. E: claim Dragonfang";
        g.combat.burst(b.pos,{255,204,100},100,16); return;
    }
    if(!fight.active) { bool nearby=false; for(auto* e:g.entities()) if(e->alive&&(e->kind==Kind::Player||e->kind==Kind::Remote)&&distance(e->pos,b.pos)<23) nearby=true; if(!nearby) return; fight.active=true; g.message="Dragon awakens. Dodge the red rings!"; }
    auto es=g.entities(); Entity* victim=nullptr; float best=1000; bool survivors=false;
    for(auto* e:es) if(e->alive&&hostile(b,*e)) { survivors=true; float d=distance(e->pos,b.pos); if(d<best) { best=d; victim=e; } }
    if(!survivors||best>55) { g.resetBoss(); g.message="The encounter resets. R: respawn after a wipe."; return; }
    if(b.taunt>0) for(auto* e:es) if(e->id==b.target&&e->alive) victim=e;
    if(!victim) return;
    fight.elapsed+=dt; b.facing=normalized(victim->pos-b.pos); b.target=victim->id;
    int phase=1; for(const auto& p:g.data.phases) if(b.hp/b.maxHp<=p.threshold) phase=p.phase;
    if(phase!=fight.phase) { fight.phase=phase; fight.ring=.7f; fight.adds=2; g.message="Dragon - phase "+std::to_string(phase); g.combat.burst(b.pos,{245,118,61},80,12); }
    const auto& def=g.data.phases[size_t(phase-1)]; float enrage=fight.elapsed>g.data.boss.enrage?2.5f:1;
    if(enrage>1) g.message="Dragon is ENRAGED!";
    b.attackTimer-=dt;
    if(distance(b.pos,victim->pos)>3.5f) b.pos=g.world.move(b.pos,b.facing*b.speed*dt);
    else if(b.attackTimer<=0) { dealDamage(*victim,b.power*enrage,g.texts); b.attackTimer=1.5f; }
    fight.ring-=dt; fight.breath-=dt; fight.wing-=dt; fight.adds-=dt;
    if(fight.ring<=0) {
        fight.ring=def.ringInterval; std::vector<Entity*> allies;
        for(auto* e:es) if(e->alive&&hostile(b,*e)&&distance(e->pos,b.pos)<35) allies.push_back(e);
        for(int i=0;i<def.rings&&!allies.empty();++i) { auto* e=allies[size_t((int(fight.elapsed)+i)%int(allies.size()))]; g.combat.areas.push_back({e->pos,4.5f,4,1.5f,.8f,0,def.damage*enrage,b.id,"enemy_fire"}); }
    }
    if(fight.breath<=0) { fight.breath=def.breathInterval; g.combat.areas.push_back({b.pos,13,2.5f,1.5f,.6f,0,def.damage*.7f*enrage,b.id,"cone",b.facing}); }
    if(fight.wing<=0) { fight.wing=12-phase; g.combat.areas.push_back({b.pos,8,.15f,1.5f,1,0,55*enrage,b.id,"sweep"}); g.message="Dragon spreads its wings - move away!"; }
    if(fight.adds<=0&&def.adds>0) { fight.adds=20; for(int i=0;i<def.adds;++i) { size_t before=g.mobs.size(); g.spawnMob(b.pos+Vec2{float(i)*4-4,5},0); if(g.mobs.size()>before) g.mobs.back().summoned=true; } }
}
}
