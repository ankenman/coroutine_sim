#include "models/transaction_inbox.h"

#include <cassert>
#include <utility>

namespace csim {

TransactionInbox::TransactionInbox(sim_t& sim)
    : sim(sim), snoops_done_ev(sim.event()), data_done_ev(sim.event())
{
}

auto
TransactionInbox::deliver(payload_ptr payload) -> void
{
    const auto& chi     = payload->require_as<chi::chi_fields>();
    const auto  channel = chi.channel;

    pending.push(std::move(payload));

    // Trigger any pending arrival.
    if (pending_arrival.has_value()) {
        pending_arrival->trigger();
        pending_arrival.reset();
    }

    switch (channel) {
    case chi::ChiChannel::SRSP:
        // Stubbed — snoop responses will distinguish from CompAck
        // when snoops are modeled.
        break;
    case chi::ChiChannel::WDAT:
    case chi::ChiChannel::RDAT:
        data_chunks_received++;
        if (data_chunks_expected.has_value()) {
            assert(data_chunks_received <= *data_chunks_expected &&
                   "more data chunks received than expected");
            if (data_chunks_received == *data_chunks_expected && !data_done_triggered) {
                data_done_ev.trigger();
                data_done_triggered = true;
            }
        }
        break;
    case chi::ChiChannel::REQ:
    case chi::ChiChannel::CRSP:
    case chi::ChiChannel::SNP:
        // Not counted. Coroutine processes via arrival().
        break;
    }
}

auto
TransactionInbox::expect_snoops(int count) -> void
{
    assert(!snoops_expected.has_value() && "expect_snoops called twice");
    assert(count >= 0 && "negative snoop count");
    snoops_expected = count;

    if (snoops_received >= count && !snoops_done_triggered) {
        snoops_done_ev.trigger();
        snoops_done_triggered = true;
    }
}

auto
TransactionInbox::expect_data_chunks(int count) -> void
{
    assert(!data_chunks_expected.has_value() && "expect_data_chunks called twice");
    assert(count >= 0 && "negative data chunk count");
    data_chunks_expected = count;

    if (data_chunks_received >= count && !data_done_triggered) {
        data_done_ev.trigger();
        data_done_triggered = true;
    }
}

auto
TransactionInbox::arrival() -> event_t
{
    assert(!pending_arrival.has_value() && "arrival() called while previous arrival is pending");

    auto ev = sim.event();
    if (!pending.empty()) {
        ev.trigger();
    }
    else {
        pending_arrival = ev;
    }
    return ev;
}

auto
TransactionInbox::all_snoops_received() -> event_t
{
    assert(snoops_expected.has_value() && "all_snoops_received awaited before expect_snoops");
    return snoops_done_ev;
}

auto
TransactionInbox::all_data_chunks_received() -> event_t
{
    assert(data_chunks_expected.has_value() &&
           "all_data_chunks_received awaited before expect_data_chunks");
    return data_done_ev;
}

} // namespace csim