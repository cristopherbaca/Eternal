#include "mods/mods.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>
#include <cmath>
#include <stdexcept>
namespace er
{
    ModSet loadMods(const std::filesystem::path &requested, const std::vector<std::string> &additional, bool disabled)
    {
        ModSet result;
        if (disabled)
            return result;
        auto root = requested.empty() ? (std::filesystem::exists("mods") ? std::filesystem::path("mods") : std::filesystem::path(ER_DATA_DIR).parent_path() / "mods") : requested;
        std::vector<std::string> ids;
        std::ifstream enabled(root / "enabled.txt");
        std::string line;
        while (std::getline(enabled, line))
        {
            std::istringstream in(line);
            std::string id;
            if (in >> id && id[0] != '#')
                ids.push_back(id);
        }
        ids.insert(ids.end(), additional.begin(), additional.end());
        std::set<std::string> seen;
        for (const auto &id : ids)
        {
            if (id.empty() || id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") != std::string::npos)
                throw std::runtime_error("Invalid mod id: " + id);
            if (!seen.insert(id).second)
                continue;
            auto directory = root / id;
            std::ifstream manifest(directory / "mod.txt");
            if (!manifest)
                throw std::runtime_error("Missing mod manifest: " + (directory / "mod.txt").string());
            int api = 0;
            std::string name = id;
            while (std::getline(manifest, line))
            {
                std::istringstream in(line);
                std::string key;
                if (!(in >> key) || key[0] == '#')
                    continue;
                if (key == "api")
                    in >> api;
                else if (key == "name")
                    in >> std::quoted(name);
                else
                    throw std::runtime_error("Unknown mod manifest field: " + key);
                if (in.fail())
                    throw std::runtime_error("Malformed mod manifest: " + id);
            }
            if (api != 1)
                throw std::runtime_error("Unsupported mod API in " + id + " (expected 1)");
            if (name.empty() || name.size() > 60 || name.find_first_of("\n\r\x1b") != std::string::npos)
                throw std::runtime_error("Invalid mod name: " + id);
            auto &g = result.generation;
            std::ifstream world(directory / "world.txt");
            while (std::getline(world, line))
            {
                std::istringstream in(line);
                std::string key;
                double value = 0;
                if (!(in >> key) || key[0] == '#')
                    continue;
                if (!(in >> value) || !std::isfinite(value))
                    throw std::runtime_error("Invalid world value in " + id + ": " + key);
                auto require = [&](double low, double high, bool integer = false)
                {
                    if (value < low || value > high || (integer && value != std::floor(value)))
                        throw std::runtime_error("World value out of range in " + id + ": " + key);
                };
                if (key == "tree_density")
                {
                    require(0, 1);
                    g.treeDensity = float(value);
                }
                else if (key == "lake_threshold")
                {
                    require(0, .6);
                    g.lakeThreshold = float(value);
                }
                else if (key == "biome_scale")
                {
                    require(64, 1024);
                    g.biomeScale = float(value);
                }
                else if (key == "enemies_per_chunk")
                {
                    require(0, 8, true);
                    g.enemiesPerChunk = int(value);
                }
                else if (key == "respawn_seconds")
                {
                    require(5, 600);
                    g.respawnSeconds = float(value);
                }
                else if (key == "level_distance")
                {
                    require(30, 10000);
                    g.levelDistance = float(value);
                }
                else if (key == "seed_salt")
                {
                    require(0, 4294967295.0, true);
                    g.seedSalt = unsigned(value);
                }
                else
                    throw std::runtime_error("Unknown world setting in " + id + ": " + key);
            }
            result.directories.push_back(directory);
            result.names.push_back(name);
        }
        return result;
    }
}
