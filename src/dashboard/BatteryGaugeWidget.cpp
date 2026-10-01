#include "BatteryGaugeWidget.h"
#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPen>
#include <algorithm>

BatteryGaugeWidget::BatteryGaugeWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(60);
}

void BatteryGaugeWidget::setSoC(float percent) {
    float clamped = std::clamp(percent, 0.0f, 100.0f);
    if (clamped == soc_) return;
    soc_ = clamped;
    update();                       // schedule a repaint
}

void BatteryGaugeWidget::setCharging(bool charging) {
    if (charging == charging_) return;
    charging_ = charging;
    update();
}

void BatteryGaugeWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Battery body + the small terminal nub on the right
    const QRectF body(6, 8, width() - 30, height() - 16);
    const QRectF nub(body.right() + 2, height() / 2.0 - 10, 12, 20);

    p.setPen(QPen(QColor("#87ceeb"), 3));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(body, 8, 8);
    p.setBrush(QColor("#87ceeb"));
    p.drawRoundedRect(nub, 3, 3);

    // Fill bar: width proportional to SoC, colour by level
    QColor fill = QColor("#2ecc71");                  // > 50 % green
    if (soc_ <= 50.0f) fill = QColor("#ffb703");      // 20-50 % amber
    if (soc_ <= 20.0f) fill = QColor("#e63946");      // <= 20 % red
    const QRectF inner = body.adjusted(5, 5, -5, -5);
    QRectF bar = inner;
    bar.setWidth(inner.width() * soc_ / 100.0);
    p.setPen(Qt::NoPen);
    p.setBrush(fill);
    p.drawRoundedRect(bar, 5, 5);

    // Text on top
    QFont f = font();
    f.setBold(true);
    f.setPointSize(16);
    p.setFont(f);
    p.setPen(Qt::white);
    QString text = QString::number(soc_, 'f', 0) + "%";
    if (charging_) text += "   CHARGING";
    p.drawText(body, Qt::AlignCenter, text);
}
