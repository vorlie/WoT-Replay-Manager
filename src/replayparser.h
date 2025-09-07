#pragma once
#include <string>
#include <vector>
#include "jsondecoder.h"

using json::JSONValue;

struct ReplayData {
    JSONValue metadata;
    std::vector<JSONValue> battles;
};

class ReplayParser {
public:
    explicit ReplayParser(const std::string& filepath);
    bool parse();
    const ReplayData& getData() const;

private:
    std::string filepath;
    std::string fileContent;
    ReplayData data;

    bool readFile();
    std::vector<std::string> extractJsonBlocks() const;
};
