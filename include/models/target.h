#pragma once

#include <cstdint>
#include <string>

#include "csim/core/module.h"
#include "csim/core/payload.h"
#include "csim/core/port.h"
#include "csim/core/sim_types.h"
#include "csim/utilities/work_queue.h"
#include "protocols/chi.h"

namespace csim {

class Target : public Module {
public:
    Port port;

    Target(sim_t& sim, System& sys, uint32_t id, std::string name, time_ps clock_period_ps);

    auto start() -> void override;

private:
    time_ps  clock_period_ps;
    uint32_t data_latency_cycles     = 50;
    uint32_t response_latency_cycles = 1;

    WorkQueue outbound_data;

    auto handle_request(payload_ptr payload) -> void;
    auto service_data_queue() -> proc_t;

    static auto create_data_payload(const chi::chi_fields& req_chi, uint64_t txn_uid)
        -> payload_ptr;
};

} // namespace csim