#include "models/interconnect.h"

#include <cassert>
#include <ranges>

#include "protocols/chi.h"

namespace csim {

Interconnect::Interconnect(sim_t& sim, System& sys, uint32_t id, std::string name)
    : Module(sim, sys, id, std::move(name))
{
}

auto
Interconnect::elaborate() -> void
{
    for (auto& port_ptr : port_map | std::views::values) {
        port_ptr->on_receive(this, &Interconnect::handle_incoming);
    }
}

auto
Interconnect::start() -> void
{
    // No coroutines — interconnect is purely synchronous.
}

auto
Interconnect::handle_incoming(payload_ptr payload) -> void
{
    const auto& chi = payload->require_as<chi::chi_fields>();

    uint32_t tgt_id = 0;
    switch (chi.channel) {
    case chi::ChiChannel::REQ:
        tgt_id = chi.req.tgt_id;
        break;
    case chi::ChiChannel::WDAT:
    case chi::ChiChannel::RDAT:
        tgt_id = chi.dat.tgt_id;
        break;
    case chi::ChiChannel::CRSP:
    case chi::ChiChannel::SRSP:
        tgt_id = chi.rsp.tgt_id;
        break;
    case chi::ChiChannel::SNP:
        tgt_id = chi.snp.tgt_id;
        break;
    default:
        assert(false && "unhandled channel in interconnect routing");
    }

    get_port(tgt_id).send(std::move(payload));
}

} // namespace csim