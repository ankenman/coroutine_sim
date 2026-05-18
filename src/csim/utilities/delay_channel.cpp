#include "csim/utilities/delay_channel.h"

#include <cassert>
#include <utility>

namespace csim {

DelayChannel::DelayChannel(sim_t& sim) : sim(sim) {}

auto
DelayChannel::push(time_ps offset, payload_ptr payload) -> void
{
    queue.emplace(sim.now() + offset, std::move(payload));
}

auto
DelayChannel::empty() const -> bool
{
    return queue.empty();
}

auto
DelayChannel::size() const -> std::size_t
{
    return queue.size();
}

auto
DelayChannel::has_ready_payload() const -> bool
{
    return !queue.empty() && queue.top().first <= sim.now();
}

auto
DelayChannel::front_time() const -> time_ps
{
    assert(!queue.empty() && "front_time on empty DelayChannel");
    return queue.top().first;
}

auto
DelayChannel::pop() -> payload_ptr
{
    assert(has_ready_payload() && "pop on DelayChannel without ready payload");
    auto payload = std::move(const_cast<entry&>(queue.top()).second);
    queue.pop();
    return payload;
}

} // namespace csim