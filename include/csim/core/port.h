#pragma once

#include <functional>
#include "csim/core/sim_types.h"
#include "payload.h"

namespace csim {

class Port {
public:
    using callback_t = std::function<void(payload_ptr)>;

    template <typename Self> auto on_receive(Self* self, void (Self::*method)(payload_ptr)) -> void
    {
        rx_ = [self, method](payload_ptr p) -> auto { (self->*method)(std::move(p)); };
    }

    auto send(payload_ptr p) -> void { peer_->rx_(std::move(p)); }

    auto set_peer(Port* peer) -> void { peer_ = peer; }

private:
    callback_t rx_;
    Port*      peer_ = nullptr;

    friend auto bind(Port& a, Port& b) -> void;
};

inline auto
bind(Port& a, Port& b) -> void
{
    a.peer_ = &b;
    b.peer_ = &a;
}

} // namespace csim