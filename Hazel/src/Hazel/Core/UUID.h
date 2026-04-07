#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace Hazel
{
    class UUID
    {
    public:
        UUID();
        UUID(uint64_t uuid);
        UUID(const UUID&) = default;

        operator uint64_t() const
        {
            return m_UUID;
        }

        bool operator==(const UUID& other) const
        {
            return m_UUID == other.m_UUID;
        }

    private:
        uint64_t m_UUID;
    };
} // namespace Hazel

template <>
struct std::hash<Hazel::UUID>
{
    size_t operator()(const Hazel::UUID& uuid) const noexcept
    {
        return hash<uint64_t>{}(uuid);
    }
}; // namespace std