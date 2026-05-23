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
System::elaborate_all() -> void
{
    for (auto* module : modules) {
        module->elaborate();
    }
}

auto
System::start_all() -> void
{
    for (auto* m : modules)
        m->start();
}

auto
Module::add_port(uint32_t neighbor_id) -> Port&
{
    auto [it, inserted] = port_map.emplace(neighbor_id, std::make_unique<Port>());
    assert(inserted && "duplicate port for neighbor id");
    return *it->second;
}

auto
Module::get_port(uint32_t neighbor_id) -> Port&
{
    return *port_map.at(neighbor_id);
}
auto
Module::single_port() const -> Port&
{
    assert(port_map.size() == 1 && "single_port() requires exactly one port");
    return *port_map.begin()->second;
}

} // namespace csim