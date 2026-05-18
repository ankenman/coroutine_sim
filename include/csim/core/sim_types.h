#pragma once

#include <sstream>
#include <fschuetz04/simcpp20.hpp>
#include <string>
#include <iomanip>
#include <magic_enum/magic_enum.hpp>

namespace csim {
// Project-wide time type. Change here, change everywhere.
using time_ps   = uint64_t;
using addr_t    = uint64_t;
using txn_uid_t = uint64_t;
using sim_t     = simcpp20::simulation<time_ps>;
using proc_t    = simcpp20::process<time_ps>;
using event_t   = simcpp20::event<time_ps>;

inline auto
to_ns(const time_ps t) -> double
{
    return t / 1000.0;
}

inline auto
to_ns_str(const time_ps t) -> std::string
{
    std::stringstream ns_stream;
    ns_stream << std::fixed << std::setprecision(2) << to_ns(t) << " ns";
    return ns_stream.str();
}

// clang-format off
constexpr time_ps operator""_ps(unsigned long long v) { return v; }
constexpr time_ps operator""_ns(unsigned long long v) { return v * 1000; }
constexpr time_ps operator""_us(unsigned long long v) { return v * 1000000; }
// clang-format on

template <typename E>
auto
name_of(E value) -> std::string_view
{
    return magic_enum::enum_name(value);
}

} // namespace csim