#include "csim/config/knob_system.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace csim::test {

class ParseCommandLineTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Clear ConfigManager state before each test.
        ConfigManager::instance().clear();
    }

    void TearDown() override
    {
        // Clear again so we don't leak state to other tests.
        ConfigManager::instance().clear();
    }

    // Build a fake argv from a vector of strings.
    // Stores strings as members so the c_str() pointers remain valid for the test duration.
    auto build_argv(const std::vector<std::string>& args) -> std::vector<char*>
    {
        argv_storage = args;
        std::vector<char*> argv;
        argv.reserve(argv_storage.size());
        for (auto& s : argv_storage) {
            argv.push_back(s.data());
        }
        return argv;
    }

    // Helper: register a knob in module "test_mod".
    template <typename T> auto add_knob(const std::string& name, const T& default_value) -> Knob<T>&
    {
        auto& kl = ConfigManager::instance().get_or_create("test_mod");
        return kl.add_knob(name, "test knob", default_value);
    }

private:
    std::vector<std::string> argv_storage;
};

TEST_F(ParseCommandLineTest, NoArgsLeavesDefaults)
{
    auto& clock_period = add_knob<int>("clock_period", 100);

    auto argv = build_argv({"program"});
    ConfigManager::instance().parse_command_line(argv.size(), argv.data());

    EXPECT_EQ(clock_period.get(), 100);
}

TEST_F(ParseCommandLineTest, SingleIntKnobOverride)
{
    auto& clock_period = add_knob<int>("clock_period", 100);

    auto argv = build_argv({"program", "--test_mod.clock_period", "250"});
    ConfigManager::instance().parse_command_line(argv.size(), argv.data());

    EXPECT_EQ(clock_period.get(), 250);
}

TEST_F(ParseCommandLineTest, MultipleKnobsOverride)
{
    auto& clock_period = add_knob<int>("clock_period", 100);
    auto& buffer_size  = add_knob<int>("buffer_size", 1024);

    auto argv =
        build_argv({"program", "--test_mod.clock_period", "250", "--test_mod.buffer_size", "2048"});
    ConfigManager::instance().parse_command_line(argv.size(), argv.data());

    EXPECT_EQ(clock_period.get(), 250);
    EXPECT_EQ(buffer_size.get(), 2048);
}

TEST_F(ParseCommandLineTest, BoolKnobWithoutValueSetsTrue)
{
    auto& debug = add_knob<bool>("debug", false);

    auto argv = build_argv({"program", "--test_mod.debug"});
    ConfigManager::instance().parse_command_line(argv.size(), argv.data());

    EXPECT_TRUE(debug.get());
}

TEST_F(ParseCommandLineTest, BoolKnobWithFalseValue)
{
    auto& debug = add_knob<bool>("debug", true);

    auto argv = build_argv({"program", "--test_mod.debug", "false"});
    ConfigManager::instance().parse_command_line(argv.size(), argv.data());

    EXPECT_FALSE(debug.get());
}

TEST_F(ParseCommandLineTest, UnknownKnobLogsWarning)
{
    // Capture stderr.
    testing::internal::CaptureStderr();

    auto argv = build_argv({"program", "--unknown.knob", "5"});
    ConfigManager::instance().parse_command_line(argv.size(), argv.data());

    std::string captured = testing::internal::GetCapturedStderr();

    // Should contain some warning text.
    EXPECT_TRUE(captured.find("unknown.knob") != std::string::npos);
    EXPECT_FALSE(captured.empty());
}

TEST_F(ParseCommandLineTest, NonStrictIgnoresUnknownKnob)
{
    auto argv = build_argv({"program", "--unknown.knob", "5"});

    EXPECT_NO_THROW(
        ConfigManager::instance().parse_command_line(argv.size(), argv.data(), /*strict=*/false));
}

TEST_F(ParseCommandLineTest, NonStrictPreservesValidKnobs)
{
    auto& clock_period = add_knob<int>("clock_period", 100);

    auto argv = build_argv({"program", "--test_mod.clock_period", "250", "--unknown.knob", "999"});

    ConfigManager::instance().parse_command_line(argv.size(), argv.data(), /*strict=*/false);

    EXPECT_EQ(clock_period.get(), 250);
}

// ===== Multi-pass parsing =====

TEST_F(ParseCommandLineTest, TwoPassFirstNonStrictThenStrict)
{
    auto& early_knob = add_knob<int>("early", 0);

    auto argv = build_argv({"program", "--test_mod.early", "1", "--test_mod.late", "2"});

    // First pass: non-strict; late knob ignored.
    ConfigManager::instance().parse_command_line(argv.size(), argv.data(), /*strict=*/false);
    EXPECT_EQ(early_knob.get(), 1);

    // Register the late knob.
    auto& late_knob = add_knob<int>("late", 0);

    // Second pass: strict; both are valid now.
    ConfigManager::instance().parse_command_line(argv.size(), argv.data(), /*strict=*/true);
    EXPECT_EQ(early_knob.get(), 1);
    EXPECT_EQ(late_knob.get(), 2);
}

/*
String knobs — quoted strings, strings with spaces, escaped characters.
Double/float knobs — decimal parsing, edge cases.
--help interaction — does check_for_help work after parsing?
--config flag — load from a file, verify values. Requires creating temp files.
--json flag — same as above but JSON.
Precedence — config file value vs command-line value (command-line wins).
Boolean variants — "true"/"True"/"1"/"yes"/"on" all parse to true; equivalents for false.
*/

} // namespace csim::test