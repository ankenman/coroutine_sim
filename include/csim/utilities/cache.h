#pragma once

#include <cstdint>
#include <unordered_map>

#include "csim/core/sim_types.h"

namespace csim {

// Fully-associative metadata-only cache for 64-byte lines. Callers pass
// any byte address; the cache aligns to a line boundary internally.
//
// Currently unbounded with no eviction. A capacity limit and LRU policy
// can be added later without changing the public API.
class Cache {
public:
    static constexpr uint64_t line_bytes = 64;

    struct Line {
        bool valid = true;
        // Future: coherence state, owners, last-access tick, etc.
    };

    Cache() = default;

    [[nodiscard]] auto contains(addr_t addr) const -> bool;
    auto               install(addr_t addr) -> void;
    auto               invalidate(addr_t addr) -> void;

    [[nodiscard]] auto size() const -> std::size_t;

private:
    static auto align(addr_t addr) -> addr_t { return addr & ~(line_bytes - 1); }

    std::unordered_map<addr_t, Line> lines;
};

} // namespace csim