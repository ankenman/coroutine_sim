#include "csim/config/config.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>

namespace csim::config {
namespace {
std::unordered_map<std::string, std::unique_ptr<KnobList>> module_knob_lists;
std::unordered_map<std::string, KnobBase*>                 all_knobs;
std::vector<std::string>                                   module_order;
bool                                                       current_strict = true;

auto
set_knob_value(const std::string& key, const std::string& value) -> void
{
    auto it = all_knobs.find(key);
    if (it == all_knobs.end()) {
        if (current_strict) {
            std::string error_msg = "Unknown knob: " + key;
            if (key.find('.') == std::string::npos) {
                error_msg += "\nDid you mean to specify a knob? Use --module." + key + " value";
            }
            throw std::runtime_error(error_msg);
        }
        return;
    }
    it->second->set_from_string(value);
}

} // namespace

auto
get_or_create(const std::string& module_name) -> KnobList&
{
    if (module_knob_lists.find(module_name) == module_knob_lists.end()) {
        module_knob_lists[module_name] = std::make_unique<KnobList>(module_name);
        module_order.push_back(module_name);
    }
    return *module_knob_lists[module_name];
}

auto
get_or_create(const char* module_name) -> KnobList&
{
    return get_or_create(std::string(module_name));
}

auto
register_knob(const std::string& full_name, KnobBase* knob) -> void
{
    all_knobs[full_name] = knob;
}

auto
parse_command_line(int argc, char* argv[], bool strict) -> void
{
    // Priority: JSON (lowest) -> config file -> command line args (highest)

    current_strict = strict;

    // Load JSON file first (lowest priority)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--json") {
            if (i + 1 < argc) {
                try {
                    parse_json_file(argv[++i]);
                }
                catch (const std::exception& e) {
                    std::cerr << "Error loading JSON file: " << e.what() << "\n";
                }
            }
            else {
                std::cerr << "Error: --json requires a filename\n";
            }
        }
    }

    // Load config file second (overrides JSON)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config") {
            if (i + 1 < argc) {
                try {
                    parse_config_file(argv[++i]);
                }
                catch (const std::exception& e) {
                    std::cerr << "Error loading config file: " << e.what() << "\n";
                }
            }
            else {
                std::cerr << "Error: --config requires a filename\n";
            }
        }
    }

    // Final pass: command line args (highest priority)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            continue;
        }

        if (arg == "--json" || arg == "--config") {
            if (i + 1 < argc)
                ++i;
            continue;
        }

        if (arg == "--dump-config") {
            if (i + 1 < argc) {
                // write_config_file locks internally; release ours briefly.
                write_config_file(argv[++i]);
                std::cout << "Configuration written to " << argv[i] << "\n";
            }
            else {
                std::cerr << "Error: --dump-config requires a filename\n";
            }
            continue;
        }

        if (arg == "--dump-json") {
            if (i + 1 < argc) {
                write_json_file(argv[++i]);
                std::cout << "JSON configuration written to " << argv[i] << "\n";
            }
            else {
                std::cerr << "Error: --dump-json requires a filename\n";
            }
            continue;
        }

        if (arg.substr(0, 2) == "--") {
            std::string key = arg.substr(2);

            if (i + 1 < argc && argv[i + 1][0] != '-') {
                std::string value = argv[++i];
                try {
                    set_knob_value(key, value);
                }
                catch (const std::exception& e) {
                    std::cerr << "Error setting knob: " << e.what() << "\n";
                }
            }
            else {
                try {
                    set_knob_value(key, "true");
                }
                catch (const std::exception& e) {
                    std::cerr << "Error setting knob: " << e.what() << "\n";
                }
            }
        }
    }
    current_strict = true;
}

auto
parse_config_file(const std::string& filename) -> void
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + filename);
    }

    std::string line;
    int         line_number = 0;

    while (std::getline(file, line)) {
        ++line_number;

        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty())
            continue;

        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) {
            std::cerr << "Warning: Invalid line " << line_number
                      << " in config file (missing '=')\n";
            continue;
        }

        std::string key   = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);

        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        try {
            set_knob_value(key, value);
        }
        catch (const std::exception& e) {
            std::cerr << "Error on line " << line_number << ": " << e.what() << "\n";
        }
    }
}

auto
parse_json_file(const std::string& filename) -> void
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + filename);
    }

    nlohmann::json j;
    try {
        file >> j;
    }
    catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Failed to parse JSON file: " + std::string(e.what()));
    }

    for (auto& [key, val] : j.items()) {
        if (val.is_object()) {
            for (auto& [knob_name, knob_val] : val.items()) {
                std::string full_name = key + "." + knob_name;
                try {
                    std::string str_val =
                        knob_val.is_string() ? knob_val.get<std::string>() : knob_val.dump();
                    set_knob_value(full_name, str_val);
                }
                catch (const std::exception& e) {
                    std::cerr << "Error in JSON for key '" << full_name << "': " << e.what()
                              << "\n";
                }
            }
        }
        else {
            try {
                std::string str_val = val.is_string() ? val.get<std::string>() : val.dump();
                set_knob_value(key, str_val);
            }
            catch (const std::exception& e) {
                std::cerr << "Error in JSON for key '" << key << "': " << e.what() << "\n";
            }
        }
    }
}

