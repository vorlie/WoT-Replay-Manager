#ifndef REPLAYHELPER_H
#define REPLAYHELPER_H

#endif // REPLAYHELPER_H

#pragma once
#include <string>
#include <vector>
#include "json.hpp"

struct ReplayData {
    nlohmann::json metadata;
    std::vector<nlohmann::json> battles;
};

class ReplayHelper {
public:
    static std::vector<unsigned char> readBinaryFile(const std::string &filepath);
    static ReplayData parseReplay(const std::vector<unsigned char> &data);
};
