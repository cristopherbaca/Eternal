#pragma once
#include "npc/population.hpp"
namespace er {
struct Quest {
    NpcId giver{}; PlaceId target{}; int serial=0;
    bool complete=false,rewarded=false;
    std::string title;
};
}
