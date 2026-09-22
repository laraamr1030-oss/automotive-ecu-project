/****************************************************************************
** Meta object code from reading C++ file 'CANWorker.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../CANWorker.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CANWorker.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN9CANWorkerE_t {};
} // unnamed namespace

template <> constexpr inline auto CANWorker::qt_create_metaobjectdata<qt_meta_tag_ZN9CANWorkerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "CANWorker",
        "vehicleSpeedUpdated",
        "",
        "speed",
        "engineSpeedUpdated",
        "rpm",
        "fuelLevelUpdated",
        "level",
        "coolantTempUpdated",
        "temp",
        "batteryVoltageUpdated",
        "voltage",
        "doorStatusUpdated",
        "fl",
        "fr",
        "rl",
        "rr",
        "turnSignalsChanged",
        "left",
        "right",
        "hazardChanged",
        "hazard",
        "process",
        "run",
        "stop"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'vehicleSpeedUpdated'
        QtMocHelpers::SignalData<void(int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'engineSpeedUpdated'
        QtMocHelpers::SignalData<void(int)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Signal 'fuelLevelUpdated'
        QtMocHelpers::SignalData<void(float)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Float, 7 },
        }}),
        // Signal 'coolantTempUpdated'
        QtMocHelpers::SignalData<void(float)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Float, 9 },
        }}),
        // Signal 'batteryVoltageUpdated'
        QtMocHelpers::SignalData<void(float)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Float, 11 },
        }}),
        // Signal 'doorStatusUpdated'
        QtMocHelpers::SignalData<void(bool, bool, bool, bool)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 13 }, { QMetaType::Bool, 14 }, { QMetaType::Bool, 15 }, { QMetaType::Bool, 16 },
        }}),
        // Signal 'turnSignalsChanged'
        QtMocHelpers::SignalData<void(bool, bool)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 18 }, { QMetaType::Bool, 19 },
        }}),
        // Signal 'hazardChanged'
        QtMocHelpers::SignalData<void(bool)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 21 },
        }}),
        // Slot 'process'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'run'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'stop'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CANWorker, qt_meta_tag_ZN9CANWorkerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject CANWorker::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9CANWorkerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9CANWorkerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9CANWorkerE_t>.metaTypes,
    nullptr
} };

void CANWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CANWorker *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->vehicleSpeedUpdated((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->engineSpeedUpdated((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->fuelLevelUpdated((*reinterpret_cast<std::add_pointer_t<float>>(_a[1]))); break;
        case 3: _t->coolantTempUpdated((*reinterpret_cast<std::add_pointer_t<float>>(_a[1]))); break;
        case 4: _t->batteryVoltageUpdated((*reinterpret_cast<std::add_pointer_t<float>>(_a[1]))); break;
        case 5: _t->doorStatusUpdated((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[4]))); break;
        case 6: _t->turnSignalsChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 7: _t->hazardChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 8: _t->process(); break;
        case 9: _t->run(); break;
        case 10: _t->stop(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (CANWorker::*)(int )>(_a, &CANWorker::vehicleSpeedUpdated, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (CANWorker::*)(int )>(_a, &CANWorker::engineSpeedUpdated, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (CANWorker::*)(float )>(_a, &CANWorker::fuelLevelUpdated, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (CANWorker::*)(float )>(_a, &CANWorker::coolantTempUpdated, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (CANWorker::*)(float )>(_a, &CANWorker::batteryVoltageUpdated, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (CANWorker::*)(bool , bool , bool , bool )>(_a, &CANWorker::doorStatusUpdated, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (CANWorker::*)(bool , bool )>(_a, &CANWorker::turnSignalsChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (CANWorker::*)(bool )>(_a, &CANWorker::hazardChanged, 7))
            return;
    }
}

const QMetaObject *CANWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CANWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9CANWorkerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int CANWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void CANWorker::vehicleSpeedUpdated(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void CANWorker::engineSpeedUpdated(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void CANWorker::fuelLevelUpdated(float _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void CANWorker::coolantTempUpdated(float _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void CANWorker::batteryVoltageUpdated(float _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void CANWorker::doorStatusUpdated(bool _t1, bool _t2, bool _t3, bool _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 6
void CANWorker::turnSignalsChanged(bool _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2);
}

// SIGNAL 7
void CANWorker::hazardChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}
QT_WARNING_POP
