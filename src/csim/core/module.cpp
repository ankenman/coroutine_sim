#include "csim/core/module.h"

#include <utility>

#include "csim/core/system.h"

namespace csim {

Module::Module(sim_t& sim, System& sys, uint32_t id, std::string name)
    : sim(sim), name(std::move(name)), stored_id(id)
{
    sys.register_module(this);
}

} // namespace csim