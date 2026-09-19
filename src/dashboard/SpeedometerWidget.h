#pragma once
#include <QWidget>

// Fully implemented — this is Session 10 material, not part of this lab.
// You do not need to modify this file.

class SpeedometerWidget : public QWidget {
    Q_OBJECT
public:
    explicit SpeedometerWidget(QWidget* parent = nullptr);
    void setSpeed(float speed_kmh);
    float speed() const { return speed_; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    float speed_ = 0.0f;
    float max_speed_ = 220.0f;
};
