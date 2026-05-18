#pragma once

#include <cstddef>
#include <queue>
#include <utility>
#include <vector>

#include "../core/payload.h"
#include "scheduled_event.h"
#include "csim/core/sim_types.h"

namespace csim {

class WorkQueue {
public:
    WorkQueue(sim_t& sim, time_ps minimum_offset);

    auto push(time_ps offset, payload_ptr payload) -> void;
    auto pop() -> payload_ptr;

    [[nodiscard]] auto front() const -> const payload_ptr&;
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> std::size_t;

    // Consumer awaits this; fires when a payload is ready to pop.
    auto wait() -> simcpp20::event<uint64_t>;

private:
    sim_t&         sim;
    time_ps        minimum_offset;
    ScheduledEvent event;

    using entry = std::pair<time_ps, payload_ptr>;
    std::priority_queue<entry, std::vector<entry>, std::greater<>> queue;

    auto schedule_next_event() -> void;
};

} // namespace csim