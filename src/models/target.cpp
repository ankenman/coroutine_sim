#include "models/target.h"

#include <memory>
#include <utility>

namespace csim {

Target::Target(sim_t& sim, System& sys, uint32_t id, std::string name, time_ps clock_period_ps)
    : Module(sim, sys, id, std::move(name)),
      clock_period_ps(clock_period_ps),
      outbound_data(sim, clock_period_ps)
{
    port.on_receive(this, &Target::handle_request);
}

auto
Target::start() -> void
{
    service_data_queue();
}

auto
Target::handle_request(payload_ptr payload) -> void
{
    const auto& req_chi = payload->require_as<chi::chi_fields>();

    tracer.instant(name, "received_req", payload->txn_uid, payload->flit_id, req_chi.address,
                   {{"opcode", std::string(name_of(req_chi.req.opcode))},
                    {"txn_id", std::to_string(req_chi.txn_id)}});

    auto response = create_data_payload(req_chi, payload->txn_uid);

    const time_ps offset_ps = static_cast<time_ps>(data_latency_cycles) * clock_period_ps;

    outbound_data.push(offset_ps, std::move(response));
}

auto
Target::service_data_queue() -> proc_t
{
    while (true) {
        co_await outbound_data.wait();
        auto payload = outbound_data.pop();

        const auto& chi_pl = payload->require_as<chi::chi_fields>();
        tracer.instant(name, "sending_rdat", payload->txn_uid, payload->flit_id, chi_pl.address,
                       {{"opcode", std::string(name_of(chi_pl.dat.opcode))},
                        {"txn_id", std::to_string(chi_pl.txn_id)}});

        port.send(std::move(payload));
    }
}

auto
Target::create_data_payload(const chi::chi_fields& req_chi, uint64_t txn_uid) -> payload_ptr
{
    auto data_payload = std::make_shared<Payload>(txn_uid);

    data_payload->protocol = chi::chi_fields{
        .channel = chi::ChiChannel::RDAT,
        .address = req_chi.address,
        .txn_id  = req_chi.txn_id,
        .dat =
            {
                .opcode    = chi::DatOpcode::CompData,
                .src_id    = req_chi.req.tgt_id,
                .tgt_id    = req_chi.req.src_id,
                .home_n_id = req_chi.req.tgt_id,
                .resp      = chi::Resp::UC_or_UD,
            },
    };

    return data_payload;
}

} // namespace csim