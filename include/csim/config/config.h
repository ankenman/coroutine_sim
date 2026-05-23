#pragma once

#include "knob_list.h"
#include <string>

namespace csim::config {

// Module registration — called by modules during construction
auto get_or_create(const std::string& module_name) -> KnobList&;
auto get_or_create(const char* module_name) -> KnobList&;

// Called by KnobList::add_knob to register the knob in the flat lookup
auto register_knob(const std::string& full_name, KnobBase* knob) -> void;

// Parsing
auto parse_command_line(int argc, char* argv[], bool strict = true) -> void;
auto parse_config_file(const std::string& filename) -> void;
auto parse_json_file(const std::string& filename) -> void;

// Writing
auto write_config_file(const std::string& filename, bool verbose = false) -> void;
auto write_json_file(const std::string& filename) -> void;

// Introspection
auto print_help() -> void;
auto check_for_help(int argc, char* argv[]) -> void;
auto get_knob(const std::string& full_name) -> KnobBase*;

// Lifecycle
auto reset_all() -> void;
auto clear() -> void;
auto remove_module(const std::string& module_name) -> void;

} // namespace csim::config

#include "knob_list.inl"