#include "models/topology_loader.h"
#include "models/home_agent.h"
#include "models/initiator.h"
#include "models/target.h"
#include "models/interconnect.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace csim {

TopologyLoader::TopologyLoader(sim_t& sim, System& sys) : sim(sim), sys(sys) {}

auto
TopologyLoader::load(const std::string& filename) -> void
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open topology file: " + filename);
    }

    nlohmann::json topology;
    try {
        file >> topology;
    }
    catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Failed to parse topology JSON: " + std::string(e.what()));
    }

    // Phase 1: construct modules.
    if (topology.contains("modules")) {
        for (const auto& spec : topology["modules"]) {
            construct_module(spec);
        }
    }

    // Phase 2: wire neighbors.
    if (topology.contains("modules")) {
        std::set<std::pair<std::string, std::string>> seen;
        for (const auto& spec : topology["modules"]) {
            wire_neighbors(spec, seen);
        }
    }
}

auto
TopologyLoader::get_module(const std::string& name) const -> Module*
{
    auto it = modules.find(name);
    return (it != modules.end()) ? it->second.get() : nullptr;
}

auto
TopologyLoader::construct_module(const nlohmann::json& spec) -> void
{
    const std::string name = spec["name"];
    const std::string type = spec["type"];

    if (modules.find(name) != modules.end()) {
        throw std::runtime_error("Duplicate module name: " + name);
    }

    std::unique_ptr<Module> module;
    if (type == "initiator") {
        module = construct_initiator(spec);
    }
    else if (type == "home_agent") {
        module = construct_home_agent(spec);
    }
    else if (type == "target") {
        module = construct_target(spec);
    }
    else if (type == "interconnect") {
        module = construct_interconnect(spec);
    }
    else {
        throw std::runtime_error("Unknown module type: " + type);
    }

    modules[name] = std::move(module);

    if (spec.contains("config")) {
        apply_module_config(name, spec["config"]);
    }
}

auto
TopologyLoader::construct_initiator(const nlohmann::json& spec) -> std::unique_ptr<Module>
{
    const std::string name = spec["name"];
    const uint32_t    id   = spec["id"];

    return std::make_unique<Initiator>(sim, sys, id, name);
}

auto
TopologyLoader::construct_home_agent(const nlohmann::json& spec) -> std::unique_ptr<Module>
{
    const std::string name = spec["name"];
    const uint32_t    id   = spec["id"];

    return std::make_unique<HomeAgent>(sim, sys, id, name);
}
auto
TopologyLoader::construct_target(const nlohmann::json& spec) -> std::unique_ptr<Module>
{
    const std::string name = spec["name"];
    const uint32_t    id   = spec["id"];

    return std::make_unique<Target>(sim, sys, id, name);
}
auto
TopologyLoader::construct_interconnect(const nlohmann::json& spec) -> std::unique_ptr<Module>
{
    const std::string name = spec["name"];
    const uint32_t    id   = spec["id"];

    return std::make_unique<Interconnect>(sim, sys, id, name);
}
auto
TopologyLoader::apply_module_config(const std::string& name, const nlohmann::json& config) -> void
{
    for (const auto& [key, value] : config.items()) {
        const std::string full_key = name + "." + key;
        auto*             knob     = csim::config::get_knob(full_key);
        if (knob == nullptr) {
            throw std::runtime_error("Unknown knob '" + full_key + "' in config block");
        }

        std::string val_str;
        if (value.is_string()) {
            val_str = value.get<std::string>();
        }
        else {
            val_str = value.dump();
        }

        knob->set_from_string(val_str);
    }
}

auto
TopologyLoader::wire_neighbors(const nlohmann::json&                          spec,
                               std::set<std::pair<std::string, std::string>>& seen) -> void
{
    if (!spec.contains("neighbors")) {
        return;
    }

    const std::string self_name = spec["name"];

    for (const auto& neighbor_item : spec["neighbors"]) {
        const std::string neighbor_name = neighbor_item;

        auto neighbor_it = modules.find(neighbor_name);
        if (neighbor_it == modules.end()) {
            throw std::runtime_error("Unknown neighbor '" + neighbor_name + "' for module '" +
                                     self_name + "'");
        }

        // Canonical edge key — dedupes when the neighbors list is symmetric.
        auto key =
            std::make_pair(std::min(self_name, neighbor_name), std::max(self_name, neighbor_name));
        if (seen.contains(key)) {
            continue;
        }
        seen.insert(key);

        Module* self     = modules.at(self_name).get();
        Module* neighbor = neighbor_it->second.get();

        Port& self_port     = self->add_port(neighbor->id());
        Port& neighbor_port = neighbor->add_port(self->id());

        bind(self_port, neighbor_port);
    }
}

} // namespace csim
