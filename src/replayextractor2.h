#ifndef REPLAYEXTRACTOR2_H
#define REPLAYEXTRACTOR2_H

#endif // REPLAYEXTRACTOR2_H
#pragma once
#include "replayparser.h"
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <memory>
#include <iostream>
#include <cstdlib> // for std::abort

struct BattleMetadata {
    std::string replay_date;
    std::string player_vehicle;
    std::string nation;
    std::string tank_tag;
    std::string version;
    std::string client_version;
    std::string player_name;
};

struct BattlePerformance {
    int damage_dealt = 0;
};

class ReplayExtractor {
public:
    explicit ReplayExtractor(const ReplayData& data, const std::string& filename = "")
        : data(data), filename(filename) {}

    std::optional<BattleMetadata> getBattleMetadata() const {
        // Expect metadata to be a JSON object
        auto metaPtr = std::get_if<std::shared_ptr<std::map<std::string, json::JSONValue>>>(&data.metadata.value);
        if (!metaPtr || !*metaPtr) {
            std::cerr << "Metadata is null in file: " << filename << "\n";
            std::abort();
        }

        const auto& obj = **metaPtr;
        BattleMetadata meta;

        auto getStringOrCrash = [&](const std::string& key) -> std::string {
            auto it = obj.find(key);
            if (it == obj.end()) {
                std::cerr << "Missing key '" << key << "' in file: " << filename << "\n";
                std::abort();
            }

            const auto& val = it->second.value;
            auto strPtr = std::get_if<std::string>(&val);
            if (!strPtr) {
                std::cerr << "Null or invalid string for key '" << key << "' in file: " << filename << "\n";
                std::abort();
            }

            return *strPtr;
        };

        meta.replay_date = getStringOrCrash("dateTime");
        meta.player_vehicle = getStringOrCrash("playerVehicle");

        std::string vehicle = meta.player_vehicle;
        size_t dash = vehicle.find('-');
        if (dash != std::string::npos) {
            meta.nation = vehicle.substr(0, dash);
            meta.tank_tag = vehicle.substr(dash + 1);
        }

        meta.version = getStringOrCrash("clientVersionFromXml");
        meta.client_version = getStringOrCrash("clientVersionFromXml");
        meta.player_name = getStringOrCrash("playerName");

        return meta;
    }

    std::vector<BattlePerformance> getBattlePerformance() const {
        std::vector<BattlePerformance> performances;

        for (size_t idx = 0; idx < data.battles.size(); ++idx) {
            const auto& battleVal = data.battles[idx];
            auto battlePtr = std::get_if<std::shared_ptr<std::map<std::string, json::JSONValue>>>(&battleVal.value);
            if (!battlePtr || !*battlePtr) {
                std::cerr << "Battle " << idx << " is null in file: " << filename << "\n";
                std::abort();
            }

            const auto& battle = **battlePtr;
            BattlePerformance perf;

            auto it = battle.find("damageDealt");
            if (it != battle.end()) {
                auto intPtr = std::get_if<int64_t>(&it->second.value);
                if (!intPtr) {
                    std::cerr << "damageDealt is not an int in battle " << idx << " in file: " << filename << "\n";
                    std::abort();
                }
                perf.damage_dealt = static_cast<int>(*intPtr);
            } else {
                std::cerr << "damageDealt missing in battle " << idx << " in file: " << filename << "\n";
                std::abort();
            }

            performances.push_back(perf);
        }

        return performances;
    }

private:
    const ReplayData& data;
    const std::string filename; // Optional: track which file we’re parsing
};
