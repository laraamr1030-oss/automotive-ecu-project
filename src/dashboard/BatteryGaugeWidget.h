#pragma once
#include <QWidget>

// Custom-painted EV battery gauge (QPainter): outline + terminal nub, a fill
// bar whose width follows the state of charge and whose colour goes
// green -> amber -> red, and a "CHARGING" tag while the BMS reports charging.
class BatteryGaugeWidget : public QWidget {
    Q_OBJECT
public:
    explicit BatteryGaugeWidget(QWidget* parent = nullptr);
    void setSoC(float percent);
    void setCharging(bool charging);
    QSize sizeHint() const override { return QSize(320, 70); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    float soc_ = 0.0f;
    bool  charging_ = false;
};
