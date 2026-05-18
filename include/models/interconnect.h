#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "csim/core/module.h"
#include "csim/core/payload.h"
#include "csim/core/port.h"
#include "csim/core/sim_types.h"

namespace csim {

class Interconnect : public Module {
public:
    Interconnect(sim_t& sim, System& sys, uint32_t id, std::string name);

    auto start() -> void override;

    // Wire up an agent. Agent's port becomes bound to one of our internal
    // ports; we record the mapping so we can route flits with tgt_id
    // matching the agent's id.
    auto attach(Module& m, Port& agent_port) -> void;

private:
    auto handle_incoming(payload_ptr payload) -> void;

    // One per attached agent. Owned by us; bound to the agent's port.
    std::unordered_map<uint32_t, std::unique_ptr<Port>> ports;
};

} // namespace csim