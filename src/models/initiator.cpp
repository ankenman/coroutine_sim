#include "models/initiator.h"

#include <memory>
#include <utility>
#include <cassert>

namespace csim {
Initiator::Initiator(sim_t& sim, System& sys, uint32_t id, std::string name, uint32_t home_id,
                     time_ps clock_period_ps)
    : Module(sim, sys, id, std::move(name)),
      clock_period_ps(clock_period_ps),
      home_id(home_id),
      outbound_req(sim),
      outbound_wdat(sim),
      outbound_srsp(sim)
{
    port.on_receive(this, &Initiator::handle_incoming);
}

auto
Initiator::workload() -> proc_t
{
    co_await sim.timeout(0);

    // Hazard test: two reads to the same address, back-to-back.
    issue_req(0x1000'1000ull, chi::ReqOpcode::ReadShared);
    co_await sim.timeout(clock_period_ps);
    issue_req(0x1000'1000ull, chi::ReqOpcode::ReadShared);
    co_await sim.timeout(clock_period_ps * 100);

    // First batch: all reads, all miss, populate cache.
    for (int i = 0; i < 3; ++i) {
        issue_req(0x1000ull + i * 64, chi::ReqOpcode::ReadShared);
        co_await sim.timeout(clock_period_ps);
    }

    co_await sim.timeout(clock_period_ps * 100);

    // Second batch: a write, then a read of the same line (should hit since write installed).
    issue_req(0x1000ull, chi::ReqOpcode::WriteUniqueFull);
    co_await sim.timeout(clock_period_ps * 100);
    issue_req(0x1000ull, chi::ReqOpcode::ReadShared);
    co_await sim.timeout(clock_period_ps);

    co_await sim.timeout(clock_period_ps * 100);

    // Third batch: write to a new address (not in cache), then read it.
    issue_req(0x2000ull, chi::ReqOpcode::WriteUniqueFull);
    co_await sim.timeout(clock_period_ps * 100);
    issue_req(0x2000ull, chi::ReqOpcode::ReadShared);
}
auto
Initiator::start() -> void
{
    tick_clock();
    workload();
}

auto
Initiator::tick_clock() -> proc_t
{
    co_await sim.timeout(0);

    while (true) {
        if (outbound_req.has_ready_payload()) {
            auto payload        = outbound_req.pop();
            payload->start_time = sim.now();
            const auto& chi     = payload->require_as<chi::chi_fields>();
            tracer.instant(name, "sending_req", payload->txn_uid, payload->flit_id, chi.address,
                           {{"opcode", std::string(name_of(chi.req.opcode))}});
            port.send(std::move(payload));
        }
        if (outbound_wdat.has_ready_payload()) {
            auto        payload = outbound_wdat.pop();
            const auto& chi     = payload->require_as<chi::chi_fields>();
            tracer.instant(name, "sending_wdat", payload->txn_uid, payload->flit_id, chi.address,
                           {{"opcode", std::string(name_of(chi.dat.opcode))},
                            {"dbid", std::to_string(chi.dat.dbid)}});
            outstanding_txns.erase(payload->txn_uid);
            port.send(std::move(payload));
        }
        if (outbound_srsp.has_ready_payload()) {
            auto        payload = outbound_srsp.pop();
            const auto& chi     = payload->require_as<chi::chi_fields>();
            tracer.instant(name, "sending_srsp", payload->txn_uid, payload->flit_id, chi.address,
                           {{"opcode", std::string(name_of(chi.rsp.opcode))}});
            outstanding_txns.erase(payload->txn_uid);
            port.send(std::move(payload));
        }
        co_await sim.timeout(clock_period_ps);
    }
}

auto
Initiator::handle_incoming(payload_ptr response) -> void
{
    const auto& chi_resp = response->require_as<chi::chi_fields>();

    switch (chi_resp.channel) {
    case chi::ChiChannel::RDAT:
        return handle_rdat(std::move(response));
    case chi::ChiChannel::CRSP:
        return handle_crsp(std::move(response));
    default:
        assert(false && "Initiator received unsupported channel");
    }
}
auto
Initiator::handle_rdat(payload_ptr response) -> void
{
    const auto& chi_resp = response->require_as<chi::chi_fields>();
    assert(chi_resp.dat.opcode == chi::DatOpcode::CompData);

    auto it = outstanding_txns.find(response->txn_uid);
    assert(it != outstanding_txns.end() && "received response for unknown txn_uid");

    const auto& original_request = it->second.request; // .request, was bare payload_ptr
    const auto  latency          = sim.now() - original_request->start_time;

    tracer.instant(
        name, "received_rdat", response->txn_uid, response->flit_id, chi_resp.address,
        {{"opcode", std::string(name_of(chi_resp.dat.opcode))}, {"latency", to_ns_str(latency)}});

    auto ack      = std::make_shared<Payload>(response->txn_uid);
    ack->protocol = chi::chi_fields{
        .channel = chi::ChiChannel::SRSP,
        .address = chi_resp.address,
        .txn_id  = chi_resp.txn_id,
        .rsp =
            {
                .opcode = chi::RspOpcode::CompAck,
                .src_id = id(),
                .tgt_id = home_id,
            },
    };

    outbound_srsp.push(clock_period_ps, std::move(ack));
}
auto
Initiator::handle_crsp(payload_ptr response) -> void
{
    const auto& chi_resp = response->require_as<chi::chi_fields>();
    assert(chi_resp.rsp.opcode == chi::RspOpcode::CompDBIDResp);

    auto it = outstanding_txns.find(response->txn_uid);
    assert(it != outstanding_txns.end() && "received CompDBIDResp for unknown txn_uid");

    const uint16_t dbid = chi_resp.rsp.dbid;

    it->second.dbid          = dbid;
    it->second.received_comp = true;

    tracer.instant(
        name, "received_crsp", response->txn_uid, response->flit_id, chi_resp.address,
        {{"opcode", std::string(name_of(chi_resp.rsp.opcode))}, {"dbid", std::to_string(dbid)}});

    // Build WDAT (NCBWrData) and queue.
    auto wdat      = std::make_shared<Payload>(response->txn_uid);
    wdat->protocol = chi::chi_fields{
        .channel = chi::ChiChannel::WDAT,
        .address = chi_resp.address,
        .txn_id  = chi_resp.txn_id,
        .dat =
            {
                .opcode = chi::DatOpcode::NonCopyBackWriteData,
                .src_id = id(),
                .tgt_id = home_id,
                .dbid   = dbid,
            },
    };

    outbound_wdat.push(clock_period_ps, std::move(wdat));
}
auto
Initiator::issue_req(addr_t address, chi::ReqOpcode opcode) -> void
{
    auto payload        = Payload::make_new_transaction();
    payload->start_time = sim.now();

    const uint32_t txn_id       = get_next_txn_id();
    const bool     exp_comp_ack = true; // TODO: make this not always true

    payload->protocol = chi::chi_fields{
        .channel = chi::ChiChannel::REQ,
        .address = address,
        .txn_id  = txn_id,
        .req =
            {
                .opcode       = opcode,
                .src_id       = id(),
                .tgt_id       = home_id,
                .exp_comp_ack = exp_comp_ack,
            },
    };

    tracer.instant(name, "creating_req", payload->txn_uid, payload->flit_id, address,
                   {{"opcode", std::string(name_of(opcode))}, {"txn_id", std::to_string(txn_id)}});

    outstanding_txns[payload->txn_uid] = OutstandingTxn{.request = payload};
    outbound_req.push(clock_period_ps, std::move(payload));
}
} // namespace csim