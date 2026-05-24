# coroutine_sim

[![googletest](https://github.com/ankenman/coroutine_sim/actions/workflows/googletest.yml/badge.svg)](https://github.com/ankenman/coroutine_sim/actions/workflows/googletest.yml)
[![clang-format](https://github.com/ankenman/coroutine_sim/actions/workflows/clang-format.yml/badge.svg)](https://github.com/ankenman/coroutine_sim/actions/workflows/clang-format.yml)

A CHI-protocol cache-coherent SoC simulator built with simcpp20 coroutines.

The simulator is loosely based on SystemC TLM-2.0 conventions — modules communicate via ports with on-receive callbacks,
payloads carry protocol-specific extensions, and a discrete-event scheduler advances simulation time. The substantive
departure is using C++20 coroutines rather than SystemC processes, which allows each transaction to be modeled as a
top-to-bottom protocol flow rather than a distributed state machine.

## What it models

- An **initiator** that issues ReadShared and WriteUniqueFull transactions on a cycle-driven schedule.
- A **home agent** with a metadata-only cache, handling reads (forward to target on miss, direct response on hit) and
  writes (absorb locally, install line on data arrival). Each transaction runs as its own coroutine; address
  serialization is enforced by a TransactionQueue.
- A **target** that services read requests with a configurable latency.
- An **interconnect** that routes flits between modules by target id.

CHI transactions terminate correctly: reads via CompAck, writes via combined CompDBIDResp + NCBWrData.

## Approach

The simulator uses a **coroutine-per-transaction** model in the home agent. Each incoming REQ spawns a coroutine that
runs the full protocol flow top-to-bottom — cache lookup, optional refill, response, completion wait — with `co_await`
at each protocol pause. Address-serialization is enforced by a TransactionQueue: transactions acquire a slot at
coroutine entry, and same-address transactions queue behind the active head.

This makes the protocol logic read as the protocol it models, rather than being distributed across callbacks and
state-machine maps.

## Topology and configuration

The SoC layout is described by a JSON topology file. Each module declares its name, type, id, neighbors, and per-module
configuration values. The topology loader constructs modules, applies their config, and wires ports based on the
neighbor lists.

```bash
./coroutine_sim --topology example/topology.json
```

Per-module parameters (clock period, latencies, queue capacities, etc.) are exposed as knobs and can be overridden from
the command line or a separate config file:

```bash
./coroutine_sim --topology example/topology.json --ha0.cache_hit_latency_cycles 5
./coroutine_sim --topology example/topology.json --config my_overrides.txt
./coroutine_sim --topology example/topology.json --json my_overrides.json
./coroutine_sim --topology example/topology.json --help    # lists all registered knobs
```

The example topology under `example/topology.json` shows a minimal initiator → interconnect → home agent → target chain.

To experiment with config overrides:

```bash
# Use the example config as a starting point
./coroutine_sim --topology example/topology.json --json example/default_config.json
```

## Tracing

The simulator emits structured events to `trace.jsonl` (one JSON event per line, crash-safe) and a human-readable
`trace.txt`. The JSONL output is convertible to Chrome Trace format for visualization in Perfetto:

```bash
python3 scripts/jsonl_to_perfetto.py trace.jsonl trace.json
```

Open `trace.json` in https://ui.perfetto.dev. Each transaction appears as a track; per-flit events appear as rows within
the track.

## Project layout

```
include/
  csim/                  reusable simulation framework
    core/                module, system, port, payload, sim_types
    utilities/           work_queue, delay_channel, scheduled_event, cache
    tracing/             tracer
    config/              knob system, csim::config namespace
  models/                CHI simulation components (HA, initiator, target,
                         interconnect, topology loader)
  protocols/             CHI enum and field definitions
src/
  csim/                  framework implementations
  models/                model implementations
  main.cpp               top-level simulation setup
test/                    unit tests (googletest)
scripts/                 utilities (Perfetto trace conversion)
example/                 example topology JSON files
```

## Building

Requires a C++20 compiler and CMake 3.14+.

```bash
mkdir build && cd build
cmake ..
cmake --build .
./src/coroutine_sim --topology ../example/topology.json
```

Dependencies (simcpp20, magic_enum, nlohmann/json, googletest) are fetched automatically via CMake FetchContent.

## Code style

The project follows the rules in `.clang-format`. CI verifies formatting using **clang-format 18**; using a matching
version locally is recommended to avoid spurious diffs.

To check formatting locally:

```bash
find include src test -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec clang-format --dry-run --Werror {} +
```

To apply formatting in place:

```bash
find include src test -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec clang-format -i {} +
```

## Status

Functional for ReadShared and WriteUniqueFull with JSON-driven topology, per-module knob configuration, and
TransactionQueue-based address hazard handling.

Planned: multiple initiators, ReadUnique / MakeUnique opcodes, snoop modeling (with structured per-peer expectations),
DMT (Direct Memory Transfer) variants, eviction modeling, statistics infrastructure, and System Address Map for routing
by address across multiple HNs.
