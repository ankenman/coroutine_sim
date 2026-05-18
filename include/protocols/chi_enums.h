#pragma once

#include <cstdint>

namespace csim::chi {

enum class ChiChannel : uint8_t { REQ, WDAT, SRSP, CRSP, RDAT, SNP };

// ARM CHI REQ Channel Opcodes
// Based on Table B13.12 from ARM CHI Specification (IHI0050G)
enum class ReqOpcode : uint8_t {
    ReqLCrdReturn                = 0x00,
    ReadShared                   = 0x01,
    ReadClean                    = 0x02,
    ReadOnce                     = 0x03,
    ReadNoSnp                    = 0x04,
    PCrdReturn                   = 0x05,
    ReadUnique                   = 0x07,
    CleanShared                  = 0x08,
    CleanInvalid                 = 0x09,
    MakeInvalid                  = 0x0A,
    CleanUnique                  = 0x0B,
    MakeUnique                   = 0x0C,
    Evict                        = 0x0D,
    ReadNoSnpSep                 = 0x11,
    CleanSharedPersistSep        = 0x13,
    DVMOp                        = 0x14,
    WriteEvictFull               = 0x15,
    WriteCleanFull               = 0x17,
    WriteUniquePtl               = 0x18,
    WriteUniqueFull              = 0x19,
    WriteBackPtl                 = 0x1A,
    WriteBackFull                = 0x1B,
    WriteNoSnpPtl                = 0x1C,
    WriteNoSnpFull               = 0x1D,
    WriteUniqueFullStash         = 0x20,
    WriteUniquePtlStash          = 0x21,
    StashOnceShared              = 0x22,
    StashOnceUnique              = 0x23,
    ReadOnceCleanInvalid         = 0x24,
    ReadOnceMakeInvalid          = 0x25,
    ReadNotSharedDirty           = 0x26,
    CleanSharedPersist           = 0x27,
    AtomicStore_0x28             = 0x28,
    AtomicStore_0x29             = 0x29,
    AtomicStore_0x2A             = 0x2A,
    AtomicStore_0x2B             = 0x2B,
    AtomicStore_0x2C             = 0x2C,
    AtomicStore_0x2D             = 0x2D,
    AtomicStore_0x2E             = 0x2E,
    AtomicStore_0x2F             = 0x2F,
    AtomicLoad                   = 0x30,
    AtomicSwap                   = 0x38,
    AtomicCompare                = 0x39,
    PrefetchTgt                  = 0x3A,
    MakeReadUnique               = 0x41,
    WriteEvictOrEvict            = 0x42,
    WriteUniqueZero              = 0x43,
    WriteNoSnpZero               = 0x44,
    StashOnceSepShared           = 0x47,
    StashOnceSepUnique           = 0x48,
    ReadPreferUnique             = 0x4C,
    CleanInvalidPoPAa            = 0x4D,
    WriteNoSnpDefa               = 0x4E,
    WriteNoSnpFullCleanSh        = 0x50,
    WriteNoSnpFullCleanInv       = 0x51,
    WriteNoSnpFullCleanShPerSep  = 0x52,
    WriteUniqueFullCleanSh       = 0x54,
    WriteUniqueFullCleanShPerSep = 0x56,
    WriteBackFullCleanSh         = 0x58,
    WriteBackFullCleanInv        = 0x59,
    WriteBackFullCleanShPerSep   = 0x5A,
    WriteCleanFullCleanSh        = 0x5C,
    WriteCleanFullCleanShPerSep  = 0x5E,
    WriteNoSnpPtlCleanSh         = 0x60,
    WriteNoSnpPtlCleanInv        = 0x61,
    WriteNoSnpPtlCleanShPerSep   = 0x62,
    WriteUniquePtlCleanSh        = 0x64,
    WriteUniquePtlCleanShPerSep  = 0x66,
    WriteNoSnpPtlCleanInvPoPAa   = 0x70,
    WriteNoSnpFullCleanInvPoPAa  = 0x71,
    WriteBackFullCleanInvPoPAa   = 0x79,
    Uninitialized                = 0xFF
};

// Sub-opcodes for AtomicStore and AtomicLoad operations
enum class AtomicSubOpcode : uint8_t {
    ADD           = 0x00, // 000
    CLR           = 0x01, // 001
    EOR           = 0x02, // 010
    SET           = 0x03, // 011
    SMAX          = 0x04, // 100
    SMIN          = 0x05, // 101
    UMAX          = 0x06, // 110
    UMIN          = 0x07, // 111
    Uninitialized = 0xFF
};

// RSP Channel Opcodes (Table B13.14)
enum class RspOpcode : uint8_t {
    RespLCrdReturn = 0x00,
    SnpResp        = 0x01,
    CompAck        = 0x02,
    RetryAck       = 0x03,
    Comp           = 0x04,
    CompDBIDResp   = 0x05,
    DBIDResp       = 0x06,
    PCrdGrant      = 0x07,
    ReadReceipt    = 0x08,
    SnpRespFwded   = 0x09,
    TagMatch       = 0x0A,
    RespSepData    = 0x0B,
    Persist        = 0x0C,
    CompPersist    = 0x0D,
    DBIDRespOrd    = 0x0E,
    StashDone      = 0x10,
    CompStashDone  = 0x11,
    CompCMO        = 0x14,
    Uninitialized  = 0xFF
};

// SNP Channel Opcodes (Table B13.15)
enum class SnpOpcode : uint8_t {
    SnpLCrdReturn        = 0x00,
    SnpShared            = 0x01,
    SnpClean             = 0x02,
    SnpOnce              = 0x03,
    SnpNotSharedDirty    = 0x04,
    SnpUniqueStash       = 0x05,
    SnpMakeInvalidStash  = 0x06,
    SnpUnique            = 0x07,
    SnpCleanShared       = 0x08,
    SnpCleanInvalid      = 0x09,
    SnpMakeInvalid       = 0x0A,
    SnpStashUnique       = 0x0B,
    SnpStashShared       = 0x0C,
    SnpDVMOp             = 0x0D,
    SnpQuery             = 0x10,
    SnpSharedFwd         = 0x11,
    SnpCleanFwd          = 0x12,
    SnpOnceFwd           = 0x13,
    SnpNotSharedDirtyFwd = 0x14,
    SnpPreferUnique      = 0x15,
    SnpPreferUniqueFwd   = 0x16,
    SnpUniqueFwd         = 0x17,
    Uninitialized        = 0xFF
};

// DAT Channel Opcodes (Table B13.16)
enum class DatOpcode : uint8_t {
    DataLCrdReturn       = 0x00,
    SnpRespData          = 0x01,
    CopyBackWriteData    = 0x02,
    NonCopyBackWriteData = 0x03,
    CompData             = 0x04,
    SnpRespDataPtl       = 0x05,
    SnpRespDataFwded     = 0x06,
    WriteDataCancel      = 0x07,
    DataSepResp          = 0x0B,
    NCBWrDataCompAck     = 0x0C,
    Uninitialized        = 0xFF
};

// Response field encoding — 3 bits on the wire.
// Interpretation depends on channel/opcode context.
// Normal Comp/CompData: I, SC, UC, I_PD, SC_PD, UC_PD
// Snoop responses:      I, SC, UD, SD, I_PD, SC_PD, UD_PD, SD_PD
// (UC and UD share 0x2; UC_PD and UD_PD share 0x6 — context disambiguates)
enum class Resp : uint8_t {
    I              = 0x0,
    SC             = 0x1,
    UC_or_UD       = 0x2,
    SD             = 0x3, // snoop responses only
    I_PD           = 0x4,
    SC_PD          = 0x5,
    UC_PD_or_UD_PD = 0x6,
    SD_PD          = 0x7, // snoop responses only
    Uninitialized  = 0xFF,
};

// 2-bit RespErr field
enum class RespErr : uint8_t {
    OK            = 0x0, // NormalOkay
    EXOK          = 0x1, // ExclusiveOkay
    DERR          = 0x2, // DataError
    NDERR         = 0x3, // NonDataError
    Uninitialized = 0xFF,
};

// 1-bit TagMatch result
enum class TagMatchResult : uint8_t {
    Fail          = 0x0,
    Pass          = 0x1,
    Uninitialized = 0xFF,
};

inline auto
is_read_opcode(ReqOpcode opcode) -> bool
{
    switch (opcode) {
    case ReqOpcode::ReadShared:
    case ReqOpcode::ReadUnique:
    case ReqOpcode::ReadClean:
    case ReqOpcode::ReadNoSnp:
    case ReqOpcode::ReadOnce:
        return true;
    default:
        return false;
    }
}
inline auto
is_write_opcode(ReqOpcode opcode) -> bool
{
    switch (opcode) {
    case ReqOpcode::WriteUniqueFull:
    case ReqOpcode::WriteUniquePtl:
    case ReqOpcode::WriteNoSnpFull:
    case ReqOpcode::WriteNoSnpPtl:
    case ReqOpcode::WriteBackFull:
    case ReqOpcode::WriteBackPtl:
    case ReqOpcode::WriteCleanFull:
    case ReqOpcode::WriteEvictFull:
        return true;
    default:
        return false;
    }
}
inline auto
is_dataless_opcode(ReqOpcode opcode) -> bool
{
    switch (opcode) {
    case ReqOpcode::CleanUnique:
    case ReqOpcode::CleanShared:
    case ReqOpcode::CleanInvalid:
    case ReqOpcode::MakeUnique:
    case ReqOpcode::MakeInvalid:
    case ReqOpcode::Evict:
        return true;
    default:
        return false;
    }
}
} // namespace csim::chi

