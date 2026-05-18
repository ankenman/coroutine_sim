#include "csim/tracing/tracer.h"
#include "csim/core/sim_types.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace csim::tracing::test {

class TracerTest : public ::testing::Test {
protected:
    sim_t                 sim;
    std::filesystem::path jsonl_path = "test_trace.jsonl";
    std::filesystem::path text_path  = "test_trace.txt";

    void TearDown() override
    {
        Tracer::instance().close();
        std::filesystem::remove(jsonl_path);
        std::filesystem::remove(text_path);
    }
};

TEST_F(TracerTest, OpenAndCloseCreatesFiles)
{
    Tracer::instance().open(jsonl_path.string(), text_path.string(), sim);
    EXPECT_TRUE(std::filesystem::exists(jsonl_path));
    EXPECT_TRUE(std::filesystem::exists(text_path));
    Tracer::instance().close();
}

TEST_F(TracerTest, EnableDisableTogglesState)
{
    Tracer::instance().open(jsonl_path.string(), text_path.string(), sim);
    EXPECT_FALSE(Tracer::instance().is_enabled());
    Tracer::instance().enable();
    EXPECT_TRUE(Tracer::instance().is_enabled());
    Tracer::instance().disable();
    EXPECT_FALSE(Tracer::instance().is_enabled());
}

TEST_F(TracerTest, EmptyPathSkipsThatFile)
{
    Tracer::instance().open(jsonl_path.string(), "", sim);
    EXPECT_TRUE(std::filesystem::exists(jsonl_path));
    EXPECT_FALSE(std::filesystem::exists(text_path));
    Tracer::instance().close();
}
TEST_F(TracerTest, DisabledByDefaultDoesNothing)
{
    Tracer::instance().open(jsonl_path.string(), text_path.string(), sim);
    Tracer::instance().instant("ha", "test_event", 42);
    Tracer::instance().close();

    std::ifstream f(text_path);
    std::string   content{std::istreambuf_iterator<char>(f), {}};
    EXPECT_TRUE(content.empty());
}

TEST_F(TracerTest, InstantWritesText)
{
    Tracer::instance().open(jsonl_path.string(), text_path.string(), sim);
    Tracer::instance().enable();
    Tracer::instance().instant("ha", "cache_hit", 42);
    Tracer::instance().close();

    std::ifstream f(text_path);
    std::string   content{std::istreambuf_iterator<char>(f), {}};
    EXPECT_EQ(content, "[t=0.00 ns] ha cache_hit txn_uid=42\n");
}

TEST_F(TracerTest, InstantWritesJsonl)
{
    Tracer::instance().open(jsonl_path.string(), text_path.string(), sim);
    Tracer::instance().enable();
    Tracer::instance().instant("ha", "cache_hit", 42);
    Tracer::instance().close();

    std::ifstream f(jsonl_path);
    std::string   content{std::istreambuf_iterator<char>(f), {}};
    EXPECT_EQ(content, R"({"ts":0.000,"pid":"42","tid":"0","cat":"ha","name":"cache_hit","ph":"i"})"
                       "\n");
}

TEST_F(TracerTest, InstantWithFlitIdAndAddress)
{
    Tracer::instance().open(jsonl_path.string(), text_path.string(), sim);
    Tracer::instance().enable();
    Tracer::instance().instant("ha", "send_data", 42, 100, 0x1000);
    Tracer::instance().close();

    std::ifstream f(text_path);
    std::string   content{std::istreambuf_iterator<char>(f), {}};
    EXPECT_EQ(content, "[t=0.00 ns] ha send_data txn_uid=42 flit_id=100 addr=0x1000\n");
}

TEST_F(TracerTest, InstantWithUserArgs)
{
    Tracer::instance().open(jsonl_path.string(), text_path.string(), sim);
    Tracer::instance().enable();
    Tracer::instance().instant("ha", "send_data", 42, std::nullopt, std::nullopt,
                               {{"opcode", "CompData"}, {"dbid", "7"}});
    Tracer::instance().close();

    std::ifstream f(text_path);
    std::string   content{std::istreambuf_iterator<char>(f), {}};
    EXPECT_EQ(content, "[t=0.00 ns] ha send_data txn_uid=42 dbid=7 opcode=CompData\n");
}
} // namespace csim::tracing::test
