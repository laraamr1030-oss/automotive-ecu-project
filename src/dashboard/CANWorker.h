#ifndef CANWORKER_H
#define CANWORKER_H

#include <QObject>
#include <QtGlobal>
#include <atomic>
#include <cstdint>
#include <linux/can.h>

// Runs on its own QThread (moveToThread). All blocking socket I/O happens
// here; the UI thread only ever receives the signals below.
class CANWorker : public QObject {
    Q_OBJECT

public:
    explicit CANWorker(QObject *parent = nullptr);
    ~CANWorker() override;

    // Phase 7: send UDS ClearDiagnosticInformation (0x14) to the Engine ECU.
    // Called DIRECTLY from the UI thread on purpose: run() blocks in poll(),
    // so a queued slot would never get a chance to execute. It is safe --
    // it is one atomic socket-fd read plus one write() of a single CAN frame.
    bool sendClearDtcRequest();

public slots:
    void process();   // connected to QThread::started
    void run();
    void stop();      // only sets a flag; run() notices within 100 ms and cleans up

signals:
    // --- Engine ECU, CAN ID 0x0C0 ---
    void engineSpeedUpdated(int rpm);
    void coolantTempUpdated(float tempCelsius);
    void throttleUpdated(float percent);
    void systemVoltageUpdated(float volts);

    // --- Fault status, CAN ID 0x0C1 ---
    void faultFlagsUpdated(int flags);      // which fault conditions are true right now
    void dtcCountUpdated(int count);        // DTCs stored (pending + confirmed)
    void faultStatusUpdated(bool celOn);    // warning lamp requested (a DTC is confirmed)

    // --- Transmission ECU, CAN ID 0x0D0 ---
    void vehicleSpeedUpdated(float speedKmh);
    void currentGearUpdated(int gear);

    // --- Body Control Module, CAN ID 0x320 ---
    void doorStatusUpdated(bool fl, bool fr, bool rl, bool rr);
    void turnSignalsChanged(bool left, bool right);
    void hazardChanged(bool hazard);

    // --- BMS ECU, CAN ID 0x350 ---
    void socUpdated(float percent);
    void packTempUpdated(float tempCelsius);
    void chargingStateUpdated(bool charging);
    void packVoltageUpdated(float volts);

    // --- UDS response 0x7E8 to our clear request ---
    void clearDtcResponse(bool positive);

private:
    bool openSocket();
    void decodeAndEmit(canid_t can_id, const uint8_t *data, uint8_t dlc);

    std::atomic<bool> stop_{false};
    std::atomic<int>  can_socket_{-1};
};

#endif // CANWORKER_H
