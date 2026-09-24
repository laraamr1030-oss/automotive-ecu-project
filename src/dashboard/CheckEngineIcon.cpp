#include "CheckEngineIcon.h"
#include <QByteArray>

namespace {
constexpr const char* kGreyColor  = "#5a5a6e";  // default: no fault
constexpr const char* kAmberColor = "#ff9800";  // lit: fault active
}

CheckEngineIcon::CheckEngineIcon(QWidget* parent) : QSvgWidget(parent) {
    setFixedSize(48, 48);
    render(kGreyColor);
}

void CheckEngineIcon::setActive(bool active) {
    if (active == active_) return;   // avoid pointless re-renders
    active_ = active;
    render(active_ ? kAmberColor : kGreyColor);
}

void CheckEngineIcon::render(const QString& color) {
    // A simple engine-block silhouette, recolored in place rather than
    // swapped between two image files. QSvgWidget::load(QByteArray) accepts
    // raw SVG markup directly -- no image files needed at all.
    QString svg = QString(
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'>"
        "<g fill='none' stroke='%1' stroke-width='4' "
        "stroke-linejoin='round' stroke-linecap='round'>"
        "<path d='M10 40 h8 v-8 h8 v-8 h8 v8 h8 v8 h8'/>"
        "<rect x='14' y='40' width='36' height='14' rx='2'/>"
        "<line x1='20' y1='54' x2='20' y2='60'/>"
        "<line x1='44' y1='54' x2='44' y2='60'/>"
        "<circle cx='20' cy='60' r='2' fill='%1'/>"
        "<circle cx='44' cy='60' r='2' fill='%1'/>"
        "</g></svg>"
    ).arg(color);

    load(svg.toUtf8());
}