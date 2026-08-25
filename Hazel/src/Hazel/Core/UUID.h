#pragma once

// ======== Aster Modify Begin ========
#include <cstddef>
#include <cstdint>
#include <functional>

// ======== Aster Modify End ========

namespace Hazel
{
    class UUID
    {
      public:
        UUID();
        UUID(uint64_t uuid);
        UUID(const UUID&) = default;

        operator uint64_t() const { return m_UUID; }

        // ======== Aster Modify Begin ========
        bool operator==(const UUID& other) const { return m_UUID == other.m_UUID; }

        // ======== Aster Modify End ========

      private:
        uint64_t m_UUID;
    };
} // namespace Hazel

// ======== Aster Modify Begin ========
template <> struct std::hash<Hazel::UUID>
{
    size_t operator()(const Hazel::UUID& uuid) const noexcept { return hash<uint64_t>{}(uuid); }

    // ======== Aster Modify End ========
}; // namespace std