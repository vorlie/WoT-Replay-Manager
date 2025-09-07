#include "replayparser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "jsondecoder.h"

ReplayParser::ReplayParser(const std::string& filepath)
    : filepath(filepath) {}

bool ReplayParser::readFile() {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return false;

    std::ostringstream ss;
    ss << file.rdbuf();
    fileContent = ss.str();
    return true;
}

std::vector<std::string> ReplayParser::extractJsonBlocks() const {
    std::vector<std::string> jsons;
    size_t pos = 0;

    while (pos < fileContent.size()) {
        size_t start = fileContent.find('{', pos);
        if (start == std::string::npos) break;

        int braceCount = 0;
        size_t end = start;
        bool inString = false;

        for (; end < fileContent.size(); ++end) {
            unsigned char c = fileContent[end];
            if (c == '"' && (end == 0 || fileContent[end - 1] != '\\')) inString = !inString;
            if (!inString) {
                if (c == '{') ++braceCount;
                else if (c == '}') --braceCount;
            }
            if (braceCount == 0) break;
        }

        if (braceCount == 0) {
            jsons.push_back(fileContent.substr(start, end - start + 1));
            pos = end + 1;
        } else break; // malformed
    }

    return jsons;
}

bool ReplayParser::parse() {
    if (!readFile()) return false;

    auto jsonBlocks = extractJsonBlocks();
    if (jsonBlocks.empty()) return false;

    try {
        data.metadata = json::parse(jsonBlocks[0]);
        for (size_t i = 1; i < jsonBlocks.size(); ++i) {
            data.battles.push_back(json::parse(jsonBlocks[i]));
        }
    } catch (const json::JSONDecodeError& e) {
        std::cerr << "JSON parse error at pos " << e.position << ": " << e.what() << "\n";
        return false;
    }

    return true;
}

const ReplayData& ReplayParser::getData() const {
    return data;
}
