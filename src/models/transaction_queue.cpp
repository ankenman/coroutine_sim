#include "models/transaction_queue.h"

#include <cassert>
#include <utility>

namespace csim {

TransactionQueue::TransactionQueue(sim_t& sim, size_t capacity) : sim(sim), capacity(capacity) {}

auto
TransactionQueue::allocate(payload_ptr req) -> Entry*
{
    assert(entries.size() < capacity && "TQ full — should have sent RetryAck");

    const txn_uid_t txn_uid = req->txn_uid;
    const addr_t    address = req->require_as<chi::chi_fields>().address;

    auto entry = std::make_unique<Entry>(Entry{
        .txn_uid       = txn_uid,
        .address       = address,
        .hazard_done   = sim.event(),
        .next_in_chain = nullptr,
    });

    Entry* entry_ptr = entry.get();
    entries[txn_uid] = std::move(entry);

    auto chain_it = chain_head.find(address);
    if (chain_it == chain_head.end()) {
        chain_head[address] = entry_ptr;
        chain_tail[address] = entry_ptr;
        entry_ptr->hazard_done.trigger();
    }
    else {
        chain_tail[address]->next_in_chain = entry_ptr;
        chain_tail[address]                = entry_ptr;
    }

    return entry_ptr;
}

auto
TransactionQueue::release(Entry* entry) -> void
{
    if (entry == nullptr) {
        return;
    }

    const addr_t    address = entry->address;
    const txn_uid_t txn_uid = entry->txn_uid;

    if (entry->next_in_chain != nullptr) {
        Entry* next         = entry->next_in_chain;
        chain_head[address] = next;
        next->hazard_done.trigger();
    }
    else {
        chain_head.erase(address);
        chain_tail.erase(address);
    }

    entries.erase(txn_uid);
}

TQEntry::TQEntry(TransactionQueue& tq, payload_ptr req) : tq(tq), entry(tq.allocate(std::move(req)))
{
}

TQEntry::~TQEntry()
{
    tq.release(entry);
}

auto
TQEntry::wait_for_grant() -> event_t
{
    return entry->hazard_done;
}

} // namespace csim