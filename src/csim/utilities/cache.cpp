#include "csim/utilities/cache.h"

namespace csim {

auto
Cache::contains(addr_t addr) const -> bool
{
    return lines.find(align(addr)) != lines.end();
}

auto
Cache::install(addr_t addr) -> void
{
    lines[align(addr)] = Line{};
}

auto
Cache::invalidate(addr_t addr) -> void
{
    lines.erase(align(addr));
}

auto
Cache::size() const -> std::size_t
{
    return lines.size();
}

} // namespace csim