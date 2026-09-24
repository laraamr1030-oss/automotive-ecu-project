#include "MainWindow.h"
#include <QThread>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto *central_widget = new QWidget(this);
    auto *main_layout = new QVBoxLayout(central_widget);

    central_widget->setStyleSheet(R"(
        QWidget {
            background-color: #0b132b;
            color: #87ceeb;
            font-family: 'Segoe UI', Arial, sans-serif;
            font-size: 14px;
            font-weight: bold;
        }
        QLabel {
            background-color: #1c2541;
            border: 1px solid #3a506b;
            border-radius: 8px;
            padding: 8px;
            margin: 2px;
            qproperty-alignment: AlignCenter;
        }
    )");

    // --- Engine ECU + fault icon row ---
    speedometer_ = new SpeedometerWidget(this);
    engine_speed_label_ = new QLabel("RPM: 0", this);
    coolant_temp_label_ = new QLabel("Coolant: 0 \u00b0C", this);
    gear_label_ = new QLabel("Gear: -", this);
    check_engine_icon_ = new CheckEngineIcon(this);

    main_layout->addWidget(speedometer_);

    auto *engine_layout = new QHBoxLayout();
    engine_layout->addWidget(engine_speed_label_);
    engine_layout->addWidget(coolant_temp_label_);
    engine_layout->addWidget(gear_label_);
    engine_layout->addWidget(check_engine_icon_);
    main_layout->addLayout(engine_layout);

    // --- BMS / battery pack section ---
    soc_label_ = new QLabel("SoC: 0%", this);
    pack_temp_label_ = new QLabel("Pack Temp: 0 \u00b0C", this);
    charging_label_ = new QLabel("Charging: NO", this);
    pack_voltage_label_ = new QLabel("Pack Voltage: 0 V", this);

    auto *battery_layout = new QGridLayout();
    battery_layout->addWidget(soc_label_, 0, 0);
    battery_layout->addWidget(pack_temp_label_, 0, 1);
    battery_layout->addWidget(charging_label_, 1, 0);
    battery_layout->addWidget(pack_voltage_label_, 1, 1);
    main_layout->addLayout(battery_layout);

    // --- Doors ---
    door_fl_label_ = new QLabel("Front Left: CLOSED", this);
    door_fr_label_ = new QLabel("Front Right: CLOSED", this);
    door_rl_label_ = new QLabel("Rear Left: CLOSED", this);
    door_rr_label_ = new QLabel("Rear Right: CLOSED", this);
    turn_signal_label_ = new QLabel("Signals: OFF", this);

    auto *door_layout = new QGridLayout();
    door_layout->addWidget(door_fl_label_, 0, 0);
    door_layout->addWidget(door_fr_label_, 0, 1);
    door_layout->addWidget(door_rl_label_, 1, 0);
    door_layout->addWidget(door_rr_label_, 1, 1);
    main_layout->addLayout(door_layout);

    main_layout->addWidget(turn_signal_label_);
    setCentralWidget(central_widget);

    can_worker_ = new CANWorker();
    auto *can_thread = new QThread(this);
    can_worker_->moveToThread(can_thread);

    connect(can_thread, &QThread::started, can_worker_, &CANWorker::process);
    connect(can_thread, &QThread::finished, can_worker_, &QObject::deleteLater);

    // --- Engine ECU (0x0C0) ---
    connect(can_worker_, &CANWorker::engineSpeedUpdated, this, [this](int rpm) {
        engine_speed_label_->setText(QString("RPM: %1").arg(rpm));
    });

    connect(can_worker_, &CANWorker::coolantTempUpdated, this, [this](float temp) {
        coolant_temp_label_->setText(QString("Coolant: %1 \u00b0C").arg(temp, 0, 'f', 1));
    });

    // --- Transmission ECU (0x0D0) ---
    // NOTE: vehicleSpeedUpdated now carries a float (km/h with 0.01 scale
    // per the signal dictionary), so this lambda must take float, not int -
    // taking int here would silently truncate the decimal.
    connect(can_worker_, &CANWorker::vehicleSpeedUpdated, this, [this](float speed) {
        if (speedometer_) speedometer_->setSpeed(speed);
    });

    connect(can_worker_, &CANWorker::currentGearUpdated, this, [this](int gear) {
        gear_label_->setText(QString("Gear: %1").arg(gear));
    });

    // --- BMS ECU (0x350) ---
    connect(can_worker_, &CANWorker::socUpdated, this, [this](float percent) {
        soc_label_->setText(QString("SoC: %1%").arg(percent, 0, 'f', 1));
    });

    connect(can_worker_, &CANWorker::packTempUpdated, this, [this](float temp) {
        pack_temp_label_->setText(QString("Pack Temp: %1 \u00b0C").arg(temp, 0, 'f', 1));
    });

    connect(can_worker_, &CANWorker::chargingStateUpdated, this, [this](bool charging) {
        charging_label_->setText(charging ? "Charging: YES" : "Charging: NO");
    });

    connect(can_worker_, &CANWorker::packVoltageUpdated, this, [this](float voltage) {
        pack_voltage_label_->setText(QString("Pack Voltage: %1 V").arg(voltage, 0, 'f', 1));
    });

    // --- Fault indicator ---
    // Currently driven by CANWorker::simulateFault() until Member 2's real
    // DTC/fault frame is wired into decodeAndEmit().
    connect(can_worker_, &CANWorker::faultStatusUpdated, check_engine_icon_, &CheckEngineIcon::setActive);

    // --- Body Control Module (0x320) ---
    connect(can_worker_, &CANWorker::doorStatusUpdated, this,
            [this](bool fl, bool fr, bool rl, bool rr) {
        door_fl_label_->setText(fl ? "Front Left: OPEN" : "Front Left: CLOSED");
        door_fr_label_->setText(fr ? "Front Right: OPEN" : "Front Right: CLOSED");
        door_rl_label_->setText(rl ? "Rear Left: OPEN" : "Rear Left: CLOSED");
        door_rr_label_->setText(rr ? "Rear Right: OPEN" : "Rear Right: CLOSED");
    });

    connect(can_worker_, &CANWorker::turnSignalsChanged, this,
            [this](bool left, bool right) {
        current_turn_l = left;
        current_turn_r = right;
        updateTurnSignalDisplay();
    });

    connect(can_worker_, &CANWorker::hazardChanged, this,
            [this](bool hazard) {
        current_hazard = hazard;
        updateTurnSignalDisplay();
    });

    can_thread->start();
}

MainWindow::~MainWindow() {
    if (can_worker_) {
        can_worker_->stop();
    }
}

void MainWindow::updateTurnSignalDisplay() {
    if (current_hazard) {
        turn_signal_label_->setText("Signals: HAZARD ON");
    } else if (current_turn_l && current_turn_r) {
        turn_signal_label_->setText("Signals: BOTH ON");
    } else if (current_turn_l) {
        turn_signal_label_->setText("Signals: LEFT <<<");
    } else if (current_turn_r) {
        turn_signal_label_->setText("Signals: RIGHT >>>");
    } else {
        turn_signal_label_->setText("Signals: OFF");
    }
}
