#pragma once

#include <functional>
#include <memory>
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

    auto send(payload_ptr p) -> void { peer_rx_(std::move(p)); }

    [[nodiscard]] auto rx() const -> callback_t { return rx_; }

    auto set_peer(callback_t peer_rx) -> void { peer_rx_ = std::move(peer_rx); }

private:
    callback_t rx_;
    callback_t peer_rx_;
};

inline auto
bind(Port& a, Port& b) -> void
{
    a.set_peer(b.rx());
    b.set_peer(a.rx());
}
} // namespace csim