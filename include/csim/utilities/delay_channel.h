#pragma once

#include <cstddef>
#include <queue>
#include <utility>
#include <vector>

#include "csim/core/payload.h"
#include "csim/core/sim_types.h"

namespace csim {

// Time-stamped priority queue of payloads. Unlike WorkQueue, this is
// passive — no events, no scheduling. The consumer polls on its own
// schedule (typically from a clocked tick loop) and drains whatever
// is ready.
//
// Useful for cycle-driven modules that already have a periodic wake-up
// and want a "what's ready right now?" interface rather than an
// event-driven "wake me when something's ready" interface.
class DelayChannel {
public:
    explicit DelayChannel(sim_t& sim);

    auto push(time_ps offset, payload_ptr payload) -> void;

    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;

    // True iff queue is non-empty and the head's ready time is <= now.
    [[nodiscard]] auto has_ready_payload() const -> bool;

    // Peek at the head. Caller must check empty() first.
    [[nodiscard]] auto front_time() const -> time_ps;

    // Pop the head and return it. Caller must check has_ready_payload()
    // (or accept popping a not-yet-ready entry, which is a programming
    // error this assert catches).
    auto pop() -> payload_ptr;

private:
    sim_t& sim;

    using entry = std::pair<time_ps, payload_ptr>;
    std::priority_queue<entry, std::vector<entry>, std::greater<>> queue;
};

} // namespace csim