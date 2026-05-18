#include "csim/sim_types.h"

#include <gtest/gtest.h>
#include <fschuetz04/simcpp20.hpp>
namespace csim {
class SubCoroutineTest : public ::testing::Test {
public:
    sim_t   sim;
    bool    sub_started        = false;
    bool    sub_finished       = false;
    bool    parent_resumed     = false;
    time_ps sub_finish_time    = 0;
    time_ps parent_resume_time = 0;

    auto sub() -> proc_t
    {
        sub_started = true;
        co_await sim.timeout(10);
        sub_finished    = true;
        sub_finish_time = sim.now();
    }

    auto parent() -> proc_t
    {
        co_await sub();
        parent_resumed     = true;
        parent_resume_time = sim.now();
    }
};

TEST_F(SubCoroutineTest, ParentAwaitsSubCompletion)
{
    parent();
    sim.run();

    EXPECT_TRUE(sub_started);
    EXPECT_TRUE(sub_finished);
    EXPECT_TRUE(parent_resumed);

    // Parent should resume after sub completes.
    EXPECT_EQ(sub_finish_time, 10);
    EXPECT_EQ(parent_resume_time, 10);
}

TEST_F(SubCoroutineTest, ParentResumesOnlyAfterSubCompletes)
{
    parent();

    // Run partway — sub should be mid-await, parent should not yet have resumed.
    sim.run_until(5);
    EXPECT_TRUE(sub_started);
    EXPECT_FALSE(sub_finished);
    EXPECT_FALSE(parent_resumed);

    // Run rest.
    sim.run();
    EXPECT_TRUE(sub_finished);
    EXPECT_TRUE(parent_resumed);
}
} // namespace csim