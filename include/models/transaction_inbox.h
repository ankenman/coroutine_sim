#pragma once

#include <cstdint>
#include <optional>
#include <queue>

#include "csim/core/payload.h"
#include "csim/core/sim_types.h"
#include "protocols/chi.h"

namespace csim {

class TransactionInbox {
public:
    explicit TransactionInbox(sim_t& sim);

    auto deliver(payload_ptr payload) -> void;

    auto expect_snoops(int count) -> void;
    auto expect_data_chunks(int count) -> void;

    auto arrival() -> event_t;
    auto all_snoops_received() -> event_t;
    auto all_data_chunks_received() -> event_t;

    std::queue<payload_ptr> pending;

private:
    sim_t&  sim;
    std::optional<event_t> pending_arrival;

    std::optional<int> snoops_expected;
    int                snoops_received = 0;
    event_t            snoops_done_ev;
    bool               snoops_done_triggered = false;

    std::optional<int> data_chunks_expected;
    int                data_chunks_received = 0;
    event_t            data_done_ev;
    bool               data_done_triggered = false;
};

} // namespace csim