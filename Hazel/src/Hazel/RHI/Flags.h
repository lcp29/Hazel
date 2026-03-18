//
// Created by helmholtz on 2026/3/14.
//

#pragma once

#include <type_traits>

namespace Hazel
{
    template<typename T>
    struct InRHIFlagScope : std::false_type {};

    template<typename BitType>
    requires InRHIFlagScope<BitType>::value
    class Flags
    {
    public:
        using UnderlyingType = std::underlying_type_t<BitType>;

        UnderlyingType value;

        Flags() : value(0) {}

        constexpr Flags(const UnderlyingType &value) : value(value) {}

        constexpr Flags(const BitType &value) : value(static_cast<UnderlyingType>(value)) {}

        constexpr Flags(const Flags &other) : value(other.value) {}

        constexpr operator UnderlyingType() const { return value; }

        constexpr operator bool() const { return value != 0; }

        constexpr Flags operator|(const Flags &rhs) const
        {
            return static_cast<UnderlyingType>(value) | static_cast<UnderlyingType>(rhs.value);
        }

        constexpr Flags operator|(const UnderlyingType &rhs) const
        {
            return static_cast<UnderlyingType>(value) | rhs;
        }

        constexpr Flags operator&(const Flags &rhs) const
        {
            return static_cast<UnderlyingType>(value) & static_cast<UnderlyingType>(rhs.value);
        }

        bool operator==(const Flags &) const = default;
    };

    template<typename BitType>
    requires InRHIFlagScope<BitType>::value
    constexpr Flags<BitType> operator|(const BitType &lhs, const Flags<BitType> &rhs)
    {
        return rhs | lhs;
    }

    template<typename BitType, typename UnderlyingType = std::underlying_type_t<BitType>>
    requires InRHIFlagScope<BitType>::value
    Flags<BitType> operator|(const BitType &lhs, const BitType &rhs)
    {
        return static_cast<UnderlyingType>(lhs) | static_cast<UnderlyingType>(rhs);
    }

    template<typename BitType>
    requires InRHIFlagScope<BitType>::value
    constexpr Flags<BitType> operator&(const BitType &lhs, const Flags<BitType> &rhs)
    {
        return Flags<BitType>(lhs) & rhs;
    }

    template<typename BitType, typename UnderlyingType = std::underlying_type_t<BitType>>
    requires InRHIFlagScope<BitType>::value
    Flags<BitType> operator&(const BitType &lhs, const BitType &rhs)
    {
        return static_cast<UnderlyingType>(lhs) & static_cast<UnderlyingType>(rhs);
    }
} // Hazel
