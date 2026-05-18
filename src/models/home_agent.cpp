#include "models/home_agent.h"

#include <cassert>
#include <memory>
#include <utility>

#include "protocols/chi.h"

namespace csim {

HomeAgent::HomeAgent(sim_t& sim, System& sys, uint32_t id, std::string name,
                     uint32_t downstream_target_id, time_ps clock_period_ps)
    : Module(sim, sys, id, std::move(name)),
      downstream_target_id(downstream_target_id),
      clock_period_ps(clock_period_ps),
      outbound_req(sim, clock_period_ps),
      outbound_dat(sim, clock_period_ps),
      outbound_crsp(sim, clock_period_ps),
      tq(sim, 8)
{
    port.on_receive(this, &HomeAgent::handle_incoming);
}

auto
HomeAgent::start() -> void
{
    service_req_queue();
    service_dat_queue();
    service_crsp_queue();
}

auto
HomeAgent::handle_incoming(payload_ptr payload) -> void
{
    const auto& chi = payload->require_as<chi::chi_fields>();

    if (chi.channel == chi::ChiChannel::REQ) {
        auto [it, inserted] =
            inboxes.emplace(payload->txn_uid, std::make_unique<TransactionInbox>(sim));
        assert(inserted && "REQ with duplicate txn_uid");

        if (chi::is_read_opcode(chi.req.opcode)) {
            handle_read_transaction(std::move(payload));
        }
        else if (chi::is_write_opcode(chi.req.opcode)) {
            handle_write_transaction(std::move(payload));
        }
        else {
            assert(false && "HomeAgent received unsupported REQ opcode");
        }
        return;
    }

    // Non-REQ: route to existing inbox by txn_uid.
    auto it = inboxes.find(payload->txn_uid);
    assert(it != inboxes.end() && "non-REQ flit with no matching inbox");
    it->second->deliver(std::move(payload));
}

auto
HomeAgent::handle_read_transaction(payload_ptr req) -> proc_t
{
    const auto&  chi_in  = req->require_as<chi::chi_fields>();
    const addr_t address = chi_in.address;

    TQEntry tq_entry{tq, req};
    tracer.instant(name, "tq waiting for grant", req->txn_uid, req->flit_id, address);
    co_await tq_entry.wait_for_grant();
    tracer.instant(name, "tq_granted - entering pipeline", req->txn_uid, req->flit_id, address);

    auto&      inbox = *inboxes[req->txn_uid];
    InboxGuard guard{.ha = *this, .txn_uid = req->txn_uid};

    const bool hit = cache.contains(address);

    if (hit) {
        tracer.instant(name, "req_hit", req->txn_uid, req->flit_id, address,
                       {{"opcode", std::string(name_of(chi_in.req.opcode))}});
        co_await read_hit(req);
    }
    else {
        tracer.instant(name, "req_miss", req->txn_uid, req->flit_id, address,
                       {{"opcode", std::string(name_of(chi_in.req.opcode))}});
        if (should_use_dmt(*req)) {
            co_await read_miss_dmt(req);
        }
        else {
            co_await read_miss_non_dmt(req);
        }
    }

    // Common: wait for CompAck.
    if (chi_in.req.exp_comp_ack) {
        co_await inbox.arrival();
        assert(!inbox.pending.empty());
        auto compack = std::move(inbox.pending.front());
        inbox.pending.pop();

        tracer.instant(
            name, "compack_received", req->txn_uid, compack->flit_id, address,
            {{"opcode", std::string(name_of(compack->require_as<chi::chi_fields>().rsp.opcode))}});
    }
}

auto
HomeAgent::read_miss_non_dmt(payload_ptr req) -> proc_t
{
    const auto&  chi_in  = req->require_as<chi::chi_fields>();
    const addr_t address = chi_in.address;
    auto&        inbox   = *inboxes[req->txn_uid];

    // Forward REQ to target.
    auto          forward_req = create_req(*req, chi::ReqOpcode::ReadNoSnp, downstream_target_id);
    const time_ps fwd_offset  = static_cast<time_ps>(pipeline_latency_cycles) * clock_period_ps;
    outbound_req.push(fwd_offset, std::move(forward_req));

    // Wait for RDAT from target.
    inbox.expect_data_chunks(1);
    co_await inbox.all_data_chunks_received();

    assert(!inbox.pending.empty());
    auto rdat_payload = std::move(inbox.pending.front());
    inbox.pending.pop();

    cache.install(address);

    tracer.instant(
        name, "received_rdat", req->txn_uid, rdat_payload->flit_id, address,
        {{"opcode", std::string(name_of(rdat_payload->require_as<chi::chi_fields>().dat.opcode))}});

    // Build and queue CompData to requester.
    auto          data_return = create_dat(*req, chi::DatOpcode::CompData, chi_in.req.src_id);
    const time_ps dat_offset  = static_cast<time_ps>(pipeline_latency_cycles) * clock_period_ps;
    outbound_dat.push(dat_offset, std::move(data_return));
}

auto
HomeAgent::read_miss_dmt(payload_ptr req) -> proc_t
{
    const auto& chi_in = req->require_as<chi::chi_fields>();
    auto&       inbox  = *inboxes[req->txn_uid];

    // TODO: forward REQ with DMT directives (return_n_id = requester).
    // For now, this is the same as non-DMT forward.
    auto          forward_req = create_req(*req, chi::ReqOpcode::ReadNoSnp, downstream_target_id);
    const time_ps fwd_offset  = static_cast<time_ps>(pipeline_latency_cycles) * clock_period_ps;
    outbound_req.push(fwd_offset, std::move(forward_req));

    // Wait for Comp from target (no data — data went directly to requester).
    // TODO: distinguish Comp vs CompData. For now, target sends CompData; assume it.
    co_await inbox.arrival();
    assert(!inbox.pending.empty());
    auto target_response = std::move(inbox.pending.front());
    inbox.pending.pop();

    tracer.instant(
        name, "received_dmt_comp", req->txn_uid, target_response->flit_id, chi_in.address,
        {{"opcode",
          std::string(name_of(target_response->require_as<chi::chi_fields>().dat.opcode))}});

    // HA does NOT send CompData. Requester already has it (in DMT).
}

auto
HomeAgent::read_hit(payload_ptr req) -> proc_t
{
    const auto& chi_in = req->require_as<chi::chi_fields>();

    auto          data_return = create_dat(*req, chi::DatOpcode::CompData, chi_in.req.src_id);
    const time_ps dat_offset  = static_cast<time_ps>(cache_hit_latency_cycles) * clock_period_ps;
    outbound_dat.push(dat_offset, std::move(data_return));

    co_return;
}

auto
HomeAgent::handle_write_transaction(payload_ptr req) -> proc_t
{
    const auto& chi_in  = req->require_as<chi::chi_fields>();
    const auto  address = chi_in.address;

    TQEntry tq_entry{tq, req};
    tracer.instant(name, "tq_acquired", req->txn_uid, req->flit_id, address);
    co_await tq_entry.wait_for_grant();
    tracer.instant(name, "tq_granted", req->txn_uid, req->flit_id, address);

    auto&      inbox = *inboxes[req->txn_uid];
    InboxGuard guard{.ha = *this, .txn_uid = req->txn_uid};

    const uint16_t dbid = allocate_dbid();

    tracer.instant(
        name, "received_req", req->txn_uid, req->flit_id, address,
        {{"opcode", std::string(name_of(chi_in.req.opcode))}, {"dbid", std::to_string(dbid)}});

    auto comp_dbid       = create_rsp(*req, chi::RspOpcode::CompDBIDResp, chi_in.req.src_id, dbid);
    const time_ps offset = static_cast<time_ps>(pipeline_latency_cycles) * clock_period_ps;
    outbound_crsp.push(offset, std::move(comp_dbid));

    inbox.expect_data_chunks(1);
    co_await inbox.all_data_chunks_received();

    assert(!inbox.pending.empty());
    auto wdat = std::move(inbox.pending.front());
    inbox.pending.pop();

    const auto& wdat_chi = wdat->require_as<chi::chi_fields>();
    assert(wdat_chi.dat.dbid == dbid && "WDAT DBID mismatch");

    cache.install(address);

    tracer.instant(
        name, "wdat_received", req->txn_uid, wdat->flit_id, address,
        {{"opcode", std::string(name_of(wdat_chi.dat.opcode))}, {"dbid", std::to_string(dbid)}});
}

auto
HomeAgent::service_req_queue() -> proc_t
{
    while (true) {
        co_await outbound_req.wait();
        auto payload = outbound_req.pop();

        const auto& chi = payload->require_as<chi::chi_fields>();
        tracer.instant(name, "sending_req", payload->txn_uid, payload->flit_id, chi.address,
                       {{"opcode", std::string(name_of(chi.req.opcode))},
                        {"tgt_id", std::to_string(chi.req.tgt_id)}});

        port.send(std::move(payload));
    }
}

auto
HomeAgent::service_dat_queue() -> proc_t
{
    while (true) {
        co_await outbound_dat.wait();
        auto payload = outbound_dat.pop();

        const auto& chi = payload->require_as<chi::chi_fields>();
        tracer.instant(name, "sending_rdat", payload->txn_uid, payload->flit_id, chi.address,
                       {{"opcode", std::string(name_of(chi.dat.opcode))},
                        {"tgt_id", std::to_string(chi.dat.tgt_id)}});

        port.send(std::move(payload));
    }
}
auto
HomeAgent::service_crsp_queue() -> proc_t
{
    while (true) {
        co_await outbound_crsp.wait();
        auto payload = outbound_crsp.pop();

        const auto& chi = payload->require_as<chi::chi_fields>();
        tracer.instant(name, "sending_crsp", payload->txn_uid, payload->flit_id, chi.address,
                       {{"opcode", std::string(name_of(chi.rsp.opcode))},
                        {"tgt_id", std::to_string(chi.rsp.tgt_id)},
                        {"dbid", std::to_string(chi.rsp.dbid)}});

        port.send(std::move(payload));
    }
}

auto
HomeAgent::create_req(const Payload& source, chi::ReqOpcode opcode, uint32_t tgt_id) -> payload_ptr
{
    const auto& source_chi = source.require_as<chi::chi_fields>();
    auto        payload    = std::make_shared<Payload>(source.txn_uid);
    payload->protocol      = chi::chi_fields{
             .channel = chi::ChiChannel::REQ,
             .address = source_chi.address,
             .txn_id  = source_chi.txn_id,
             .req =
                 {
                     .opcode = opcode,
                     .src_id = id(),
                     .tgt_id = tgt_id,
                     .size   = source_chi.req.size,
            },
    };
    return payload;
}

auto
HomeAgent::create_dat(const Payload& source, chi::DatOpcode opcode, uint32_t tgt_id) -> payload_ptr
{
    const auto& source_chi = source.require_as<chi::chi_fields>();
    auto        payload    = std::make_shared<Payload>(source.txn_uid);
    payload->protocol      = chi::chi_fields{
             .channel = chi::ChiChannel::RDAT,
             .address = source_chi.address,
             .txn_id  = source_chi.txn_id,
             .dat =
                 {
                     .opcode    = opcode,
                     .src_id    = id(),
                     .tgt_id    = tgt_id,
                     .home_n_id = id(),
                     .resp      = chi::Resp::UC_or_UD,
            },
    };
    return payload;
}

auto
HomeAgent::create_rsp(const Payload& source, chi::RspOpcode opcode, uint32_t tgt_id,
                      uint16_t dbid) -> payload_ptr
{
    const auto& source_chi = source.require_as<chi::chi_fields>();
    auto        payload    = std::make_shared<Payload>(source.txn_uid);
    payload->protocol      = chi::chi_fields{
             .channel = chi::ChiChannel::CRSP,
             .address = source_chi.address,
             .txn_id  = source_chi.txn_id,
             .rsp =
                 {
                     .opcode = opcode,
                     .src_id = id(),
                     .tgt_id = tgt_id,
                     .dbid   = dbid,
            },
    };
    return payload;
}

auto
HomeAgent::create_snp(const Payload& source, chi::SnpOpcode opcode, uint32_t tgt_id) -> payload_ptr
{
    const auto& source_chi = source.require_as<chi::chi_fields>();
    auto        payload    = std::make_shared<Payload>(source.txn_uid);
    payload->protocol      = chi::chi_fields{
             .channel = chi::ChiChannel::SNP,
             .address = source_chi.address,
             .txn_id  = source_chi.txn_id,
             .snp =
                 {
                     .opcode = opcode,
                     .src_id = id(),
                     .tgt_id = tgt_id,
            },
    };
    return payload;
}

auto
HomeAgent::should_use_dmt(const Payload& req) -> bool
{
    return false; // TODO: real DMT decision logic
}
} // namespace csim