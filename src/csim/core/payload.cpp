#include "csim/core/payload.h"

namespace csim {
auto
Payload::make_new_transaction() -> std::shared_ptr<Payload>
{
    auto p = std::make_shared<Payload>(next_txn_uid());
    return p;
}

auto
Payload::next_flit_id() -> uint64_t
{
    static std::atomic<uint64_t> c{0};
    return c.fetch_add(1, std::memory_order_relaxed);
}

auto
Payload::next_txn_uid() -> uint64_t
{
    static std::atomic<uint64_t> c{0};
    return c.fetch_add(1, std::memory_order_relaxed);
}
} // namespace csim