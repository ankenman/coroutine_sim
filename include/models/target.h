#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "csim/config/knob_system.h"
#include "csim/core/module.h"
#include "csim/core/payload.h"
#include "csim/core/sim_types.h"
#include "csim/utilities/work_queue.h"
#include "protocols/chi.h"

namespace csim {

class Target : public Module {
public:
    Target(sim_t& sim, System& sys, uint32_t id, std::string name);

    auto elaborate() -> void override;
    auto start() -> void override;

private:
    KnobList&  knob_list;
    Knob<int>& clock_period_ps_knob =
        knob_list.add_knob<int>("clock_period_ps", "Clock period in picoseconds", 1000);
    Knob<int>& data_latency_cycles_knob =
        knob_list.add_knob<int>("data_latency_cycles", "Data response latency in cycles", 50);

    // Captured at elaborate:
    time_ps  clock_period_ps;
    uint32_t data_latency_cycles;

    // Constructed at elaborate:
    std::unique_ptr<WorkQueue> outbound_data;

    auto handle_request(payload_ptr payload) -> void;
    auto service_data_queue() -> proc_t;

    static auto create_data_payload(const chi::chi_fields& req_chi,
                                    uint64_t               txn_uid) -> payload_ptr;
};

} // namespace csim