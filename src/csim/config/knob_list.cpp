#include "csim/config/knob_list.h"

namespace csim {
KnobList::KnobList(const std::string& module_name) : module_name(module_name) {}

auto
KnobList::get_knob(const std::string& name) const -> KnobBase*
{
    auto it = knobs.find(name);
    return (it != knobs.end()) ? it->second.get() : nullptr;
}

auto
KnobList::reset_all() -> void
{
    for (auto& [name, knob] : knobs) {
        knob->reset_to_default();
    }
}
} // namespace csim