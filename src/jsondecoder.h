#pragma once
#include <variant>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cctype>
#include <stdexcept>

namespace json {

// Forward declaration
struct JSONValue;

using JSONArray = std::vector<JSONValue>;
using JSONObject = std::map<std::string, JSONValue>;

// JSONValue definition
struct JSONValue {
    using ValueType = std::variant<std::nullptr_t, bool, int64_t, double, std::string,
                                   std::shared_ptr<JSONArray>, std::shared_ptr<JSONObject>>;
    ValueType value;

    JSONValue() : value(nullptr) {}
    JSONValue(bool b) : value(b) {}
    JSONValue(int64_t i) : value(i) {}
    JSONValue(double d) : value(d) {}
    JSONValue(const std::string& s) : value(s) {}
    JSONValue(const char* s) : value(std::string(s)) {}
    JSONValue(const JSONArray& a) : value(std::make_shared<JSONArray>(a)) {}
    JSONValue(const JSONObject& o) : value(std::make_shared<JSONObject>(o)) {}
};

// JSON parsing errors
struct JSONDecodeError : public std::runtime_error {
    JSONDecodeError(const std::string& msg, size_t pos)
        : std::runtime_error(msg), position(pos) {}
    size_t position;
};

// Utility functions
inline void skipWhitespace(const std::string& s, size_t& i) {
    while (i < s.size() && std::isspace(s[i])) ++i;
}

// Forward declarations
JSONValue parseValue(const std::string& s, size_t& i);
std::string parseString(const std::string& s, size_t& i);

// Parse JSON object
inline JSONValue parseObject(const std::string& s, size_t& i) {
    if (s[i] != '{') throw JSONDecodeError("Expected '{'", i);
    ++i; // skip '{'
    skipWhitespace(s, i);

    JSONObject obj;
    while (i < s.size() && s[i] != '}') {
        skipWhitespace(s, i);
        std::string key = parseString(s, i);
        skipWhitespace(s, i);
        if (s[i] != ':') throw JSONDecodeError("Expected ':'", i);
        ++i;
        skipWhitespace(s, i);
        JSONValue val = parseValue(s, i);
        obj[key] = val;
        skipWhitespace(s, i);
        if (s[i] == ',') ++i;
        skipWhitespace(s, i);
    }

    if (i >= s.size() || s[i] != '}') throw JSONDecodeError("Expected '}'", i);
    ++i;
    return JSONValue(obj);
}

// Parse JSON array
inline JSONValue parseArray(const std::string& s, size_t& i) {
    if (s[i] != '[') throw JSONDecodeError("Expected '['", i);
    ++i; // skip '['
    skipWhitespace(s, i);

    JSONArray arr;
    while (i < s.size() && s[i] != ']') {
        JSONValue val = parseValue(s, i);
        arr.push_back(val);
        skipWhitespace(s, i);
        if (s[i] == ',') ++i;
        skipWhitespace(s, i);
    }

    if (i >= s.size() || s[i] != ']') throw JSONDecodeError("Expected ']'", i);
    ++i;
    return JSONValue(arr);
}

// Parse JSON string
inline std::string parseString(const std::string& s, size_t& i) {
    if (s[i] != '"') throw JSONDecodeError("Expected string", i);
    ++i;
    std::string result;
    while (i < s.size()) {
        char c = s[i++];
        if (c == '"') break;
        if (c == '\\') {
            if (i >= s.size()) throw JSONDecodeError("Unterminated escape", i);
            char esc = s[i++];
            switch (esc) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u': // Unicode \uXXXX
                    if (i + 3 >= s.size()) throw JSONDecodeError("Invalid unicode escape", i);
                    {
                        std::string hex = s.substr(i, 4);
                        char16_t code = static_cast<char16_t>(std::stoi(hex, nullptr, 16));
                        result += static_cast<char>(code); // naive: assumes ASCII
                        i += 4;
                    }
                    break;
                default: throw JSONDecodeError("Invalid escape", i);
            }
        } else {
            result += c;
        }
    }
    return result;
}

// Parse JSON number, true, false, null
inline JSONValue parseValue(const std::string& s, size_t& i) {
    skipWhitespace(s, i);
    if (i >= s.size()) throw JSONDecodeError("Unexpected end", i);

    if (s[i] == '"') return JSONValue(parseString(s, i));
    if (s[i] == '{') return parseObject(s, i);
    if (s[i] == '[') return parseArray(s, i);

    // literals
    if (s.compare(i, 4, "true") == 0) { i += 4; return JSONValue(true); }
    if (s.compare(i, 5, "false") == 0) { i += 5; return JSONValue(false); }
    if (s.compare(i, 4, "null") == 0) { i += 4; return JSONValue(nullptr); }

    // number
    size_t start = i;
    if (s[i] == '-') ++i;
    while (i < s.size() && std::isdigit(s[i])) ++i;
    bool isFloat = false;
    if (i < s.size() && s[i] == '.') { isFloat = true; ++i; while (i < s.size() && std::isdigit(s[i])) ++i; }
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) { isFloat = true; ++i; if (s[i]=='+'||s[i]=='-') ++i; while (i<s.size() && std::isdigit(s[i])) ++i; }

    std::string numStr = s.substr(start, i - start);
    if (isFloat)
        return JSONValue(static_cast<double>(std::stod(numStr)));
    else
        return JSONValue(static_cast<int64_t>(std::stoll(numStr)));
}

// Entry point
inline JSONValue parse(const std::string& s) {
    size_t i = 0;
    JSONValue v = parseValue(s, i);
    skipWhitespace(s, i);
    if (i != s.size()) throw JSONDecodeError("Extra data", i);
    return v;
}

} // namespace json
