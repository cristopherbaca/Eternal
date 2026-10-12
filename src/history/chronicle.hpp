#pragma once
#include "npc/population.hpp"
namespace er {
struct HistoryEvent { int year=0; PlaceId place; std::string person,faction,text; };
class Chronicle {
public:
    std::vector<HistoryEvent> events;
    std::set<int> learned;
    Chronicle(const Atlas& atlas,const Population& population,const std::filesystem::path& data={});
    std::string account(int index) const;
};
}
