#ifndef CANWORKER_H
#define CANWORKER_H

#include <QObject>
#include <QtGlobal>
#include <cstdint>
#include <linux/can.h>

class CANWorker : public QObject {
    Q_OBJECT

public:
    explicit CANWorker(QObject *parent = nullptr);
    ~CANWorker() override;

public slots:
    void process();
    void run();
    void stop();

    // TEMPORARY: lets you light the Check Engine icon by hand from a button
    // or a timer, until Member 2's fault frame ID/format is finalized and
    // we can decode it for real inside decodeAndEmit(). Delete this once
    // real fault frames are wired up below.
    void simulateFault(bool active);

signals:
    // --- Engine ECU, CAN ID 0x0C0 ---
    void engineSpeedUpdated(int rpm);
    void coolantTempUpdated(float tempCelsius);

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

    // --- Fault status (wire this up for real once Member 2's DTC frame
    //     format / CAN ID is decided) ---
    void faultStatusUpdated(bool active);

    void fuelLevelUpdated(float level);
    void batteryVoltageUpdated(float voltage);

private:
    bool openSocket();
    void decodeAndEmit(canid_t can_id, const uint8_t *data, uint8_t dlc);

    bool running_{true};
    bool stop_{false};
    int can_socket_{-1};
};

#endif // CANWORKER_H