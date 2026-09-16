#ifndef JSON_H
#define JSON_H

#include <cctype>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// A small, self-contained JSON parser -- just enough of the JSON grammar
// to read the scene files this project's own Lightbox Studio exporter
// produces (see Phase 3 in the project docs). Hand-written instead of
// pulling in a third-party library so the tracer has zero external
// dependencies.
class json_value {
public:
    enum class type { null, boolean, number, string, array, object };

    type kind = type::null;
    bool bool_val = false;
    double num_val = 0.0;
    std::string str_val;
    std::vector<json_value> arr_val;
    std::map<std::string, json_value> obj_val;

    bool is_null() const { return kind == type::null; }
    bool is_array() const { return kind == type::array; }
    bool is_object() const { return kind == type::object; }

    // Every accessor below is "safe" -- a missing field or wrong type
    // never throws, it just falls back -- since scene files are allowed
    // to omit optional fields and rely on the C++ side's own defaults.
    const json_value& at(const std::string& key) const {
        static json_value null_value;
        if (kind != type::object) return null_value;
        auto it = obj_val.find(key);
        return it == obj_val.end() ? null_value : it->second;
    }

    double as_double(double fallback = 0.0) const {
        return kind == type::number ? num_val : fallback;
    }
    std::string as_string(const std::string& fallback = "") const {
        return kind == type::string ? str_val : fallback;
    }
    bool as_bool(bool fallback = false) const {
        return kind == type::boolean ? bool_val : fallback;
    }
};

class json_parser {
public:
    explicit json_parser(const std::string& text) : s(text), pos(0) {}

    json_value parse() {
        skip_ws();
        return parse_value();
    }

private:
    const std::string& s;
    size_t pos;

    void skip_ws() {
        while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) pos++;
    }

    char peek() const { return pos < s.size() ? s[pos] : '\0'; }

    void expect(char c) {
        if (peek() != c)
            throw std::runtime_error(std::string("JSON parse error: expected '") + c +
                                      "' at position " + std::to_string(pos));
        pos++;
    }

    json_value parse_value() {
        skip_ws();
        char c = peek();
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') return parse_string_value();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == 'n') return parse_null();
        return parse_number();
    }

    json_value parse_object() {
        json_value v;
        v.kind = json_value::type::object;
        expect('{');
        skip_ws();
        if (peek() == '}') { pos++; return v; }
        while (true) {
            skip_ws();
            std::string key = parse_raw_string();
            skip_ws();
            expect(':');
            v.obj_val[key] = parse_value();
            skip_ws();
            if (peek() == ',') { pos++; continue; }
            expect('}');
            break;
        }
        return v;
    }

    json_value parse_array() {
        json_value v;
        v.kind = json_value::type::array;
        expect('[');
        skip_ws();
        if (peek() == ']') { pos++; return v; }
        while (true) {
            v.arr_val.push_back(parse_value());
            skip_ws();
            if (peek() == ',') { pos++; continue; }
            expect(']');
            break;
        }
        return v;
    }

    std::string parse_raw_string() {
        expect('"');
        std::string out;
        while (true) {
            if (pos >= s.size())
                throw std::runtime_error("JSON parse error: unterminated string");
            char c = s[pos++];
            if (c == '"') break;
            if (c == '\\') {
                if (pos >= s.size()) throw std::runtime_error("JSON parse error: bad escape");
                char e = s[pos++];
                switch (e) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case 'r': out += '\r'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'u': {
                        // Minimal \uXXXX support -- basic multilingual
                        // plane only, encoded as UTF-8. Plenty for scene
                        // labels/material names.
                        if (pos + 4 > s.size())
                            throw std::runtime_error("JSON parse error: bad unicode escape");
                        std::string hex = s.substr(pos, 4);
                        pos += 4;
                        int code = std::stoi(hex, nullptr, 16);
                        if (code < 0x80) {
                            out += static_cast<char>(code);
                        } else if (code < 0x800) {
                            out += static_cast<char>(0xC0 | (code >> 6));
                            out += static_cast<char>(0x80 | (code & 0x3F));
                        } else {
                            out += static_cast<char>(0xE0 | (code >> 12));
                            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                            out += static_cast<char>(0x80 | (code & 0x3F));
                        }
                        break;
                    }
                    default: out += e; break;
                }
            } else {
                out += c;
            }
        }
        return out;
    }

    json_value parse_string_value() {
        json_value v;
        v.kind = json_value::type::string;
        v.str_val = parse_raw_string();
        return v;
    }

    json_value parse_number() {
        size_t start = pos;
        if (peek() == '-') pos++;
        while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) pos++;
        if (peek() == '.') {
            pos++;
            while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) pos++;
        }
        if (peek() == 'e' || peek() == 'E') {
            pos++;
            if (peek() == '+' || peek() == '-') pos++;
            while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) pos++;
        }
        if (pos == start)
            throw std::runtime_error("JSON parse error: expected number at position " + std::to_string(pos));
        json_value v;
        v.kind = json_value::type::number;
        v.num_val = std::stod(s.substr(start, pos - start));
        return v;
    }

    json_value parse_bool() {
        json_value v;
        v.kind = json_value::type::boolean;
        if (s.compare(pos, 4, "true") == 0) { v.bool_val = true; pos += 4; }
        else if (s.compare(pos, 5, "false") == 0) { v.bool_val = false; pos += 5; }
        else throw std::runtime_error("JSON parse error: invalid literal at position " + std::to_string(pos));
        return v;
    }

    json_value parse_null() {
        if (s.compare(pos, 4, "null") == 0) { pos += 4; return json_value(); }
        throw std::runtime_error("JSON parse error: invalid literal at position " + std::to_string(pos));
    }
};

inline json_value parse_json(const std::string& text) {
    json_parser p(text);
    return p.parse();
}

#endif
