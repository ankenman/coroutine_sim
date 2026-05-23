#include "csim/core/port.h"
#include <iostream>
#include <deque>
#include <string>
#include "csim/core/sim_types.h"
#include "csim/core/system.h"

#include "models/initiator.h"
#include "models/target.h"
#include "models/interconnect.h"
#include "models/home_agent.h"

using namespace csim;

auto
main() -> int
{
    sim_t        sim;
    csim::System sys{sim};

    tracing::Tracer::instance().open("trace.jsonl", "trace.txt", sim);
    tracing::Tracer::instance().enable();

    constexpr time_ps clock_period_ps = 1000;

    Interconnect ic(sim, sys, /*id=*/2, "interconnect");
    Initiator    i(sim, sys, /*id=*/0, "initiator", /*target_id=*/3, clock_period_ps);
    HomeAgent    ha(sim, sys, /*id=*/3, "ha", /*downstream_target_id=*/1, clock_period_ps);
    Target       t(sim, sys, /*id=*/1, "target", clock_period_ps);

    sys.elaborate_all();

    ic.attach(i, i.port);
    ic.attach(ha, ha.port);
    ic.attach(t, t.port);

    sys.start_all();
    sim.run_until(10'000_ns);

    tracing::Tracer::instance().close();

    return 0;
}
