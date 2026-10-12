#include "game.hpp"
namespace er {
int Game::buyPrice(const Person& person,const ItemDef& item) const { return std::max(2,int(item.value*(person.trait==0?1.1f:1.3f))); }
void Game::tradeWith(const Person& p,int action) {
    auto& state=population.changes[p.id]; if(state.gold<0) state.gold=p.gold;
    if(action<3) {
        int id=int((p.key+uint32_t(action))%data.items.size()); const auto& item=data.items[size_t(id)]; int price=buyPrice(p,item);
        if(hero.gold<price||hero.inventory.size()>=24) { dialogue="Not enough gold or inventory space."; return; }
        hero.gold-=price; state.gold+=price; hero.inventory.push_back(id); dialogue="Bought "+item.name+".";
    } else if(action==3) {
        if(hero.inventory.empty()) { dialogue="You have nothing to sell."; return; }
        inventoryIndex=clamp(inventoryIndex,0,int(hero.inventory.size())-1); int id=hero.inventory[size_t(inventoryIndex)]; const auto& item=data.items[size_t(id)];
        if(hero.equipment[size_t(item.slot)]==id) { dialogue="Unequip that item before selling it."; return; }
        int price=std::max(1,item.value/2); if(state.gold<price) { dialogue="I cannot afford that today."; return; }
        state.gold-=price; hero.gold+=price; hero.inventory.erase(hero.inventory.begin()+inventoryIndex); dialogue="Sold "+item.name+" for "+std::to_string(price)+" gold.";
    } else if(action==4) {
        if(hero.gold<15) { dialogue="Restoration costs 15 gold."; return; }
        hero.gold-=15; state.gold+=15; hero.hp=hero.maxHp; hero.mp=hero.maxMp; dialogue="Your equipment is tended and your strength restored.";
    }
}
}
