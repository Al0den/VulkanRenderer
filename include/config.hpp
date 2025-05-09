#pragma once

#include <unordered_map>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>

namespace vkengine {

// Forward declarations
class Config;

// Singleton instance accessor
Config& config();

/**
 * Configuration system that allows runtime modification of engine settings
 */
class Config {
public:
    // Get instance (singleton)
    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    // Value types that can be stored in the config
    enum class ValueType {
        INT,
        FLOAT,
        BOOL,
        STRING
    };

    // Generic config value
    struct ConfigValue {
        ValueType type;
        union {
            int intValue;
            float floatValue;
            bool boolValue;
        };
        std::string stringValue; // Outside the union due to std::string requiring constructor/destructor

        // Constructors for different types
        ConfigValue() : type(ValueType::INT), intValue(0) {}
        ConfigValue(int val) : type(ValueType::INT), intValue(val) {}
        ConfigValue(float val) : type(ValueType::FLOAT), floatValue(val) {}
        ConfigValue(bool val) : type(ValueType::BOOL), boolValue(val) {}
        ConfigValue(const std::string& val) : type(ValueType::STRING), intValue(0), stringValue(val) {}

        // Safely get values with type checking
        int getInt() const { 
            if (type != ValueType::INT) throw std::runtime_error("Config value is not an integer");
            return intValue; 
        }
        
        float getFloat() const { 
            if (type != ValueType::FLOAT) throw std::runtime_error("Config value is not a float");
            return floatValue; 
        }
        
        bool getBool() const { 
            if (type != ValueType::BOOL) throw std::runtime_error("Config value is not a boolean");
            return boolValue; 
        }
        
        const std::string& getString() const { 
            if (type != ValueType::STRING) throw std::runtime_error("Config value is not a string");
            return stringValue; 
        }
    };

    // Setters
    void setInt(const std::string& key, int value);
    void setFloat(const std::string& key, float value);
    void setBool(const std::string& key, bool value);
    void setString(const std::string& key, const std::string& value);

    // Getters with defaults
    int getInt(const std::string& key, int defaultValue = 0) const;
    float getFloat(const std::string& key, float defaultValue = 0.0f) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;

    // Check if a key exists
    bool hasKey(const std::string& key) const;

    // Load/Save configuration to file
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;

    // Initialize with default settings
    void initDefaults();

    // Get all keys
    std::vector<std::string> getAllKeys() const;
    ValueType getType(const std::string& key) const;

private:
    Config() { 
        initDefaults();
    }
    ~Config() = default;

    // Delete copy/move constructors and assignment operators
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(Config&&) = delete;

    std::unordered_map<std::string, ConfigValue> values;
};

enum class RenderMode {
    NORMAL_TEXTURED,
    WIREFRAME,
    // Add other modes like WIREFRAME here later
};

// Global function to access the singleton instance
inline Config& config() {
    return Config::getInstance();
}

} // namespace vkengine
