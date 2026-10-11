#pragma once
#include "core/math.hpp"
#include <array>
#include <string>
#include <vector>
namespace er {
enum class Role { Warrior,Mage,Archer,Cleric,Rogue };
enum class Kind { Player,Party,Mob,Boss,Remote };
enum class Brain { Idle,Aggro,Chase,Attack,Dead };
struct Entity {
    int id=1; std::string name="Senje"; Kind kind=Kind::Player; Role role=Role::Mage;
    Vec2 pos{0,12},velocity{},home{0,12},facing{1,0}; Color color{121,132,195};
    float hp=129,maxHp=129,mp=129,maxMp=129,armor=8,power=25,speed=8;
    int level=11,xp=0,gold=3880,target=0;
    bool alive=true,rewarded=false; Brain brain=Brain::Idle;
    float attackTimer=0,poison=0,slow=0,shield=0,deadTime=0,taunt=0,hurtTime=0,dotTimer=0;
    std::array<float,6> cooldown{};
    std::vector<int> inventory; int equipped=-1;
};
struct FloatingText { Vec2 pos; std::string text; Color color=white; float life=1.4f,maxLife=1.4f; };
inline const char* roleName(Role r) { static const char* names[]={"Warrior","Mage","Archer","Cleric","Rogue"}; return names[int(r)]; }
}
