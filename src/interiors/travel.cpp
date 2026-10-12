#include "game.hpp"
#include "combat/combat.hpp"
namespace er {
void Game::interactPlace() {
    if(!hero.alive||classMenu||inventoryMenu||helpMenu) return;
    if(world.interior) {
        auto& inside=*world.interior;
        if(distance(hero.pos,inside.entrance)<4) { leaveInterior(); return; }
        if(distance(hero.pos,inside.chest)<4) {
            if(inside.peaceful) { hero.hp=hero.maxHp; hero.mp=hero.maxMp; time+=120; message="You rest by the hearth. Health and mana restored."; return; }
            if(inside.opened) { message="The chest is empty."; return; }
            if(std::any_of(mobs.begin(),mobs.end(),[](const Entity& e){return e.alive;})) { message="Defeat the vault's guardians before opening the chest."; return; }
            int item=int(atlas.place(*interiorSite).key%data.items.size());
            if(hero.inventory.size()>=24) { message="Inventory full. Make room before claiming the chest."; return; }
            hero.inventory.push_back(item); hero.gold+=60; awardXP(100); inside.opened=true; clearedPlaces.insert(*interiorSite);
            message="Vault cleared! Found "+data.items[size_t(item)].name+", 60 gold and 100 XP.";
            return;
        }
        message="F at <: leave | F at chest/bed: interact | ^: trap"; return;
    }
    if(!hero.testing&&(fight.active||hero.hurtTime>0)) { message="Cannot enter while fighting."; return; }
    atlas.focus(hero.pos);
    const Place* nearest=nullptr;
    for(const auto& place:atlas.nearby) if(distance(place.pos,hero.pos)<7&&(!nearest||distance(place.pos,hero.pos)<distance(nearest->pos,hero.pos))) nearest=&place;
    if(!nearest) { message="Approach a landmark's banner/door and press F."; return; }
    if(!atlas.types[size_t(nearest->type)].interior) { message=nearest->name+": "+atlas.types[size_t(nearest->type)].purpose; return; }
    Place site=*nearest;
    outsidePosition=hero.pos; outsideMobs=std::move(mobs); outsideLoot=std::move(loot);
    interiorSite=site.id; world.interior.emplace(site.key,atlas.types[size_t(site.type)].inhabited);
    world.interior->opened=clearedPlaces.count(site.id); hero.pos=world.interior->entrance;
    mobs.clear(); loot.clear(); combat.areas.clear(); combat.projectiles.clear(); casts.clear(); texts.clear();
    if(!world.interior->peaceful&&!world.interior->opened) {
        for(Vec2 pos:{Vec2{22,13},Vec2{40,13}}) spawnMob(pos,0);
        if(!mobs.empty()) { mobs.back().name="Vault Warden"; mobs.back().hp=mobs.back().maxHp*=2; }
    }
    for(auto& e:party) e.pos=hero.pos+Vec2{1,0};
    camera.center=hero.pos; aim=hero.pos+Vec2{5,0}; velocity={}; hero.velocity={}; transition=.25f;
    message="Entered "+site.name+". F: interact / exit at <";
}
void Game::leaveInterior() {
    if(!world.interior) return;
    world.interior.reset(); interiorSite.reset(); hero.pos=outsidePosition; mobs=std::move(outsideMobs); loot=std::move(outsideLoot);
    combat.areas.clear(); combat.projectiles.clear(); casts.clear(); texts.clear();
    for(auto& e:party) e.pos=hero.pos+Vec2{1,0};
    camera.center=hero.pos; aim=hero.pos+Vec2{5,0}; world.focus(hero.pos); atlas.focus(hero.pos); velocity={}; hero.velocity={}; transition=.25f;
    message="Back outside. M: atlas | F: enter / interact";
}
void Game::tickInterior(float dt) {
    if(!world.interior) return;
    auto& inside=*world.interior;
    if(!inside.peaceful&&!inside.trapSpent&&inside.at(int(std::round(hero.pos.x)),int(std::round(hero.pos.y)))=='^') {
        inside.trapSpent=true; dealDamage(hero,18,texts); message="A pressure plate springs a trap!";
    }
    (void)dt;
}
}
