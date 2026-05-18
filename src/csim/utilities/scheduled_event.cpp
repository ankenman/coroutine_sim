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
    [](ScheduledEvent& self, time_ps delay, uint64_t gen) -> simcpp20::process<time_ps> {
        co_await self.sim.timeout(delay);
        if (gen != self.generation)
            co_return;
        self.pending_time.reset();
        self.ev.trigger();          // trigger the existing ev, don't replace it
        self.ev = self.sim.event(); // re-arm for next use AFTER triggering
    }(*this, delay, my_gen);
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
} // namespace csim