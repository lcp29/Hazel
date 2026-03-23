//
// Created by helmholtz on 2026/3/22.
//

#pragma once

#include "yaml-cpp/yaml.h"

#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace Hazel
{
    class GlobalSettingRegistry
    {
    public:
        template <typename T>
        void Set(const std::string& key, const T& value);

        template <typename T>
        T Get(const std::string& key, const T& defaultValue) const;

        YAML::Node Serialize() const;
        void Deserialize(const YAML::Node& node);

    private:
        template <typename T>
        static std::string ToStoredString(const T& value);

        template <typename T>
        static bool TryParseStoredString(const std::string& storedValue, T& outValue);

        std::unordered_map<std::string, std::string> m_Registries;
    };

    extern GlobalSettingRegistry GlobalSettings;

    template <typename T>
    void GlobalSettingRegistry::Set(const std::string& key, const T& value)
    {
        m_Registries[key] = ToStoredString(value);
    }

    template <typename T>
    T GlobalSettingRegistry::Get(const std::string& key, const T& defaultValue) const
    {
        auto it = m_Registries.find(key);
        if (it == m_Registries.end())
        {
            return defaultValue;
        }

        T value{};
        if (!TryParseStoredString(it->second, value))
        {
            return defaultValue;
        }

        return value;
    }

    template <typename T>
    std::string GlobalSettingRegistry::ToStoredString(const T& value)
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            return value;
        }
        else if constexpr (std::is_same_v<T, const char*>)
        {
            return value ? std::string(value) : std::string();
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            return value ? "true" : "false";
        }
        else if constexpr (std::is_integral_v<T>)
        {
            return std::to_string(value);
        }
        else if constexpr (std::is_enum_v<T>)
        {
            using UnderlyingType = std::underlying_type_t<T>;
            return ToStoredString(static_cast<UnderlyingType>(value));
        }
        else
        {
            std::ostringstream stream;
            stream << value;
            return stream.str();
        }
    }

    template <typename T>
    bool GlobalSettingRegistry::TryParseStoredString(const std::string& storedValue, T& outValue)
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            outValue = storedValue;
            return true;
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            if (storedValue == "true" || storedValue == "1")
            {
                outValue = true;
                return true;
            }
            if (storedValue == "false" || storedValue == "0")
            {
                outValue = false;
                return true;
            }

            return false;
        }
        else if constexpr (std::is_enum_v<T>)
        {
            using UnderlyingType = std::underlying_type_t<T>;
            UnderlyingType underlyingValue{};
            if (!TryParseStoredString(storedValue, underlyingValue))
            {
                return false;
            }

            outValue = static_cast<T>(underlyingValue);
            return true;
        }
        else if constexpr (std::is_integral_v<T>)
        {
            try
            {
                size_t idx = 0;
                long long parsedValue = std::stoll(storedValue, &idx, 0);
                if (idx != storedValue.size())
                {
                    return false;
                }

                if (parsedValue < std::numeric_limits<T>::min() || parsedValue > std::numeric_limits<T>::max())
                {
                    return false;
                }

                outValue = static_cast<T>(parsedValue);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }
        else
        {
            std::istringstream stream(storedValue);
            T parsedValue{};
            stream >> parsedValue;
            if (stream.fail())
            {
                return false;
            }

            stream >> std::ws;
            if (!stream.eof())
            {
                return false;
            }

            outValue = parsedValue;
            return true;
        }
    }
} // namespace Hazel