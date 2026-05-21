#pragma once

#include <stdexcept>

namespace csim {
template <typename T>
auto
KnobList::add_knob(const std::string& knob_name, const std::string& description,
                   const T& default_value) -> Knob<T>&
{
    std::string full_name = module_name.empty() ? knob_name : module_name + "." + knob_name;

    auto     knob     = std::make_unique<Knob<T>>(full_name, description, default_value);
    Knob<T>* knob_ptr = knob.get();

    if (knobs.find(knob_name) != knobs.end()) {
        throw std::runtime_error("Knob '" + knob_name + "' already exists in module '" +
                                 module_name + "'");
    }

    knobs[knob_name] = std::move(knob);
    knob_order.push_back(knob_name);

    ConfigManager::instance().register_knob(full_name, knobs[knob_name].get());

    return *knob_ptr;
}
} // namespace csim