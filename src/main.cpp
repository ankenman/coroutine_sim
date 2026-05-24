#include <filesystem>
#include <iostream>

#include "csim/config/knob_system.h"
#include "csim/core/sim_types.h"
#include "csim/core/system.h"
#include "csim/tracing/tracer.h"
#include "models/topology_loader.h"

using namespace csim;

auto
main(int argc, char* argv[]) -> int
{
    sim_t  sim;
    System sys{sim};

    // Phase 1: register the global knob list.
    auto& global_knobs    = config::get_or_create("global");
    auto& output_dir_knob = global_knobs.add_knob<std::string>(
        "output_dir", "Directory for trace and config outputs", "output");

    // Phase 2: non-strict CLI parse to pick up topology file and global knobs.
    config::parse_command_line(argc, argv, /*strict=*/false);

    // Phase 3: load topology.
    TopologyLoader    loader(sim, sys);
    const std::string topo_file = config::get_topology_file();
    if (topo_file.empty()) {
        std::cerr << "Error: --topology <file> is required\n";
        return 1;
    }
    loader.load(topo_file);

    // Phase 4: help + strict parse for final overrides.
    config::check_for_help(argc, argv);
    config::parse_command_line(argc, argv, /*strict=*/true);

    // Phase 5: prepare output directory.
    const std::filesystem::path output_dir = output_dir_knob.get();
    std::filesystem::create_directories(output_dir);

    // Phase 6: dump resolved configuration.
    config::write_json_file((output_dir / "config_used.json").string());
    config::write_config_file((output_dir / "config_used.txt").string(), /*verbose=*/true);

    // Phase 7: elaborate.
    sys.elaborate_all();

    // Phase 8: tracing.
    auto& tracer = tracing::Tracer::instance();
    tracer.open((output_dir / "trace.jsonl").string(), (output_dir / "trace.txt").string(), sim);
    tracer.enable();

    // Phase 9: run.
    sys.start_all();
    sim.run_until(10'000_ns);
    tracer.close();

    std::cout << "Outputs written to: " << output_dir << "\n";

    return 0;
}