auto
write_json_file(const std::string& filename) -> void
{
    nlohmann::json j;

    for (const auto& module_name : module_order) {
        auto it = module_knob_lists.find(module_name);
        if (it == module_knob_lists.end())
            continue;

        nlohmann::json module_json;
        for (const auto& knob_name : it->second->get_knob_names()) {
            KnobBase* knob = it->second->get_knob(knob_name);
            if (!knob)
                continue;

            std::string type = knob->type_name();
            std::string str  = knob->to_string();

            if (type == "int")
                module_json[knob_name] = std::stoi(str);
            else if (type == "double")
                module_json[knob_name] = std::stod(str);
            else if (type == "float")
                module_json[knob_name] = std::stof(str);
            else if (type == "bool")
                module_json[knob_name] = (str == "1" || str == "true");
            else
                module_json[knob_name] = str;
        }
        j[module_name] = module_json;
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file for writing: " + filename);
    }
    file << j.dump(4) << "\n";
}

auto
write_config_file(const std::string& filename, bool verbose) -> void
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file for writing: " + filename);
    }

    file << "# Configuration file generated by csim::config\n";
    file << "# Usage: ./program --config " << filename << "\n\n";

    for (const auto& module_name : module_order) {
        auto it = module_knob_lists.find(module_name);
        if (it == module_knob_lists.end())
            continue;

        file << "# Module: " << module_name << "\n";
        file << "# " << std::string(70, '-') << "\n";

        for (const auto& knob_name : it->second->get_knob_names()) {
            KnobBase* knob = it->second->get_knob(knob_name);
            if (!knob)
                continue;

            std::string full_name = module_name + "." + knob_name;
            if (verbose) {
                file << "# " << knob->get_description() << "\n";
                file << "# Type: " << knob->type_name() << "\n";
            }
            file << full_name << " = " << knob->to_string() << "\n";
            if (verbose)
                file << "\n";
        }
    }
}

auto
print_help() -> void
{
    std::cout << "Available knobs:\n\n";

    size_t max_name_len = 0;
    for (const auto& [full_name, knob] : all_knobs) {
        max_name_len = std::max(max_name_len, full_name.length());
    }

    for (const auto& module_name : module_order) {
        auto it = module_knob_lists.find(module_name);
        if (it == module_knob_lists.end())
            continue;

        std::cout << "Module: " << module_name << "\n";
        std::cout << std::string(70, '-') << "\n";

        for (const auto& knob_name : it->second->get_knob_names()) {
            KnobBase* knob = it->second->get_knob(knob_name);
            if (!knob)
                continue;

            std::string full_name = module_name + "." + knob_name;
            std::cout << "  --" << std::left << std::setw(max_name_len + 2) << full_name;
            std::cout << " " << knob->get_description();
            std::cout << " (type: " << knob->type_name();
            std::cout << ", default: " << knob->to_string() << ")\n";
        }
        std::cout << "\n";
    }

    std::cout << "Usage:\n";
    std::cout << "  Set knob values using --module.knob_name value\n";
    std::cout << "  Boolean knobs can be set with just --module.knob_name (sets to true)\n";
    std::cout << "  Load JSON file with --json filename (lowest priority)\n";
    std::cout << "  Load config file with --config filename (overrides JSON)\n";
    std::cout << "  Command line arguments override both JSON and config file\n";
    std::cout << "  Save current config with --dump-config filename\n";
    std::cout << "  Save current config as JSON with --dump-json filename\n";
}

auto
check_for_help(int argc, char* argv[]) -> void
{
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_help();
            std::exit(0);
        }
    }
}

auto
get_knob(const std::string& full_name) -> KnobBase*
{
    auto it = all_knobs.find(full_name);
    return (it != all_knobs.end()) ? it->second : nullptr;
}

auto
reset_all() -> void
{
    for (auto& [name, knob_list] : module_knob_lists) {
        knob_list->reset_all();
    }
}

auto
clear() -> void
{
    all_knobs.clear();
    module_knob_lists.clear();
    module_order.clear();
}

auto
remove_module(const std::string& module_name) -> void
{
    auto module_it = module_knob_lists.find(module_name);
    if (module_it == module_knob_lists.end())
        return;

    for (const auto& knob_name : module_it->second->get_knob_names()) {
        all_knobs.erase(module_name + "." + knob_name);
    }
    module_knob_lists.erase(module_it);

    auto order_it = std::find(module_order.begin(), module_order.end(), module_name);
    if (order_it != module_order.end()) {
        module_order.erase(order_it);
    }
}

} // namespace csim::config