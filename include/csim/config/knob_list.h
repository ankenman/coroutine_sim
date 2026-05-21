#pragma once

#include "knob_base.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace csim {
class ConfigManager;

class KnobList {
public:
    explicit KnobList(const std::string& module_name = "");

    template <typename T>
    auto add_knob(const std::string& knob_name, const std::string& description,
                  const T& default_value) -> Knob<T>&;

    auto get_knob(const std::string& name) const -> KnobBase*;

    auto reset_all() -> void;

    auto get_module_name() const -> const std::string& { return module_name; }

    auto get_knob_names() const -> const std::vector<std::string>& { return knob_order; }

private:
    std::string                                                module_name;
    std::unordered_map<std::string, std::unique_ptr<KnobBase>> knobs;
    std::vector<std::string>                                   knob_order;
};
} // namespace csim
