#pragma once
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace cs2config {
    inline std::string trim(std::string_view text) {
        const auto begin = text.find_first_not_of(" \t\r\n");
        if (begin == text.npos)
            return {};
        return std::string(text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1));
    }
    inline bool identifier(std::string_view name) {
        return !name.empty() && std::all_of(name.begin(), name.end(), [](unsigned char c) {
            return std::isalnum(c) || c == '_' || c == '.' || c == '-';
        });
    }
    inline std::string quote(std::string_view value) {
        std::string result = "\"";
        for (const char c : value) {
            if (c == '\r' || c == '\n' || c == '\0')
                continue;
            if (c == '\\' || c == '"')
                result += '\\';
            result += c;
        }
        return result + '"';
    }
    struct Token {
        std::string text;
        bool brace = false;
    };
    inline std::vector<Token> tokens(std::string_view input) {
        std::vector<Token> out;
        size_t i = input.starts_with("\xEF\xBB\xBF") ? 3 : 0;
        while (i < input.size()) {
            if (std::isspace(static_cast<unsigned char>(input[i]))) {
                ++i;
                continue;
            }
            if (input.substr(i, 2) == "//") {
                const auto end = input.find('\n', i);
                i = end == input.npos ? input.size() : end + 1;
                continue;
            }
            const char first = input[i++];
            if (first == '{' || first == '}') {
                out.push_back({std::string(1, first), true});
                continue;
            }
            std::string value;
            if (first == '"') {
                bool closed = false;
                while (i < input.size()) {
                    char c = input[i++];
                    if (c == '"') {
                        closed = true;
                        break;
                    }
                    if (c == '\\' && i < input.size() && (input[i] == '"' || input[i] == '\\'))
                        c = input[i++];
                    value += c;
                }
                if (!closed)
                    throw std::runtime_error("Unclosed quote in a settings file");
            } else {
                value += first;
                while (i < input.size() && !std::isspace(static_cast<unsigned char>(input[i])) && input[i] != '{' &&
                       input[i] != '}')
                    value += input[i++];
            }
            out.push_back({std::move(value), false});
        }
        return out;
    }
    struct Snapshot {
        std::map<std::string, std::string> convars, binds, aliases, session;
        void vcfg(std::string_view source) {
            const auto input = tokens(source);
            std::vector<std::string> stack;
            for (size_t i = 0; i < input.size();) {
                if (input[i].brace) {
                    if (input[i].text != "}" || stack.empty())
                        throw std::runtime_error("Invalid settings section");
                    stack.pop_back();
                    ++i;
                    continue;
                }
                const auto name = input[i++].text;
                if (i == input.size())
                    throw std::runtime_error("Missing settings value");
                const auto value = input[i++];
                if (value.brace) {
                    if (value.text != "{")
                        throw std::runtime_error("Missing settings value");
                    stack.push_back(name);
                    continue;
                }
                if (stack.empty() || !identifier(name))
                    continue;
                if (stack.back() == "convars")
                    convars[name] = value.text;
                if (stack.back() == "bindings")
                    binds[name] = value.text;
            }
            if (!stack.empty())
                throw std::runtime_error("Unclosed settings section");
        }
        bool convarLine(std::string_view source) {
            const auto whitespace = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
            const auto skip = [&] {
                while (!source.empty() && whitespace(source.front()))
                    source.remove_prefix(1);
            };
            skip();
            if (source.starts_with("[Console]")) {
                source.remove_prefix(9);
                skip();
            }
            if (source.empty())
                return false;
            std::string name;
            if (source.front() == '"') {
                const auto end = source.find('"', 1);
                if (end == source.npos)
                    return false;
                name = source.substr(1, end - 1);
                source.remove_prefix(end + 1);
            } else {
                const auto end = source.find_first_of(" \t\r\n:=");
                if (end == source.npos)
                    return false;
                name = source.substr(0, end);
                source.remove_prefix(end);
            }
            if (!identifier(name) || (!std::isalpha(static_cast<unsigned char>(name.front())) && name.front() != '_') ||
                name == "bind" || name == "alias" || name == "echo" || name == "ConVars")
                return false;
            if (source.empty() || (!whitespace(source.front()) && source.front() != ':' && source.front() != '='))
                return false;
            skip();
            if (!source.empty() && (source.front() == ':' || source.front() == '=')) {
                source.remove_prefix(1);
                skip();
            }
            if (source.empty())
                return false;
            std::string value;
            if (source.front() == '"') {
                source.remove_prefix(1);
                bool closed = false;
                while (!source.empty()) {
                    char c = source.front();
                    source.remove_prefix(1);
                    if (c == '"') {
                        closed = true;
                        break;
                    }
                    if (c == '\\' && !source.empty() && (source.front() == '"' || source.front() == '\\')) {
                        c = source.front();
                        source.remove_prefix(1);
                    }
                    value += c;
                }
                if (!closed)
                    return false;
            } else if (source.front() == ':' && source.find(" : ") != source.npos) {
                value.clear();
            } else if (const auto column = source.find(" : ");
                       column != source.npos && source.find(" : ", column + 3) != source.npos) {
                value = trim(source.substr(0, column));
                if (value == "cmd")
                    return false;
                source.remove_prefix(column);
            } else {
                const auto end = source.find_first_of(" \t\r\n:");
                const auto number = source.substr(0, end);
                if (number.empty())
                    return false;
                if (number != "true" && number != "false") {
                    auto numeric = number;
                    if (numeric.front() == '+')
                        numeric.remove_prefix(1);
                    double parsed = 0;
                    const auto result = std::from_chars(numeric.data(), numeric.data() + numeric.size(), parsed);
                    if (result.ec != std::errc{} || result.ptr != numeric.data() + numeric.size() ||
                        !std::isfinite(parsed))
                        return false;
                }
                value = number;
                source.remove_prefix(number.size());
            }
            if (!source.empty() && !whitespace(source.front()) && source.front() != ':')
                return false;
            session[name] = value;
            if (auto saved = convars.find(name); saved != convars.end())
                saved->second = value;
            return true;
        }
        bool bindLine(std::string_view source) {
            auto line = trim(source);
            if (line.starts_with("[Console]"))
                line = trim(line.substr(9));
            static const std::regex binding(R"cfg(^(?:bind\s+)?"([^"]+)"\s*(?:=\s*)?"((?:\\.|[^"\\])*)"\s*$)cfg");
            std::smatch match;
            if (!std::regex_match(line, match, binding)) {
                static const std::regex bare(
                    R"cfg(^(?:bind\s+)?([A-Za-z0-9_.-]+)\s*(?:=\s*)?"((?:\\.|[^"\\])*)"\s*$)cfg");
                if (!std::regex_match(line, match, bare))
                    return false;
            }
            if (!identifier(match[1].str()))
                return false;
            const auto values = tokens('"' + match[2].str() + '"');
            if (values.size() != 1)
                return false;
            binds[match[1].str()] = values[0].text;
            return true;
        }
        bool aliasLine(std::string_view source) {
            auto line = trim(source);
            if (line.starts_with("[Console]"))
                line = trim(line.substr(9));
            if (line.starts_with("alias "))
                line = trim(line.substr(6));
            static const std::regex alias(R"cfg(^"?([A-Za-z_][A-Za-z0-9_.]*)"?\s*:\s*(.*)$)cfg");
            std::smatch match;
            if (!std::regex_match(line, match, alias))
                return false;
            auto value = trim(match[2].str());
            if (value.starts_with('"')) {
                const auto parts = tokens(value);
                if (parts.size() != 1)
                    return false;
                value = parts[0].text;
            }
            aliases[match[1].str()] = value;
            return true;
        }
        std::string cfg() const {
            std::string out = "// ErScripts export\n";
            for (const auto &[name, value] : aliases)
                out += "alias " + quote(name) + " " + quote(value) + "\n";
            for (const auto &[name, value] : convars)
                out += name + " " + quote(value) + "\n";
            for (const auto &[name, value] : binds)
                out += "bind " + quote(name) + " " + quote(value) + "\n";
            return out;
        }
    };
    inline std::string read(const std::filesystem::path &path) {
        if (std::filesystem::file_size(path) > 4 * 1024 * 1024)
            throw std::runtime_error("Settings file is too large");
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
            throw std::runtime_error("Cannot read a settings file");
        return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    }
    inline void write(const std::filesystem::path &path, std::string_view text) {
        std::ofstream file(path, std::ios::binary);
        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        file.close();
        if (!file)
            throw std::runtime_error("Cannot save the export");
    }
} // namespace cs2config
