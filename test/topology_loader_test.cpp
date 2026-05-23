#include "models/topology_loader.h"
#include "csim/config/config.h"
#include "csim/core/system.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace csim::test {

class TopologyLoaderTest : public ::testing::Test {
public:
    sim_t                 sim;
    System                sys{sim};
    std::filesystem::path test_file = "test_topology.json";

    void SetUp() override { csim::config::clear(); }

    void TearDown() override
    {
        csim::config::clear();
        std::filesystem::remove(test_file);
    }

    auto write_json(const std::string& content) -> void
    {
        std::ofstream f(test_file);
        f << content;
    }
};

// ===== Smoke tests =====

TEST_F(TopologyLoaderTest, EmptyTopologyLoadsSuccessfully)
{
    write_json(R"({"modules": [], "connections": []})");

    TopologyLoader loader(sim, sys);
    EXPECT_NO_THROW(loader.load(test_file.string()));
}

TEST_F(TopologyLoaderTest, MalformedJsonThrows)
{
    write_json("not valid json");

    TopologyLoader loader(sim, sys);
    EXPECT_THROW(loader.load(test_file.string()), std::runtime_error);
}

TEST_F(TopologyLoaderTest, MissingFileThrows)
{
    TopologyLoader loader(sim, sys);
    EXPECT_THROW(loader.load("does_not_exist.json"), std::runtime_error);
}

// ===== Single-module construction =====

TEST_F(TopologyLoaderTest, ConstructsSingleInitiator)
{
    write_json(R"({
      "modules": [
        {"name": "init0", "type": "initiator", "id": 0, "config": {"home_id": 1}}
      ],
      "connections": []
    })");

    TopologyLoader loader(sim, sys);
    loader.load(test_file.string());

    EXPECT_NE(loader.get_module("init0"), nullptr);
}

TEST_F(TopologyLoaderTest, UnknownTypeThrows)
{
    write_json(R"({
      "modules": [
        {"name": "x", "type": "unknown_type", "id": 0}
      ],
      "connections": []
    })");

    TopologyLoader loader(sim, sys);
    EXPECT_THROW(loader.load(test_file.string()), std::runtime_error);
}

TEST_F(TopologyLoaderTest, DuplicateNameThrows)
{
    write_json(R"({
      "modules": [
        {"name": "init0", "type": "initiator", "id": 0, "home_id": 1},
        {"name": "init0", "type": "initiator", "id": 1, "home_id": 1}
      ],
      "connections": []
    })");

    TopologyLoader loader(sim, sys);
    EXPECT_THROW(loader.load(test_file.string()), std::runtime_error);
}

// ===== Knob value application =====

TEST_F(TopologyLoaderTest, AppliesPerModuleConfigToKnobs)
{
    write_json(R"({
      "modules": [
        {
          "name": "ha0",
          "type": "home_agent",
          "id": 1,
          "downstream_target_id": 2,
          "config": {
            "tq_capacity": 16,
            "cache_hit_latency_cycles": 7
          }
        }
      ],
      "connections": []
    })");

    TopologyLoader loader(sim, sys);
    loader.load(test_file.string());

    auto* tq_knob = csim::config::get_knob("ha0.tq_capacity");
    ASSERT_NE(tq_knob, nullptr);
    EXPECT_EQ(tq_knob->to_string(), "16");

    auto* lat_knob = csim::config::get_knob("ha0.cache_hit_latency_cycles");
    ASSERT_NE(lat_knob, nullptr);
    EXPECT_EQ(lat_knob->to_string(), "7");
}

// ===== Connections =====

TEST_F(TopologyLoaderTest, ConnectsTwoModules)
{
    write_json(R"({
      "modules": [
        {"name": "init0", "type": "initiator", "id": 0, "neighbors": ["ha0"],
         "config": {"home_id": 1}},
        {"name": "ha0", "type": "home_agent", "id": 1, "neighbors": ["init0"],
         "config": {"downstream_target_id": 2}}
      ]
    })");

    TopologyLoader loader(sim, sys);
    EXPECT_NO_THROW(loader.load(test_file.string()));
}

TEST_F(TopologyLoaderTest, ConnectionWithUnknownNameThrows)
{
    write_json(R"({
      "modules": [
        {"name": "init0", "type": "initiator", "id": 0, "neighbors": ["ghost"],
         "config": {"home_id": 1}}
      ]
    })");

    TopologyLoader loader(sim, sys);
    EXPECT_THROW(loader.load(test_file.string()), std::runtime_error);
}

} // namespace csim::test