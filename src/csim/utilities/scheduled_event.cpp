#include "csim/utilities/scheduled_event.h"

namespace csim {
ScheduledEvent::ScheduledEvent(sim_t& sim) : sim(sim), ev(sim.event()) {}
auto
ScheduledEvent::notify(time_ps delay) -> void
{
    time_ps fire_at = sim.now() + delay;
    if (pending_time.has_value() && *pending_time <= fire_at)
        return;
    ++generation;
    pending_time      = fire_at;
    const auto my_gen = generation;
    fire_after_delay(delay, my_gen);
}
auto
ScheduledEvent::wait() -> simcpp20::event<time_ps>
{
    return ev;
}
void
ScheduledEvent::cancel()
{
    ++generation;
    pending_time.reset();
}

auto
ScheduledEvent::fire_after_delay(time_ps delay, uint64_t gen) -> proc_t
{
    co_await sim.timeout(delay);
    if (gen != generation)
        co_return;
    pending_time.reset();
    ev.trigger();          // trigger the existing ev, don't replace it
    ev = sim.event(); // re-arm for next use AFTER triggering
}
} // namespace csim
