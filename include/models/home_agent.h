#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "csim/core/module.h"
#include "csim/core/payload.h"
#include "csim/core/port.h"
#include "csim/core/sim_types.h"
#include "csim/utilities/work_queue.h"
#include "csim/utilities/cache.h"
#include "csim/config/knob_system.h"
#include "models/transaction_inbox.h"
#include "models/transaction_queue.h"

namespace csim {

class HomeAgent : public Module {
public:
    HomeAgent(sim_t& sim, System& sys, uint32_t id, std::string name);

    auto elaborate() -> void override;
    auto start() -> void override;

private:
    struct InboxGuard {
        HomeAgent& ha;
        txn_uid_t  txn_uid;
        ~InboxGuard() { ha.inboxes.erase(txn_uid); }
    };

    KnobList&  knob_list;
    Knob<int>& clock_period_ps_knob =
        knob_list.add_knob<int>("clock_period_ps", "Clock period in picoseconds", 1000);
    Knob<int>& downstream_target_id_knob =
        knob_list.add_knob<int>("downstream_target_id", "Target agent ID for transactions", 1);
    Knob<int>& cache_hit_latency_cycles_knob =
        knob_list.add_knob<int>("cache_hit_latency_cycles", "Cache hit latency in cycles", 3);
    Knob<int>& pipeline_latency_cycles_knob =
        knob_list.add_knob<int>("pipeline_latency_cycles", "Pipeline latency in cycles", 5);
    Knob<int>& tq_capacity_knob =
        knob_list.add_knob<int>("tq_capacity", "Number of TQ entries", 16);

    // Captured at elaborate:
    time_ps  clock_period_ps;
    uint32_t downstream_target_id;
    uint32_t cache_hit_latency_cycles;
    uint32_t pipeline_latency_cycles;

    // Constructed at elaborate:
    std::unique_ptr<WorkQueue>        outbound_req;
    std::unique_ptr<WorkQueue>        outbound_dat;
    std::unique_ptr<WorkQueue>        outbound_crsp;
    std::unique_ptr<TransactionQueue> tq;

    Cache cache;

    // Service coroutines.
    auto service_req_queue() -> proc_t;
    auto service_dat_queue() -> proc_t;
    auto service_crsp_queue() -> proc_t;

    // Inbound dispatch.
    auto handle_incoming(payload_ptr payload) -> void;
    auto handle_read_transaction(payload_ptr req) -> proc_t;
    auto handle_write_transaction(payload_ptr req) -> proc_t;
    auto allocate_dbid() -> uint16_t { return next_dbid++; }

    std::unordered_map<txn_uid_t, std::unique_ptr<TransactionInbox>> inboxes;

    uint16_t next_dbid = 0;

    // Create payload helpers
    auto create_req(const Payload& source, chi::ReqOpcode opcode, uint32_t tgt_id) -> payload_ptr;
    auto create_dat(const Payload& source, chi::DatOpcode opcode, uint32_t tgt_id) -> payload_ptr;
    auto create_snp(const Payload& source, chi::SnpOpcode opcode, uint32_t tgt_id) -> payload_ptr;
    auto create_rsp(const Payload& source, chi::RspOpcode opcode, uint32_t tgt_id,
                    uint16_t dbid = 0) -> payload_ptr;

    // Read coroutines
    auto read_miss_dmt(payload_ptr req) -> proc_t;
    auto read_miss_non_dmt(payload_ptr req) -> proc_t;
    auto read_hit(payload_ptr req) -> proc_t;

    // Generic helpers
    auto should_use_dmt(const Payload& req) -> bool;
};

} // namespace csim