#include "models/interconnect.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <utility>

#include "protocols/chi.h"

namespace csim {

Interconnect::Interconnect(sim_t& sim, System& sys, uint32_t id, std::string name)
    : Module(sim, sys, id, std::move(name))
{
}

auto
Interconnect::start() -> void
{
    // No coroutines yet — interconnect is purely synchronous.
}

auto
Interconnect::attach(Module& m, Port& agent_port) -> void
{
    assert(ports.find(m.id()) == ports.end() && "duplicate node_id on interconnect");

    auto p = std::make_unique<Port>();
    p->on_receive(this, &Interconnect::handle_incoming);
    bind(*p, agent_port);
    ports[m.id()] = std::move(p);
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

    auto it = ports.find(tgt_id);
    assert(it != ports.end() && "tgt_id not attached to interconnect");

    it->second->send(std::move(payload));
}

} // namespace csim