#include "MainWindow.h"
#include <QDebug>
#include <QStringList>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("EV Instrument Cluster");

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
        QPushButton {
            background-color: #3a506b;
            border: 1px solid #87ceeb;
            border-radius: 8px;
            padding: 8px 16px;
            margin: 2px;
        }
        QPushButton:hover   { background-color: #4f6d8f; }
        QPushButton:pressed { background-color: #22334a; }
    )");

    // --- Speedometer (custom QPainter gauge #1) ---
    speedometer_ = new SpeedometerWidget(this);
    main_layout->addWidget(speedometer_);

    // --- Engine row ---
    engine_speed_label_ = new QLabel("RPM: 0", this);
    coolant_temp_label_ = new QLabel("Coolant: 0 \u00b0C", this);
    gear_label_ = new QLabel("Gear: -", this);
    check_engine_icon_ = new CheckEngineIcon(this);

    auto *engine_layout = new QHBoxLayout();
    engine_layout->addWidget(engine_speed_label_);
    engine_layout->addWidget(coolant_temp_label_);
    engine_layout->addWidget(gear_label_);
    engine_layout->addWidget(check_engine_icon_);
    main_layout->addLayout(engine_layout);

    throttle_label_ = new QLabel("Throttle: 0 %", this);
    system_voltage_label_ = new QLabel("12 V System: 0 V", this);
    auto *engine2_layout = new QHBoxLayout();
    engine2_layout->addWidget(throttle_label_);
    engine2_layout->addWidget(system_voltage_label_);
    main_layout->addLayout(engine2_layout);

    // --- BMS / battery pack (custom QPainter gauge #2) ---
    battery_gauge_ = new BatteryGaugeWidget(this);
    pack_temp_label_ = new QLabel("Pack Temp: 0 \u00b0C", this);
    pack_voltage_label_ = new QLabel("Pack Voltage: 0 V", this);

    main_layout->addWidget(battery_gauge_);
    auto *battery_layout = new QHBoxLayout();
    battery_layout->addWidget(pack_temp_label_);
    battery_layout->addWidget(pack_voltage_label_);
    main_layout->addLayout(battery_layout);

    // --- Diagnostics row: stored DTCs, active fault conditions, clear button ---
    dtc_count_label_ = new QLabel("Stored DTCs: 0", this);
    fault_flags_label_ = new QLabel("Active faults: none", this);
    clear_dtc_button_ = new QPushButton("Clear DTCs (UDS 0x14)", this);
    diag_status_label_ = new QLabel("Diagnostics: idle", this);

    auto *diag_layout = new QHBoxLayout();
    diag_layout->addWidget(dtc_count_label_);
    diag_layout->addWidget(fault_flags_label_, 1);
    diag_layout->addWidget(clear_dtc_button_);
    main_layout->addLayout(diag_layout);
    main_layout->addWidget(diag_status_label_);

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

    // --- Worker thread (moveToThread pattern) ---
    can_worker_ = new CANWorker();
    can_thread_ = new QThread(this);
    can_worker_->moveToThread(can_thread_);

    connect(can_thread_, &QThread::started, can_worker_, &CANWorker::process);
    connect(can_thread_, &QThread::finished, can_worker_, &QObject::deleteLater);

    // --- Engine ECU (0x0C0) ---
    connect(can_worker_, &CANWorker::engineSpeedUpdated, this, [this](int rpm) {
        engine_speed_label_->setText(QString("RPM: %1").arg(rpm));
    });
    connect(can_worker_, &CANWorker::coolantTempUpdated, this, [this](float temp) {
        coolant_temp_label_->setText(QString("Coolant: %1 \u00b0C").arg(temp, 0, 'f', 1));
    });
    connect(can_worker_, &CANWorker::throttleUpdated, this, [this](float pct) {
        throttle_label_->setText(QString("Throttle: %1 %").arg(pct, 0, 'f', 1));
    });
    connect(can_worker_, &CANWorker::systemVoltageUpdated, this, [this](float volts) {
        system_voltage_label_->setText(QString("12 V System: %1 V").arg(volts, 0, 'f', 1));
    });

    // --- Transmission ECU (0x0D0) ---
    connect(can_worker_, &CANWorker::vehicleSpeedUpdated, this, [this](float speed) {
        if (speedometer_) speedometer_->setSpeed(speed);
    });
    connect(can_worker_, &CANWorker::currentGearUpdated, this, [this](int gear) {
        gear_label_->setText(QString("Gear: %1").arg(gear));
    });

    // --- BMS ECU (0x350) ---
    connect(can_worker_, &CANWorker::socUpdated, battery_gauge_, &BatteryGaugeWidget::setSoC);
    connect(can_worker_, &CANWorker::chargingStateUpdated, battery_gauge_, &BatteryGaugeWidget::setCharging);
    connect(can_worker_, &CANWorker::packTempUpdated, this, [this](float temp) {
        pack_temp_label_->setText(QString("Pack Temp: %1 \u00b0C").arg(temp, 0, 'f', 1));
    });
    connect(can_worker_, &CANWorker::packVoltageUpdated, this, [this](float voltage) {
        pack_voltage_label_->setText(QString("Pack Voltage: %1 V").arg(voltage, 0, 'f', 1));
    });

    // --- Fault status (0x0C1): REAL data from the Engine ECU's FaultManager ---
    connect(can_worker_, &CANWorker::faultStatusUpdated, check_engine_icon_, &CheckEngineIcon::setActive);
    connect(can_worker_, &CANWorker::dtcCountUpdated, this, [this](int count) {
        dtc_count_label_->setText(QString("Stored DTCs: %1").arg(count));
    });
    connect(can_worker_, &CANWorker::faultFlagsUpdated, this, [this](int flags) {
        fault_flags_label_->setText("Active faults: " + describeFaultFlags(flags));
    });

    // --- Phase 7: Clear DTCs from the dashboard itself ---
    connect(clear_dtc_button_, &QPushButton::clicked, this, [this]() {
        bool sent = can_worker_ && can_worker_->sendClearDtcRequest();
        diag_status_label_->setText(sent ? "Diagnostics: clear request sent (0x14)..."
                                         : "Diagnostics: could not send (no CAN socket)");
    });
    connect(can_worker_, &CANWorker::clearDtcResponse, this, [this](bool positive) {
        diag_status_label_->setText(positive ? "Diagnostics: ECU cleared all DTCs (positive response 0x54)"
                                             : "Diagnostics: ECU rejected the clear request (7F 14)");
    });

    // --- Body Control Module (0x320) ---
    connect(can_worker_, &CANWorker::doorStatusUpdated, this,
            [this](bool fl, bool fr, bool rl, bool rr) {
        door_fl_label_->setText(fl ? "Front Left: OPEN" : "Front Left: CLOSED");
        door_fr_label_->setText(fr ? "Front Right: OPEN" : "Front Right: CLOSED");
        door_rl_label_->setText(rl ? "Rear Left: OPEN" : "Rear Left: CLOSED");
        door_rr_label_->setText(rr ? "Rear Right: OPEN" : "Rear Right: CLOSED");
    });
    connect(can_worker_, &CANWorker::turnSignalsChanged, this, [this](bool left, bool right) {
        current_turn_l = left;
        current_turn_r = right;
        updateTurnSignalDisplay();
    });
    connect(can_worker_, &CANWorker::hazardChanged, this, [this](bool hazard) {
        current_hazard = hazard;
        updateTurnSignalDisplay();
    });

    can_thread_->start();
}

MainWindow::~MainWindow() {
    // Stop the worker, then wait for its thread to finish BEFORE Qt destroys
    // the QThread object (destroying a still-running QThread crashes).
    if (can_worker_) can_worker_->stop();
    if (can_thread_) {
        can_thread_->quit();
        can_thread_->wait(2000);
    }
}

QString MainWindow::describeFaultFlags(int flags) const {
    QStringList active;
    if (flags & 0x01) active << "P0117 Overheat";
    if (flags & 0x02) active << "P0300 Misfire";
    if (flags & 0x04) active << "P0563 12V High";
    if (flags & 0x08) active << "P0A7E Pack Overtemp";
    if (flags & 0x10) active << "P0AFA Pack Volt Low";
    return active.isEmpty() ? QString("none") : active.join(", ");
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
