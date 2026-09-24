#pragma once
#include <QSvgWidget>
#include <QString>

// A check-engine ("CEL") icon rendered from inline SVG markup, recolored at
// runtime rather than swapped between two static image files. Default grey
// when no fault is active; lit (amber) when CANWorker tells us a fault is
// active via faultStatusUpdated. This widget never decides for itself
// whether there's a fault -- it only ever displays what it's told.
class CheckEngineIcon : public QSvgWidget {
    Q_OBJECT
public:
    explicit CheckEngineIcon(QWidget* parent = nullptr);

public slots:
    void setActive(bool active);

private:
    void render(const QString& color);
    bool active_ = false;
};