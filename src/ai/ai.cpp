#include "ai/ai.hpp"
#include "game.hpp"
namespace er {
void tickMobs(Game& g,float dt) {
    auto es=g.entities();
    for(auto& mob:g.mobs) {
        if(!mob.alive) continue;
        const auto& def=g.data.enemies[size_t(mob.archetype)]; Entity* target=nullptr; float best=def.aggro;
        if(mob.taunt>0) for(auto* e:es) if(e->id==mob.target&&e->alive) target=e;
        if(!target) for(auto* e:es) if(e->alive&&hostile(mob,*e)) { float d=distance(mob.pos,e->pos); if(d<best) { target=e; best=d; } }
        mob.velocity={}; mob.attackTimer=std::max(0.f,mob.attackTimer-dt);
        if(!target) { mob.brain=Brain::Idle; if(distance(mob.pos,mob.home)>1) mob.velocity=normalized(mob.home-mob.pos)*mob.speed*.5f; }
        else {
            mob.target=target->id; mob.facing=normalized(target->pos-mob.pos); float d=distance(mob.pos,target->pos);
            if(d>def.range) { mob.brain=mob.brain==Brain::Idle?Brain::Aggro:Brain::Chase; mob.velocity=mob.facing*mob.speed; }
            else { mob.brain=Brain::Attack; if(mob.attackTimer<=0) { dealDamage(*target,mob.power,g.texts); mob.attackTimer=1.3f; g.combat.burst(target->pos,mob.color,5,1); } }
        }
        mob.pos=g.world.move(mob.pos,mob.velocity*dt*(mob.slow>0?.2f:1.f));
        g.world.pushGrass(mob.pos,2,dt*15);
    }
}
}
