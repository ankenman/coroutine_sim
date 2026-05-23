#pragma once

#include <cstdint>
#include <string>

#include "csim/core/module.h"
#include "csim/core/payload.h"
#include "csim/core/sim_types.h"

namespace csim {

class Interconnect : public Module {
public:
    Interconnect(sim_t& sim, System& sys, uint32_t id, std::string name);

    auto elaborate() -> void override;
    auto start() -> void override;

private:
    auto handle_incoming(payload_ptr payload) -> void;
};

} // namespace csim