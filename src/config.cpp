#include "../include/config.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace vkengine {

void Config::setInt(const std::string& key, int value) {
    values[key] = ConfigValue(value);
}

void Config::setFloat(const std::string& key, float value) {
    values[key] = ConfigValue(value);
}

void Config::setBool(const std::string& key, bool value) {
    values[key] = ConfigValue(value);
}

void Config::setString(const std::string& key, const std::string& value) {
    values[key] = ConfigValue(value);
}

int Config::getInt(const std::string& key, int defaultValue) const {
    auto it = values.find(key);
    if (it != values.end() && it->second.type == ValueType::INT) {
        return it->second.intValue;
    }
    return defaultValue;
}

float Config::getFloat(const std::string& key, float defaultValue) const {
    auto it = values.find(key);
    if (it != values.end() && it->second.type == ValueType::FLOAT) {
        return it->second.floatValue;
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
    auto it = values.find(key);
    if (it != values.end() && it->second.type == ValueType::BOOL) {
        return it->second.boolValue;
    }
    return defaultValue;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = values.find(key);
    if (it != values.end() && it->second.type == ValueType::STRING) {
        return it->second.stringValue;
    }
    return defaultValue;
}

bool Config::hasKey(const std::string& key) const {
    return values.find(key) != values.end();
}

bool Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    values.clear();
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '=')) {
            std::string value;
            if (std::getline(is_line, value)) {
                // Trim spaces
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                // Determine value type
                if (value == "true" || value == "false") {
                    setBool(key, value == "true");
                } else if (value.find('.') != std::string::npos) {
                    try {
                        setFloat(key, std::stof(value));
                    } catch (const std::exception&) {
                        setString(key, value);
                    }
                } else {
                    try {
                        setInt(key, std::stoi(value));
                    } catch (const std::exception&) {
                        setString(key, value);
                    }
                }
            }
        }
    }

    file.close();
    return true;
}

bool Config::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << "# VulkanRenderer Configuration File\n";
    file << "# Generated automatically\n\n";

    for (const auto& pair : values) {
        file << pair.first << " = ";
        
        switch (pair.second.type) {
            case ValueType::INT:
                file << pair.second.intValue;
                break;
            case ValueType::FLOAT:
                file << pair.second.floatValue;
                break;
            case ValueType::BOOL:
                file << (pair.second.boolValue ? "true" : "false");
                break;
            case ValueType::STRING:
                file << pair.second.stringValue;
                break;
        }
        
        file << "\n";
    }

    file.close();
    return true;
}

void Config::initDefaults() {
    // Graphics settings
    setInt("render_distance", 6);
    setInt("meshing_technique", 0); // 0: Simple, 1: Greedy
    setFloat("player_speed", 30.0f);
    setFloat("fov", 60.0f);
    setInt("render_mode", static_cast<int>(RenderMode::NORMAL_TEXTURED));
}

std::vector<std::string> Config::getAllKeys() const {
    std::vector<std::string> keys;
    keys.reserve(values.size());
    
    for (const auto& pair : values) {
        keys.push_back(pair.first);
    }
    
    return keys;
}

Config::ValueType Config::getType(const std::string& key) const {
    auto it = values.find(key);
    if (it != values.end()) {
        return it->second.type;
    }
    throw std::runtime_error("Config key not found: " + key);
}
} // namespace vkengine
