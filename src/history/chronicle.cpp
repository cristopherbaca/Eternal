#include "history/chronicle.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace er
{
    Chronicle::Chronicle(const Atlas &atlas, const Population &population, const std::filesystem::path &data)
    {
        auto path = data / "history.txt";
        if (!std::filesystem::exists(path))
            path = std::filesystem::path(ER_DATA_DIR) / "history.txt";
        std::ifstream in(path);
        std::string row;
        std::vector<std::string> templates;
        while (std::getline(in, row))
            if (!row.empty() && row[0] != '#')
            {
                std::istringstream line(row);
                std::string text;
                if (!(line >> std::quoted(text)))
                    throw std::runtime_error("Invalid history template");
                templates.push_back(text);
            }
        if (templates.empty())
            throw std::runtime_error("No history templates");
        for (int i = 0; i < 24; ++i)
        {
            uint32_t key = hash(atlas.seed + uint32_t(i) * 977);
            PlaceId id{int(key % 7) - 3, int((key >> 5) % 7) - 3};
            if (i == 3)
                for (int x = 1; x < 100; ++x)
                {
                    auto candidate = atlas.place({x, 0});
                    const auto &type = atlas.types[size_t(candidate.type)];
                    if (type.interior && !type.inhabited)
                    {
                        id = candidate.id;
                        break;
                    }
                }
            auto place = atlas.place(id);
            auto p = population.person(place, int(key % 12));
            HistoryEvent event{710 + i * 11 + int(key % 7), id, p.name, population.jobs[size_t(p.job)].faction, templates[size_t(i) % templates.size()]};
            auto replace = [&](const std::string &from, const std::string &to)
            { size_t at=0; while((at=event.text.find(from,at))!=std::string::npos) { event.text.replace(at,from.size(),to); at+=to.size(); } };
            replace("{person}", event.person);
            replace("{place}", place.name);
            replace("{faction}", event.faction);
            events.push_back(event);
        }
    }
    std::string Chronicle::account(int index) const
    {
        const auto &e = events[size_t(index) % events.size()];
        return "Year " + std::to_string(e.year) + ": " + e.text;
    }
}
