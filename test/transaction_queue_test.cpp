#include "models/transaction_queue.h"
#include "csim/core/payload.h"
#include "csim/core/sim_types.h"
#include "protocols/chi.h"

#include <gtest/gtest.h>

namespace csim::test {
namespace {

auto
make_req_payload(uint64_t txn_uid, addr_t address) -> payload_ptr
{
    auto p      = std::make_shared<Payload>(txn_uid);
    p->protocol = chi::chi_fields{
        .channel = chi::ChiChannel::REQ,
        .address = address,
        .req     = {.opcode = chi::ReqOpcode::ReadShared},
    };
    return p;
}

} // namespace

class TransactionQueueTest : public ::testing::Test {
public:
    sim_t            sim;
    TransactionQueue tq{sim, 8};
    int              completed            = 0;
    time_ps          last_completion_time = 0;

    auto acquire_hold_release(payload_ptr req, time_ps hold_duration) -> proc_t
    {
        TQEntry slot{tq, req};
        co_await slot.wait_for_grant();
        co_await sim.timeout(hold_duration);
        completed++;
        last_completion_time = sim.now();
    }
};

TEST_F(TransactionQueueTest, SameAddressSerialized)
{
    // Both same address. Second should wait for first.
    acquire_hold_release(make_req_payload(1, 0x1000), 1000);
    acquire_hold_release(make_req_payload(2, 0x1000), 1000);

    sim.run();

    EXPECT_EQ(completed, 2);
    // Total time should be ~2000 (serialized), not ~1000.
    EXPECT_GE(last_completion_time, 2000);
}

TEST_F(TransactionQueueTest, DifferentAddressesConcurrent)
{
    acquire_hold_release(make_req_payload(1, 0x1000), 1000);
    acquire_hold_release(make_req_payload(2, 0x2000), 1000);

    sim.run();

    EXPECT_EQ(completed, 2);
    // Should finish around 1000 (concurrent), not 2000 (serialized).
    EXPECT_LE(last_completion_time, 1500);
}
} // namespace csim::test
