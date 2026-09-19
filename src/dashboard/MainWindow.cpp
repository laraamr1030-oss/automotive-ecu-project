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

    speedometer_ = new SpeedometerWidget(this);
    engine_speed_label_ = new QLabel("RPM: 0", this);
    fuel_label_ = new QLabel("Fuel: 0%", this);
    coolant_temp_label_ = new QLabel("Coolant: 0 °C", this);
    battery_label_ = new QLabel("Battery: 0 V", this);

    door_fl_label_ = new QLabel("Front Left: CLOSED", this);
    door_fr_label_ = new QLabel("Front Right: CLOSED", this);
    door_rl_label_ = new QLabel("Rear Left: CLOSED", this);
    door_rr_label_ = new QLabel("Rear Right: CLOSED", this);
    turn_signal_label_ = new QLabel("Signals: OFF", this);

    main_layout->addWidget(speedometer_);
    main_layout->addWidget(engine_speed_label_);
    main_layout->addWidget(fuel_label_);
    main_layout->addWidget(coolant_temp_label_);
    main_layout->addWidget(battery_label_);
    
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

    connect(can_worker_, &CANWorker::vehicleSpeedUpdated, this, [this](int speed) {
        if (speedometer_) speedometer_->setSpeed(speed);
    });

    connect(can_worker_, &CANWorker::engineSpeedUpdated, this, [this](int rpm) {
        engine_speed_label_->setText(QString("RPM: %1").arg(rpm));
    });

    connect(can_worker_, &CANWorker::fuelLevelUpdated, this, [this](float level) {
        fuel_label_->setText(QString("Fuel: %1%").arg(level, 0, 'f', 1));
    });

    connect(can_worker_, &CANWorker::coolantTempUpdated, this, [this](float temp) {
        coolant_temp_label_->setText(QString("Coolant: %1 °C").arg(temp, 0, 'f', 1));
    });

    connect(can_worker_, &CANWorker::batteryVoltageUpdated, this, [this](float voltage) {
        battery_label_->setText(QString("Battery: %1 V").arg(voltage, 0, 'f', 1));
    });

    connect(can_worker_, &CANWorker::doorStatusUpdated, this,
            [this](bool fl, bool fr, bool rl, bool rr) {
        door_fl_label_->setText(fl ? "Front Left: OPEN" : "Front Left: CLOSED");
        door_fr_label_->setText(fr ? "Front Right: OPEN" : "Front Right: CLOSED");
        door_rl_label_->setText(rl ? "Rear Left: OPEN" : "Rear Left: CLOSED");
        door_rr_label_->setText(rr ? "Rear Right: CLOSED" : "Rear Right: CLOSED");
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
