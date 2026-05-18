#include "models/transaction_inbox.h"
#include "csim/core/payload.h"
#include "csim/core/sim_types.h"
#include "protocols/chi.h"

#include <gtest/gtest.h>

namespace csim::test {
namespace {

auto
make_dat_payload(uint64_t txn_uid) -> payload_ptr
{
    auto p      = std::make_shared<Payload>(txn_uid);
    p->protocol = chi::chi_fields{
        .channel = chi::ChiChannel::RDAT,
        .dat     = {.opcode = chi::DatOpcode::CompData},
    };
    return p;
}

} // namespace

class TransactionInboxTest : public ::testing::Test {
public:
    sim_t            sim;
    TransactionInbox inbox{sim};
    bool             fired = false;

    auto await_arrival() -> proc_t
    {
        co_await inbox.arrival();
        fired = true;
    }

    auto await_data_chunks() -> proc_t
    {
        co_await inbox.all_data_chunks_received();
        fired = true;
    }

    auto await_snoops() -> proc_t
    {
        co_await inbox.all_snoops_received();
        fired = true;
    }
};

TEST_F(TransactionInboxTest, DeliverFiresArrival)
{
    await_arrival();
    inbox.deliver(make_dat_payload(1));
    sim.run();
    EXPECT_TRUE(fired);
    EXPECT_EQ(inbox.pending.size(), 1u);
}

TEST_F(TransactionInboxTest, ExpectZeroDataChunksFiresImmediately)
{
    inbox.expect_data_chunks(0);
    await_data_chunks();
    sim.run();
    EXPECT_TRUE(fired);
}

TEST_F(TransactionInboxTest, ExpectOneDataChunkFiresOnDelivery)
{
    inbox.expect_data_chunks(1);
    await_data_chunks();
    inbox.deliver(make_dat_payload(1));
    sim.run();
    EXPECT_TRUE(fired);
}

TEST_F(TransactionInboxTest, ExpectTwoDataChunksFiresOnSecondDelivery)
{
    inbox.expect_data_chunks(2);
    await_data_chunks();

    inbox.deliver(make_dat_payload(1));
    sim.run_until(1);
    EXPECT_FALSE(fired);

    inbox.deliver(make_dat_payload(1));
    sim.run();
    EXPECT_TRUE(fired);
}
} // namespace csim::test
