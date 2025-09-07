#pragma once
#include "replayparser.h"
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <memory>

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
    explicit ReplayExtractor(const ReplayData& data) : data(data) {}

    std::optional<BattleMetadata> getBattleMetadata() const {
        // Make sure metadata is an object
        if (!std::holds_alternative<std::shared_ptr<std::map<std::string, json::JSONValue>>>(data.metadata.value))
            return std::nullopt;

        const auto& objPtr = std::get<std::shared_ptr<std::map<std::string, json::JSONValue>>>(data.metadata.value);
        if (!objPtr) return std::nullopt;
        const auto& obj = *objPtr;

        BattleMetadata meta;

        // Helper lambda to get string safely
        auto getString = [&](const std::string& key) -> std::string {
            auto it = obj.find(key);
            if (it == obj.end()) return "";
            if (std::holds_alternative<std::string>(it->second.value))
                return std::get<std::string>(it->second.value);
            return "";
        };

        meta.replay_date = getString("dateTime");
        meta.player_vehicle = getString("playerVehicle");
        meta.version = getString("clientVersionFromXml");
        meta.client_version = getString("clientVersionFromXml");
        meta.player_name = getString("playerName");

        // Split vehicle into nation and tank tag
        std::string vehicle = meta.player_vehicle;
        size_t dash = vehicle.find('-');
        if (dash != std::string::npos) {
            meta.nation = vehicle.substr(0, dash);
            meta.tank_tag = vehicle.substr(dash + 1);
        }

        return meta;
    }

    std::vector<BattlePerformance> getBattlePerformance() const {
        std::vector<BattlePerformance> performances;

        for (const auto& battleVal : data.battles) {
            if (!std::holds_alternative<std::shared_ptr<std::map<std::string, json::JSONValue>>>(battleVal.value))
                continue;

            const auto& battlePtr = std::get<std::shared_ptr<std::map<std::string, json::JSONValue>>>(battleVal.value);
            if (!battlePtr) continue;
            const auto& battle = *battlePtr;

            BattlePerformance perf;

            auto it = battle.find("damageDealt");
            if (it != battle.end() && std::holds_alternative<int64_t>(it->second.value))
                perf.damage_dealt = static_cast<int>(std::get<int64_t>(it->second.value));

            performances.push_back(perf);
        }

        return performances;
    }

private:
    const ReplayData& data;
};
