#pragma once

#include <cstdint>
#include <string>

#include "sim_types.h"
#include "csim/tracing/tracer.h"
#include "csim/core/port.h"

namespace csim {

class System;

class Module {
public:
    Module(sim_t& sim, System& sys, uint32_t id, std::string name);
    virtual ~Module() = default;

    // Called after all modules are constructed and config is finalized.
    // Modules override this to build internal state that depends on knob values.
    // Default: no-op.
    virtual auto elaborate() -> void {}

    virtual auto start() -> void {}

    [[nodiscard]] auto id() const -> uint32_t { return stored_id; }

    auto add_port(uint32_t neighbor_id) -> Port&;
    auto get_port(uint32_t neighbor_id) -> Port&;

    sim_t&            sim;
    const std::string name;

    Module(const Module&)            = delete;
    Module& operator=(const Module&) = delete;

protected:
    tracing::Tracer& tracer = tracing::Tracer::instance();

    std::unordered_map<uint32_t, std::unique_ptr<Port>> port_map;

    auto single_port() const -> Port&;

private:
    uint32_t stored_id;
};

} // namespace csim