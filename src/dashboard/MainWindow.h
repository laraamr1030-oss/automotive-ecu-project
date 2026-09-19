#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include "CANWorker.h"
#include "SpeedometerWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void updateTurnSignalDisplay();

private:
    CANWorker *can_worker_{nullptr};

    SpeedometerWidget *speedometer_{nullptr};
    QLabel *engine_speed_label_{nullptr};
    QLabel *fuel_label_{nullptr};
    QLabel *coolant_temp_label_{nullptr};
    QLabel *battery_label_{nullptr};
    
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
