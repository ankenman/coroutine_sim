#pragma once

#include "csim/core/module.h"
#include "csim/core/sim_types.h"
#include "csim/core/system.h"

#include <nlohmann/json.hpp>

#include <set>
#include <string>
#include <unordered_map>

namespace csim {

class TopologyLoader {
public:
    TopologyLoader(sim_t& sim, System& sys);

    auto load(const std::string& filename) -> void;
    auto get_module(const std::string& name) const -> Module*;

private:
    sim_t&  sim;
    System& sys;

    std::unordered_map<std::string, std::unique_ptr<Module>> modules;

    auto construct_module(const nlohmann::json& spec) -> void;
    auto construct_initiator(const nlohmann::json& spec) -> std::unique_ptr<Module>;
    auto construct_home_agent(const nlohmann::json& spec) -> std::unique_ptr<Module>;
    auto construct_target(const nlohmann::json& spec) -> std::unique_ptr<Module>;
    auto construct_interconnect(const nlohmann::json& spec) -> std::unique_ptr<Module>;
    auto apply_module_config(const std::string& name, const nlohmann::json& config) -> void;
    auto wire_neighbors(const nlohmann::json&                          spec,
                        std::set<std::pair<std::string, std::string>>& seen) -> void;
};

} // namespace csim
