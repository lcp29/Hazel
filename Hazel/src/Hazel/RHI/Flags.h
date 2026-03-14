//
// Created by helmholtz on 2026/3/14.
//

#pragma once

namespace Hazel
{
    template<typename BitType>
    class Flags
    {
    public:
        using UnderlyingType = std::underlying_type_t<BitType>;

        UnderlyingType value;

        Flags() : value(0) {}

        Flags(UnderlyingType value) : value(value) {}

        Flags(BitType value) : value(static_cast<UnderlyingType>(value)) {}

        Flags(const Flags &other) : value(other.value) {}

        operator UnderlyingType() const { return value; }

        operator bool() const { return value != 0; }

        Flags operator|(const Flags &rhs)
        {
            return static_cast<UnderlyingType>(value) | static_cast<UnderlyingType>(rhs.value);
        }

        Flags operator|(const UnderlyingType &rhs)
        {
            return static_cast<UnderlyingType>(value) | rhs.value;
        }

        Flags operator&(const Flags &rhs)
        {
            return static_cast<UnderlyingType>(value) | static_cast<UnderlyingType>(rhs.value);
        }

        bool operator==(const Flags &) const = default;
    };

    template<typename BitType, typename UnderlyingType = std::underlying_type_t<BitType>>
    Flags<BitType> operator|(const UnderlyingType &lhs, const Flags<BitType> &rhs)
    {
        return rhs | lhs;
    }

    template<typename BitType, typename UnderlyingType = std::underlying_type_t<BitType>>
    Flags<BitType> operator&(const UnderlyingType &lhs, const Flags<BitType> &rhs)
    {
        return rhs | lhs;
    }
} // Hazel
