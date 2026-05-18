#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include "csim/core/sim_types.h"
#include "protocols/chi.h"

namespace csim {

struct Payload {
    const uint64_t flit_id;
    uint64_t       txn_uid = 0;

    time_ps start_time = 0;
    time_ps end_time   = 0;

    // protocol identity
    std::variant<std::monostate, chi::chi_fields, chi::chi_credit_fields> protocol;

    // orthogonal extensions (deferred — empty for now, mechanism in place)
    std::vector<std::shared_ptr<struct extension_base>> extensions;

    // Default: no transaction context. Used for credits and similar.
    Payload() : flit_id(next_flit_id()) {}

    // Child flit: caller provides the transaction's uid (typically copied
    // from a related payload).
    explicit Payload(uint64_t txn_uid) : flit_id(next_flit_id()), txn_uid(txn_uid) {}

    // Factory for the originating flit of a new transaction.
    static auto make_new_transaction() -> std::shared_ptr<Payload>;

    template <typename T> auto is() const -> bool { return std::holds_alternative<T>(protocol); }
    template <typename T> auto as() -> T& { return std::get<T>(protocol); }
    template <typename T> auto as() const -> const T& { return std::get<T>(protocol); }

    // Like as<T>(), but asserts the variant holds T. Use in handlers that
    // know by construction what type they should receive — fail loud if
    // wiring is wrong.
    template <typename T> auto require_as() -> T&
    {
        assert(is<T>() && "payload variant type mismatch in require_as");
        return as<T>();
    }
    template <typename T> auto require_as() const -> const T&
    {
        assert(is<T>() && "payload variant type mismatch in require_as");
        return as<T>();
    }

private:
    static auto next_flit_id() -> uint64_t;
    static auto next_txn_uid() -> uint64_t;
};

using payload_ptr = std::shared_ptr<Payload>;

} // namespace csim