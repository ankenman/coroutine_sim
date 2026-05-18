#include "csim/utilities/scheduled_event.h"
#include "csim/core/sim_types.h"

#include <gtest/gtest.h>
#include <fschuetz04/simcpp20.hpp>

using namespace csim;

class ScheduledEventTest : public ::testing::Test {
public:
    sim_t sim;

protected:
    ScheduledEvent       sched_event{sim};
    std::vector<time_ps> fired_at;
};

auto
wait_and_record(simcpp20::simulation<uint64_t>& sim, ScheduledEvent& sched_event,
                std::vector<time_ps>& fired_at) -> simcpp20::process<uint64_t>
{
    co_await sched_event.wait();
    fired_at.push_back(sim.now());
}

TEST_F(ScheduledEventTest, NoFireWithoutNotify)
{
    wait_and_record(sim, sched_event, fired_at);
    sim.run_until(10);

    EXPECT_TRUE(fired_at.empty());
}

TEST_F(ScheduledEventTest, NotifyOnce)
{
    auto proc = wait_and_record(sim, sched_event, fired_at);

    sched_event.notify(5);
    sim.run_until(10);

    EXPECT_EQ(fired_at.size(), 1);
    EXPECT_EQ(fired_at[0], 5);
}

TEST_F(ScheduledEventTest, EarlierNotifyWins)
{
    auto proc = wait_and_record(sim, sched_event, fired_at);

    sched_event.notify(5);
    sched_event.notify(4);
    sim.run_until(10);

    EXPECT_EQ(fired_at.size(), 1);
    EXPECT_EQ(fired_at[0], 4);
}
TEST_F(ScheduledEventTest, LaterNotifyIgnoredIfEarlierPending)
{
    wait_and_record(sim, sched_event, fired_at);
    sched_event.notify(2);
    sched_event.notify(5); // should be ignored
    sim.run_until(10);

    EXPECT_EQ(fired_at.size(), 1);
    EXPECT_EQ(fired_at[0], 2);
}

TEST_F(ScheduledEventTest, CancelPreventsFiree)
{
    wait_and_record(sim, sched_event, fired_at);
    sched_event.notify(3);
    sched_event.cancel();
    sim.run_until(10);

    EXPECT_TRUE(fired_at.empty());
}

TEST_F(ScheduledEventTest, NotifyAfterCancelFires)
{
    wait_and_record(sim, sched_event, fired_at);
    sched_event.notify(3);
    sched_event.cancel();
    sched_event.notify(7);
    sim.run_until(10);

    EXPECT_EQ(fired_at.size(), 1);
    EXPECT_EQ(fired_at[0], 7);
}

TEST_F(ScheduledEventTest, MultipleWaitersAllUnblocked)
{
    wait_and_record(sim, sched_event, fired_at);
    wait_and_record(sim, sched_event, fired_at);
    wait_and_record(sim, sched_event, fired_at);
    sched_event.notify(4);
    sim.run_until(10);

    EXPECT_EQ(fired_at.size(), 3);
    for (time_ps t : fired_at)
        EXPECT_EQ(t, 4);
}

TEST_F(ScheduledEventTest, CanBeReusedAfterFiring)
{
    // First wait
    wait_and_record(sim, sched_event, fired_at);
    sched_event.notify(2);
    sim.run_until(3);

    EXPECT_EQ(fired_at.size(), 1);
    EXPECT_EQ(fired_at[0], 2);

    // Second wait on the same ScheduledEvent
    wait_and_record(sim, sched_event, fired_at);
    sched_event.notify(3); // relative to now (t=3), fires at t=6
    sim.run_until(10);

    EXPECT_EQ(fired_at.size(), 2);
    EXPECT_EQ(fired_at[1], 6);
}
