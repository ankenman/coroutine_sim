#include "csim/core/system.h"
#include "csim/core/module.h"

namespace csim {

System::System(sim_t& sim) : sim(sim) {}

auto
System::register_module(Module* m) -> void
{
    modules.push_back(m);
}

auto
System::start_all() -> void
{
    for (auto* m : modules)
        m->start();
}

} // namespace csim