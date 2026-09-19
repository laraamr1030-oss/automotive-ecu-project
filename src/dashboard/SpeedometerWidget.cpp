#include "SpeedometerWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <cmath>

SpeedometerWidget::SpeedometerWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(200, 200);
}

void SpeedometerWidget::setSpeed(float speed_kmh) {
    speed_ = std::clamp(speed_kmh, 0.0f, max_speed_);
    update();
}

void SpeedometerWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width(), h = height();
    int side = std::min(w, h);
    p.translate(w / 2.0, h / 2.0);
    p.scale(side / 220.0, side / 220.0);

    // Background circle
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(30, 30, 30));
    p.drawEllipse(-100, -100, 200, 200);

    // Arc (210° to -30° = 240° sweep)
    p.setPen(QPen(QColor(80, 80, 80), 8));
    p.setBrush(Qt::NoBrush);
    p.drawArc(-80, -80, 160, 160, 210 * 16, -240 * 16);

    // Speed markings
    p.setPen(QPen(Qt::white, 2));
    for (int s = 0; s <= 220; s += 20) {
        float angle_deg = 210.0f - (s / 220.0f * 240.0f);
        float rad = angle_deg * M_PI / 180.0f;
        p.drawLine(
            static_cast<int>(75 * std::cos(rad)), static_cast<int>(-75 * std::sin(rad)),
            static_cast<int>(85 * std::cos(rad)), static_cast<int>(-85 * std::sin(rad))
        );
    }

    // Needle
    float angle_deg = 210.0f - (speed_ / max_speed_ * 240.0f);
    float rad = angle_deg * M_PI / 180.0f;
    p.setPen(QPen(QColor(220, 50, 50), 3));
    p.drawLine(0, 0,
        static_cast<int>(70 * std::cos(rad)),
        static_cast<int>(-70 * std::sin(rad))
    );

    // Center hub
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(200, 200, 200));
    p.drawEllipse(-6, -6, 12, 12);

    // Speed text
    p.setPen(Qt::white);
    QFont font("Arial", 18, QFont::Bold);
    p.setFont(font);
    p.drawText(QRect(-40, 30, 80, 30), Qt::AlignCenter,
               QString::number(static_cast<int>(speed_)));

    p.setFont(QFont("Arial", 8));
    p.drawText(QRect(-30, 55, 60, 20), Qt::AlignCenter, "km/h");
}
