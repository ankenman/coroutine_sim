#pragma once

#include <cstdint>
#include <fstream>
#include <map>
#include <optional>
#include <string>

#include "csim/core/sim_types.h"

namespace csim::tracing {

struct Event {
    time_ps                            timestamp;
    std::string                        component_name;
    std::string                        event_name;
    std::optional<uint64_t>            txn_uid;
    std::optional<addr_t>              address;
    std::map<std::string, std::string> args;
};

class Tracer {
public:
    static auto instance() -> Tracer&;

    // Open output files and bind to a simulation. Must be called before
    // any logging. Both paths can be the same file (not recommended) or
    // empty strings to skip that output.
    auto open(const std::string& jsonl_path, const std::string& text_path, sim_t& sim) -> void;

    auto close() -> void;

    auto               enable() -> void;
    auto               disable() -> void;
    [[nodiscard]] auto is_enabled() const -> bool;
    // clang-format off
    // Emit an instant event at the current sim time.
    auto instant(std::string                        component_name,
                 std::string                        event_name,
                 uint64_t                           txn_uid,
                 std::optional<uint64_t>            flit_id = {},
                 std::optional<addr_t>              address = {},
                 std::map<std::string, std::string> args    = {}) -> void;

    // Non-copyable, non-movable.
    Tracer(const Tracer&)                    = delete;
    auto operator=(const Tracer&) -> Tracer& = delete;
    // clang-format on
private:
    Tracer() = default;
    ~Tracer(); // ensures files are flushed/closed at program exit

    sim_t*        sim_ptr = nullptr;
    std::ofstream jsonl_out;
    std::ofstream text_out;
    bool          enabled = false;
};
} // namespace csim::tracing