#include "csim/core/port.h"
#include <iostream>
#include "csim/core/sim_types.h"
#include "csim/core/system.h"
#include "models/topology_loader.h"
#include "csim/config/knob_system.h"

using namespace csim;

auto
main(int argc, char* argv[]) -> int
{
    sim_t  sim;
    System sys{sim};

    // Phase 1: get the topology file from CLI (and any non-strict knob parsing).
    config::parse_command_line(argc, argv, /*strict=*/false);

    // Phase 2: load topology, which constructs modules and applies their config.
    TopologyLoader    loader(sim, sys);
    const std::string topo_file = config::get_topology_file();
    if (topo_file.empty()) {
        std::cerr << "Error: --topology <file> is required\n";
        return 1;
    }
    loader.load(topo_file);

    // Phase 3: now all knobs are registered. Re-parse strictly, plus help.
    csim::config::check_for_help(argc, argv);
    csim::config::parse_command_line(argc, argv, /*strict=*/true);

    auto& tracer = tracing::Tracer::instance();
    tracer.open("trace.jsonl", "trace.txt", sim);
    tracer.enable();

    sys.elaborate_all();

    sys.start_all();
    sim.run_until(10'000_ns);
    tracer.close();

    return 0;
}
