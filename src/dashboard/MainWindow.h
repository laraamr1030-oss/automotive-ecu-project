#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QThread>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include "CANWorker.h"
#include "SpeedometerWidget.h"
#include "BatteryGaugeWidget.h"
#include "CheckEngineIcon.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void updateTurnSignalDisplay();

private:
    QString describeFaultFlags(int flags) const;

    CANWorker *can_worker_{nullptr};
    QThread   *can_thread_{nullptr};

    // --- Engine ECU (0x0C0) ---
    SpeedometerWidget *speedometer_{nullptr};
    QLabel *engine_speed_label_{nullptr};
    QLabel *coolant_temp_label_{nullptr};
    QLabel *throttle_label_{nullptr};
    QLabel *system_voltage_label_{nullptr};

    // --- Transmission ECU (0x0D0) ---
    QLabel *gear_label_{nullptr};

    // --- BMS ECU (0x350) ---
    BatteryGaugeWidget *battery_gauge_{nullptr};
    QLabel *pack_temp_label_{nullptr};
    QLabel *pack_voltage_label_{nullptr};

    // --- Fault status (0x0C1) + diagnostics ---
    CheckEngineIcon *check_engine_icon_{nullptr};
    QLabel *dtc_count_label_{nullptr};
    QLabel *fault_flags_label_{nullptr};
    QLabel *diag_status_label_{nullptr};
    QPushButton *clear_dtc_button_{nullptr};

    QLabel *door_fl_label_{nullptr};
    QLabel *door_fr_label_{nullptr};
    QLabel *door_rl_label_{nullptr};
    QLabel *door_rr_label_{nullptr};

    QLabel *turn_signal_label_{nullptr};

    bool current_turn_l{false};
    bool current_turn_r{false};
    bool current_hazard{false};
};

#endif // MAINWINDOW_H
