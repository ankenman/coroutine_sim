#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "csim/core/payload.h"
#include "csim/core/sim_types.h"
#include "protocols/chi.h"

namespace csim {

class TransactionQueue {
public:
    TransactionQueue(sim_t& sim, size_t capacity);

    struct Entry {
        txn_uid_t txn_uid;
        addr_t    address;
        event_t   hazard_done;
        Entry*    next_in_chain;
    };

private:
    friend class TQEntry;

    auto allocate(payload_ptr req) -> Entry*;
    auto release(Entry* entry) -> void;

    sim_t& sim;
    size_t capacity;

    std::unordered_map<txn_uid_t, std::unique_ptr<Entry>> entries;
    std::unordered_map<addr_t, Entry*>                    chain_head;
    std::unordered_map<addr_t, Entry*>                    chain_tail;
};

class TQEntry {
public:
    TQEntry(TransactionQueue& tq, payload_ptr req);
    ~TQEntry();

    TQEntry(const TQEntry&)                    = delete;
    auto operator=(const TQEntry&) -> TQEntry& = delete;
    TQEntry(TQEntry&&)                         = delete;
    auto operator=(TQEntry&&) -> TQEntry&      = delete;

    auto wait_for_grant() -> event_t;

private:
    TransactionQueue&        tq;
    TransactionQueue::Entry* entry;
};

} // namespace csim