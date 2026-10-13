#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace er
{
    struct GenerationConfig
    {
        float treeDensity = .62f, lakeThreshold = .28f, biomeScale = 192, respawnSeconds = 45, levelDistance = 120;
        int enemiesPerChunk = 3;
        unsigned seedSalt = 0;
    };
    struct ModSet
    {
        GenerationConfig generation;
        std::vector<std::filesystem::path> directories;
        std::vector<std::string> names;
    };
    ModSet loadMods(const std::filesystem::path &root = {}, const std::vector<std::string> &additional = {}, bool disabled = false);
}
