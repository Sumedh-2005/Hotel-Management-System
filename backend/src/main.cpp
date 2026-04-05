/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║        GRAND HORIZON — C++ Hotel Management Backend         ║
 * ║  OOP-based simulation with file I/O and JSON processing     ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * Compile:  g++ -std=c++17 -o hotel main.cpp
 * Run:      ./hotel
 *
 * File Integration:
 *   Input:  data/pending_booking.json  (written by frontend)
 *   Output: data/booking_result.json   (read by frontend)
 *   Store:  data/bookings.json         (persistent storage)
 *   Rooms:  data/rooms.json            (room inventory)
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <random>

// ═══════════════════════════════════════════════════════════════
// SECTION 1: JSON MINI-PARSER (no external dependencies)
// ═══════════════════════════════════════════════════════════════

class JSON {
public:
    enum Type { STRING, NUMBER, BOOL, OBJECT, ARRAY, NIL };

    Type type;
    std::string str_val;
    double num_val = 0;
    bool bool_val = false;
    std::map<std::string, JSON> obj;
    std::vector<JSON> arr;

    JSON() : type(NIL) {}
    explicit JSON(const std::string& s) : type(STRING), str_val(s) {}
    explicit JSON(double n) : type(NUMBER), num_val(n) {}
    explicit JSON(bool b) : type(BOOL), bool_val(b) {}

    std::string getString(const std::string& key, const std::string& def = "") const {
        if (type == OBJECT) {
            auto it = obj.find(key);
            if (it != obj.end() && it->second.type == STRING)
                return it->second.str_val;
        }
        return def;
    }

    double getNumber(const std::string& key, double def = 0) const {
        if (type == OBJECT) {
            auto it = obj.find(key);
            if (it != obj.end() && it->second.type == NUMBER)
                return it->second.num_val;
        }
        return def;
    }

    bool getBool(const std::string& key, bool def = false) const {
        if (type == OBJECT) {
            auto it = obj.find(key);
            if (it != obj.end() && it->second.type == BOOL)
                return it->second.bool_val;
        }
        return def;
    }

    // Simple JSON serializer
    std::string serialize(int indent = 0) const {
        std::string pad(indent * 2, ' ');
        std::string childPad((indent + 1) * 2, ' ');

        switch (type) {
            case STRING: return "\"" + escapeStr(str_val) + "\"";
            case NUMBER: {
                std::ostringstream oss;
                if (num_val == (long long)num_val) oss << (long long)num_val;
                else oss << std::fixed << std::setprecision(2) << num_val;
                return oss.str();
            }
            case BOOL: return bool_val ? "true" : "false";
            case NIL: return "null";
            case OBJECT: {
                if (obj.empty()) return "{}";
                std::string out = "{\n";
                bool first = true;
                for (const auto& [k, v] : obj) {
                    if (!first) out += ",\n";
                    out += childPad + "\"" + k + "\": " + v.serialize(indent + 1);
                    first = false;
                }
                return out + "\n" + pad + "}";
            }
            case ARRAY: {
                if (arr.empty()) return "[]";
                std::string out = "[\n";
                bool first = true;
                for (const auto& v : arr) {
                    if (!first) out += ",\n";
                    out += childPad + v.serialize(indent + 1);
                    first = false;
                }
                return out + "\n" + pad + "]";
            }
        }
        return "null";
    }

private:
    static std::string escapeStr(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else out += c;
        }
        return out;
    }

public:
    // Simple parser (handles flat objects and arrays of objects)
    static JSON parse(const std::string& input) {
        size_t pos = 0;
        return parseValue(input, pos);
    }

private:
    static void skipWhitespace(const std::string& s, size_t& pos) {
        while (pos < s.size() && (s[pos]==' '||s[pos]=='\n'||s[pos]=='\r'||s[pos]=='\t')) pos++;
    }

    static std::string parseString(const std::string& s, size_t& pos) {
        pos++; // skip opening "
        std::string out;
        while (pos < s.size() && s[pos] != '"') {
            if (s[pos] == '\\') { pos++; if (pos < s.size()) out += s[pos]; }
            else out += s[pos];
            pos++;
        }
        pos++; // skip closing "
        return out;
    }

    static JSON parseValue(const std::string& s, size_t& pos) {
        skipWhitespace(s, pos);
        if (pos >= s.size()) return JSON();

        if (s[pos] == '{') return parseObject(s, pos);
        if (s[pos] == '[') return parseArray(s, pos);
        if (s[pos] == '"') {
            std::string str = parseString(s, pos);
            JSON j; j.type = STRING; j.str_val = str; return j;
        }
        if (s[pos] == 't') { pos += 4; JSON j; j.type = BOOL; j.bool_val = true; return j; }
        if (s[pos] == 'f') { pos += 5; JSON j; j.type = BOOL; j.bool_val = false; return j; }
        if (s[pos] == 'n') { pos += 4; return JSON(); }
        // number
        std::string numStr;
        while (pos < s.size() && (isdigit(s[pos])||s[pos]=='-'||s[pos]=='.'||s[pos]=='e'||s[pos]=='E'||s[pos]=='+'))
            numStr += s[pos++];
        JSON j; j.type = NUMBER; j.num_val = std::stod(numStr); return j;
    }

    static JSON parseObject(const std::string& s, size_t& pos) {
        pos++; // skip {
        JSON obj; obj.type = OBJECT;
        skipWhitespace(s, pos);
        while (pos < s.size() && s[pos] != '}') {
            skipWhitespace(s, pos);
            if (s[pos] != '"') { pos++; continue; }
            std::string key = parseString(s, pos);
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ':') pos++;
            skipWhitespace(s, pos);
            obj.obj[key] = parseValue(s, pos);
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ',') pos++;
            skipWhitespace(s, pos);
        }
        if (pos < s.size()) pos++; // skip }
        return obj;
    }

    static JSON parseArray(const std::string& s, size_t& pos) {
        pos++; // skip [
        JSON arr; arr.type = ARRAY;
        skipWhitespace(s, pos);
        while (pos < s.size() && s[pos] != ']') {
            arr.arr.push_back(parseValue(s, pos));
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ',') pos++;
            skipWhitespace(s, pos);
        }
        if (pos < s.size()) pos++; // skip ]
        return arr;
    }
};
