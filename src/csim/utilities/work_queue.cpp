#include "csim/utilities/work_queue.h"

#include <cassert>
#include <utility>

namespace csim {

WorkQueue::WorkQueue(sim_t& sim, time_ps minimum_offset)
    : sim(sim), minimum_offset(minimum_offset), event(sim)
{
}

auto
WorkQueue::push(time_ps offset, payload_ptr payload) -> void
{
    queue.emplace(sim.now() + offset, std::move(payload));
    schedule_next_event();
}

auto
WorkQueue::pop() -> payload_ptr
{
    assert(!queue.empty() && "pop on empty WorkQueue");
    auto payload = std::move(const_cast<entry&>(queue.top()).second);
    queue.pop();
    schedule_next_event();
    return payload;
}

auto
WorkQueue::front() const -> const payload_ptr&
{
    assert(!queue.empty() && "front on empty WorkQueue");
    return queue.top().second;
}

auto
WorkQueue::empty() const -> bool
{
    return queue.empty();
}

auto
WorkQueue::size() const -> std::size_t
{
    return queue.size();
}

auto
WorkQueue::wait() -> simcpp20::event<uint64_t>
{
    return event.wait();
}

auto
WorkQueue::schedule_next_event() -> void
{
    if (queue.empty()) {
        return;
    }
    const auto head_time = queue.top().first;
    time_ps    offset;
    if (head_time <= sim.now() + minimum_offset) {
        offset = minimum_offset;
    }
    else {
        offset = head_time - sim.now();
    }
    event.notify(offset);
}

} // namespace csim