#include <magic_enum/magic_enum.hpp>

template <> struct magic_enum::customize::enum_range<csim::chi::ReqOpcode> {
    static constexpr int min = 0;
    static constexpr int max = 255;
};
template <> struct magic_enum::customize::enum_range<csim::chi::RspOpcode> {
    static constexpr int min = 0;
    static constexpr int max = 255;
};
template <> struct magic_enum::customize::enum_range<csim::chi::SnpOpcode> {
    static constexpr int min = 0;
    static constexpr int max = 255;
};
template <> struct magic_enum::customize::enum_range<csim::chi::DatOpcode> {
    static constexpr int min = 0;
    static constexpr int max = 255;
};
template <> struct magic_enum::customize::enum_range<csim::chi::Resp> {
    static constexpr int min = 0;
    static constexpr int max = 255;
};
template <> struct magic_enum::customize::enum_range<csim::chi::RespErr> {
    static constexpr int min = 0;
    static constexpr int max = 255;
};
template <> struct magic_enum::customize::enum_range<csim::chi::TagMatchResult> {
    static constexpr int min = 0;
    static constexpr int max = 255;
};
template <> struct magic_enum::customize::enum_range<csim::chi::AtomicSubOpcode> {
    static constexpr int min = 0;
    static constexpr int max = 255;
};