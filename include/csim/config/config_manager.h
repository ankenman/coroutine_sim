#pragma once

#include "knob_list.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>

namespace csim {
class ConfigManager {
public:
    static auto instance() -> ConfigManager&
    {
        static ConfigManager instance;
        return instance;
    }

    ConfigManager(const ConfigManager&)            = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    auto get_or_create(const std::string& module_name) -> KnobList&;
    auto get_or_create(const char* module_name) -> KnobList&;

    auto register_knob(const std::string& full_name, KnobBase* knob) -> void;

    auto parse_command_line(int argc, char* argv[], bool strict = true) -> void;
    auto parse_config_file(const std::string& filename) -> void;
    auto parse_json_file(const std::string& filename) -> void;

    auto write_config_file(const std::string& filename, bool verbose = false) const -> void;
    auto write_json_file(const std::string& filename) const -> void;

    auto print_help() const -> void;

    auto reset_all() -> void;

    auto clear() -> void;

    auto remove_module(const std::string& module_name) -> void;

    auto get_knob(const std::string& full_name) const -> KnobBase*;

    auto check_for_help(int argc, char* argv[]) const -> void;

private:
    ConfigManager() = default;

    bool current_strict = true;

    auto set_knob_value(const std::string& key, const std::string& value) -> void;

    mutable std::mutex                                         mutex;
    std::unordered_map<std::string, std::unique_ptr<KnobList>> module_knob_lists;
    std::unordered_map<std::string, KnobBase*>                 all_knobs;
    std::vector<std::string>                                   module_order;
};
} // namespace csim

#include "knob_list.inl"