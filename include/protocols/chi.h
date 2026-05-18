#pragma once

#include <cstdint>

#include "../csim/core/sim_types.h"
#include "protocols/chi_enums.h"

namespace csim::chi {

struct chi_fields {
    // common header — meaningful on all channels
    ChiChannel channel;
    addr_t     address = 0;
    uint32_t   txn_id  = 0;
    uint8_t    qos     = 0;

    // REQ-channel fields
    struct {
        ReqOpcode opcode        = ReqOpcode::Uninitialized;
        uint32_t  src_id        = 0;
        uint32_t  tgt_id        = 0;
        uint32_t  return_nid    = 0;
        uint32_t  return_txn_id = 0;
        bool      allow_retry   = true;
        uint8_t   order         = 0b00;
        uint8_t   size          = 0b110;
        struct {
            bool allocate  = false;
            bool cacheable = false;
            bool device    = false;
            bool ewa       = false;
        } mem_attr;
        bool snp_attr      = false;
        bool do_dwt        = false;
        bool likely_shared = false;
        bool exp_comp_ack  = false;
    } req;

    // RSP-channel fields
    struct {
        RspOpcode opcode    = RspOpcode::Uninitialized;
        uint32_t  src_id    = 0;
        uint32_t  tgt_id    = 0;
        Resp      resp      = Resp::Uninitialized;
        Resp      fwd_state = Resp::Uninitialized;
        uint8_t   pcrd_type = 0;
        uint16_t  dbid      = 0;
    } rsp;

    // SNP-channel fields
    struct {
        SnpOpcode opcode          = SnpOpcode::Uninitialized;
        uint32_t  src_id          = 0;
        uint32_t  tgt_id          = 0;
        uint32_t  fwd_nid         = 0;
        uint32_t  fwd_txn_id      = 0;
        bool      ret_to_src      = false;
        bool      do_not_go_to_sd = false;
    } snp;

    // DAT-channel fields
    struct {
        DatOpcode opcode    = DatOpcode::Uninitialized;
        uint32_t  src_id    = 0;
        uint32_t  tgt_id    = 0;
        uint32_t  home_n_id = 0;
        Resp      resp      = Resp::Uninitialized;
        Resp      fwd_state = Resp::Uninitialized;
        uint16_t  dbid      = 0;
    } dat;
};

struct chi_credit_fields {
    ChiChannel channel;
    uint32_t   source_id = 0;
};

struct chi_txn_key {
    uint32_t src_id;
    uint32_t txn_id;

    auto operator==(const chi_txn_key&) const -> bool = default;
};

} // namespace csim::chi

namespace std {
// This implementation is based on Boost hash combine (where the magic numbers come from)
template <> struct hash<csim::chi::chi_txn_key> {
    auto operator()(const csim::chi::chi_txn_key& k) const noexcept -> size_t
    {
        const auto h1 = std::hash<uint32_t>{}(k.src_id);
        const auto h2 = std::hash<uint32_t>{}(k.txn_id);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};
} // namespace std