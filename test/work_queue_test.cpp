#include "csim/utilities/work_queue.h"
#include "csim/core/payload.h"
#include "csim/core/sim_types.h"

#include <gtest/gtest.h>
#include <fschuetz04/simcpp20.hpp>

#include <memory>
#include <vector>

namespace csim::test {

namespace {
constexpr time_ps min_offset = 1_ns;
}

class WorkQueueTest : public ::testing::Test {
public:
    sim_t     sim;
    WorkQueue queue{sim, min_offset};

    struct Observation {
        time_ps     when;
        payload_ptr what;
    };
    std::vector<Observation> observations;

    // Consumer coroutine — loops, awaits, pops, records.
    auto consumer() -> proc_t
    {
        while (true) {
            co_await queue.wait();
            observations.push_back({.when = sim.now(), .what = queue.pop()});
        }
    }
};

TEST_F(WorkQueueTest, EmptyAtStart)
{
    EXPECT_TRUE(queue.empty());
}

TEST_F(WorkQueueTest, AddAndRemoveElement)
{
    auto payload = std::make_shared<Payload>();
    queue.push(2_ns, payload);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1u);

    auto popped = queue.pop();
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(popped, payload);
}

TEST_F(WorkQueueTest, FrontElementIsSameAsInput)
{
    auto payload = std::make_shared<Payload>();
    queue.push(2_ns, payload);
    EXPECT_EQ(queue.front(), payload);
}

TEST_F(WorkQueueTest, ElementsAreInOrderByTime)
{
    auto p0 = std::make_shared<Payload>();
    auto p1 = std::make_shared<Payload>();
    auto p2 = std::make_shared<Payload>();

    queue.push(4_ns, p0);
    queue.push(2_ns, p1);
    queue.push(6_ns, p2);

    EXPECT_EQ(queue.pop(), p1); // earliest first
    EXPECT_EQ(queue.pop(), p0);
    EXPECT_EQ(queue.pop(), p2);
    EXPECT_TRUE(queue.empty());
}

TEST_F(WorkQueueTest, ConsumerWakesAtCorrectTime)
{
    auto payload = std::make_shared<Payload>();
    consumer();

    queue.push(5_ns, payload);
    sim.run_until(10_ns);

    EXPECT_EQ(observations.size(), 1u);
    EXPECT_EQ(observations[0].when, 5_ns);
    EXPECT_EQ(observations[0].what, payload);
}

TEST_F(WorkQueueTest, ConsumerSeesEventsInTimeOrder)
{
    auto p0 = std::make_shared<Payload>();
    auto p1 = std::make_shared<Payload>();
    consumer();

    queue.push(4_ns, p0);
    queue.push(2_ns, p1);
    sim.run_until(10_ns);

    EXPECT_EQ(observations.size(), 2u);
    EXPECT_EQ(observations[0].when, 2_ns);
    EXPECT_EQ(observations[0].what, p1);
    EXPECT_EQ(observations[1].when, 4_ns);
    EXPECT_EQ(observations[1].what, p0);
}

} // namespace csim::test
