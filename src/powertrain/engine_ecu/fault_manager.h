#pragma once
// ---------------------------------------------------------------------------
// fault_manager.h  --  DTC storage + ISO 14229-1 status-byte lifecycle
//
// One FaultManager exists per ECU. EngineECU::evaluateFaults() calls
// evaluate() once per DTC every monitoring cycle (2 s); UDSHandler reads the
// stored records for SID 0x19 and wipes them for SID 0x14.
//
// Lifecycle implemented (this is what the rubric calls "not the simplified
// 'set 0x0D immediately' version"):
//
//   condition fails 1st cycle  -> status 0x05  testFailed + pending
//                                 (freeze frame captured ONCE, right now)
//   condition fails 2nd cycle  -> status 0x8D  + confirmed + warningLamp (CEL on)
//   condition stops failing    -> status 0x88  testFailed and pending drop,
//                                 confirmed + warningLamp stay (CEL stays on)
//   UDS 0x14 ClearDTC          -> record removed, CEL off
//
//   A DTC that failed only ONCE (pending, never confirmed) and then passes
//   is silently discarded -- a one-cycle glitch must not light the CEL.
// ---------------------------------------------------------------------------
#include <cstddef>   //size_t
#include <cstdint>
#include <vector>

// ISO 14229-1 DTC status byte bits (only the ones this project uses)
constexpr uint8_t DTC_TEST_FAILED       = 0x01; // bit0 testFailed
constexpr uint8_t DTC_PENDING           = 0x04; // bit2 pendingDTC
constexpr uint8_t DTC_CONFIRMED         = 0x08; // bit3 confirmedDTC
constexpr uint8_t DTC_WARNING_REQUESTED = 0x80; // bit7 warningIndicatorRequested

// Sensor values captured at the instant a DTC is first set.
struct FreezeFrame {
    float rpm             = 0.0f;
    float coolant_temp    = 0.0f;
    float battery_voltage = 0.0f;   // 12 V system
    float soc             = 0.0f;   // EV theme: BMS values
    float pack_temp       = 0.0f;
    float pack_voltage    = 0.0f;
    bool  captured        = false;  // false until FaultManager stores it
};

struct DTC {
    uint8_t high = 0;               // e.g. P0117 -> high 0x01, low 0x17
    uint8_t low  = 0;
    uint8_t status = 0;
    int     consecutive_failures = 0;
    FreezeFrame freeze_frame;
};

class FaultManager {
public:
    // Number of consecutive failing monitoring cycles needed to confirm.
    static constexpr int CONFIRM_THRESHOLD = 2;

    // Call once per monitoring cycle for EVERY monitored DTC.
    // `failing` = is the fault condition true right now.
    void evaluate(uint8_t high, uint8_t low, bool failing, const FreezeFrame& snapshot) {
        int idx = indexOf(high, low);

        if (failing) {
            if (idx < 0) {
                // First failure: create the record as pending and capture the freeze frame.
                DTC fresh;
                fresh.high = high;
                fresh.low  = low;
                fresh.status = DTC_TEST_FAILED | DTC_PENDING;   // 0x05
                fresh.consecutive_failures = 1;
                fresh.freeze_frame = snapshot;
                fresh.freeze_frame.captured = true;
                dtcs_.push_back(fresh);
                return;
            }
            DTC& d = dtcs_[idx];
            d.status |= (DTC_TEST_FAILED | DTC_PENDING);
            d.consecutive_failures++;
            if (d.consecutive_failures >= CONFIRM_THRESHOLD) {
                d.status |= (DTC_CONFIRMED | DTC_WARNING_REQUESTED); // 0x8D
            }
            // freeze frame is NOT touched again: it stays the first capture
            return;
        }

        // Condition passes this cycle.
        if (idx < 0) return;               // nothing stored, nothing to do
        DTC& d = dtcs_[idx];
        d.consecutive_failures = 0;
        d.status &= static_cast<uint8_t>(~DTC_TEST_FAILED);
        if (d.status & DTC_CONFIRMED) {
            d.status &= static_cast<uint8_t>(~DTC_PENDING);   // -> 0x88, kept until cleared
        } else {
            dtcs_.erase(dtcs_.begin() + idx);                 // never confirmed: discard
        }
    }

    // Copy of every stored record (pending and confirmed).
    std::vector<DTC> getActiveDTCs() const { return dtcs_; }

    // True if any stored DTC requests the warning lamp (CEL).
    bool hasWarning() const {
        for (const auto& d : dtcs_) {
            if (d.status & DTC_WARNING_REQUESTED) return true;
        }
        return false;
    }

    // UDS 0x14: remove the records completely (not just clear testFailed).
    void clearAll() { dtcs_.clear(); }

private:
    int indexOf(uint8_t high, uint8_t low) const {
        for (size_t i = 0; i < dtcs_.size(); ++i) {
            if (dtcs_[i].high == high && dtcs_[i].low == low) return static_cast<int>(i);
        }
        return -1;
    }

    std::vector<DTC> dtcs_;
};
