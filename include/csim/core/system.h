#pragma once

#include <vector>

#include "sim_types.h"

namespace csim {

class Module;

class System {
public:
    explicit System(sim_t& sim);

    auto register_module(Module* m) -> void;
    auto elaborate_all() -> void;
    auto start_all() -> void;

    sim_t& sim;

private:
    std::vector<Module*> modules;
};

} // namespace csim