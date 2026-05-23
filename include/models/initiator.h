#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <optional>

#include "csim/utilities/delay_channel.h"
#include "csim/core/module.h"
#include "csim/core/payload.h"
#include "csim/core/port.h"
#include "csim/core/sim_types.h"
#include "protocols/chi.h"
#include "csim/config/knob_system.h"

namespace csim {

class Initiator : public Module {
public:
    Initiator(sim_t& sim, System& sys, uint32_t id, std::string name);

    auto elaborate() -> void override;
    auto start() -> void override;

private:
    auto tick_clock() -> proc_t;
    auto workload() -> proc_t;
    auto issue_req(addr_t address, chi::ReqOpcode) -> void;
    auto handle_incoming(payload_ptr payload) -> void;
    auto handle_rdat(payload_ptr response) -> void;
    auto handle_crsp(payload_ptr response) -> void;
    auto get_next_txn_id() -> uint32_t { return next_txn_id++; }

    KnobList&  knob_list;
    Knob<int>& clock_period_ps_knob =
        knob_list.add_knob<int>("clock_period_ps", "Clock period in picoseconds", 1000);
    Knob<int>& home_id_knob =
        knob_list.add_knob<int>("home_id", "Home agent ID for transactions", 1);

    time_ps  clock_period_ps;
    uint32_t home_id;

    uint32_t next_txn_id = 0;

    DelayChannel outbound_req;
    DelayChannel outbound_wdat;
    DelayChannel outbound_srsp;

    struct OutstandingTxn {
        payload_ptr             request;
        std::optional<uint16_t> dbid;
        bool                    received_comp = false;
    };
    std::unordered_map<uint64_t, OutstandingTxn> outstanding_txns;
};

} // namespace csim