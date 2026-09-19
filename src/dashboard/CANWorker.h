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

signals:
    void vehicleSpeedUpdated(int speed);
    void engineSpeedUpdated(int rpm);
    void fuelLevelUpdated(float level);
    void coolantTempUpdated(float temp);
    void batteryVoltageUpdated(float voltage);
    void doorStatusUpdated(bool fl, bool fr, bool rl, bool rr);
    void turnSignalsChanged(bool left, bool right);
    void hazardChanged(bool hazard);

private:
    bool openSocket();
    void decodeAndEmit(canid_t can_id, const uint8_t *data, uint8_t dlc);

    bool running_{true};
    bool stop_{false};
    int can_socket_{-1};
};

#endif // CANWORKER_H
