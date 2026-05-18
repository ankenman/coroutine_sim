#pragma once

#include "fschuetz04/simcpp20.hpp"
#include <optional>
#include <cstdint>
#include "csim/core/sim_types.h"

namespace csim {
struct ScheduledEvent {
    sim_t& sim;
    uint64_t                       generation = 0;
    simcpp20::event<time_ps>       ev;
    std::optional<time_ps>         pending_time;

    explicit ScheduledEvent(sim_t& sim);
    auto notify(time_ps delay) -> void;
    auto wait() -> simcpp20::event<time_ps>;
    void cancel();
};
} // namespace csim