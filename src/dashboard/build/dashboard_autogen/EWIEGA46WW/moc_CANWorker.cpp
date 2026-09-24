/****************************************************************************
** Meta object code from reading C++ file 'CANWorker.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../CANWorker.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CANWorker.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CANWorker_t {
    QByteArrayData data[37];
    char stringdata0[399];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CANWorker_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CANWorker_t qt_meta_stringdata_CANWorker = {
    {
QT_MOC_LITERAL(0, 0, 9), // "CANWorker"
QT_MOC_LITERAL(1, 10, 18), // "engineSpeedUpdated"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 3), // "rpm"
QT_MOC_LITERAL(4, 34, 18), // "coolantTempUpdated"
QT_MOC_LITERAL(5, 53, 11), // "tempCelsius"
QT_MOC_LITERAL(6, 65, 19), // "vehicleSpeedUpdated"
QT_MOC_LITERAL(7, 85, 8), // "speedKmh"
QT_MOC_LITERAL(8, 94, 18), // "currentGearUpdated"
QT_MOC_LITERAL(9, 113, 4), // "gear"
QT_MOC_LITERAL(10, 118, 17), // "doorStatusUpdated"
QT_MOC_LITERAL(11, 136, 2), // "fl"
QT_MOC_LITERAL(12, 139, 2), // "fr"
QT_MOC_LITERAL(13, 142, 2), // "rl"
QT_MOC_LITERAL(14, 145, 2), // "rr"
QT_MOC_LITERAL(15, 148, 18), // "turnSignalsChanged"
QT_MOC_LITERAL(16, 167, 4), // "left"
QT_MOC_LITERAL(17, 172, 5), // "right"
QT_MOC_LITERAL(18, 178, 13), // "hazardChanged"
QT_MOC_LITERAL(19, 192, 6), // "hazard"
QT_MOC_LITERAL(20, 199, 10), // "socUpdated"
QT_MOC_LITERAL(21, 210, 7), // "percent"
QT_MOC_LITERAL(22, 218, 15), // "packTempUpdated"
QT_MOC_LITERAL(23, 234, 20), // "chargingStateUpdated"
QT_MOC_LITERAL(24, 255, 8), // "charging"
QT_MOC_LITERAL(25, 264, 18), // "packVoltageUpdated"
QT_MOC_LITERAL(26, 283, 5), // "volts"
QT_MOC_LITERAL(27, 289, 18), // "faultStatusUpdated"
QT_MOC_LITERAL(28, 308, 6), // "active"
QT_MOC_LITERAL(29, 315, 16), // "fuelLevelUpdated"
QT_MOC_LITERAL(30, 332, 5), // "level"
QT_MOC_LITERAL(31, 338, 21), // "batteryVoltageUpdated"
QT_MOC_LITERAL(32, 360, 7), // "voltage"
QT_MOC_LITERAL(33, 368, 7), // "process"
QT_MOC_LITERAL(34, 376, 3), // "run"
QT_MOC_LITERAL(35, 380, 4), // "stop"
QT_MOC_LITERAL(36, 385, 13) // "simulateFault"

    },
    "CANWorker\0engineSpeedUpdated\0\0rpm\0"
    "coolantTempUpdated\0tempCelsius\0"
    "vehicleSpeedUpdated\0speedKmh\0"
    "currentGearUpdated\0gear\0doorStatusUpdated\0"
    "fl\0fr\0rl\0rr\0turnSignalsChanged\0left\0"
    "right\0hazardChanged\0hazard\0socUpdated\0"
    "percent\0packTempUpdated\0chargingStateUpdated\0"
    "charging\0packVoltageUpdated\0volts\0"
    "faultStatusUpdated\0active\0fuelLevelUpdated\0"
    "level\0batteryVoltageUpdated\0voltage\0"
    "process\0run\0stop\0simulateFault"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CANWorker[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      18,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      14,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  104,    2, 0x06 /* Public */,
       4,    1,  107,    2, 0x06 /* Public */,
       6,    1,  110,    2, 0x06 /* Public */,
       8,    1,  113,    2, 0x06 /* Public */,
      10,    4,  116,    2, 0x06 /* Public */,
      15,    2,  125,    2, 0x06 /* Public */,
      18,    1,  130,    2, 0x06 /* Public */,
      20,    1,  133,    2, 0x06 /* Public */,
      22,    1,  136,    2, 0x06 /* Public */,
      23,    1,  139,    2, 0x06 /* Public */,
      25,    1,  142,    2, 0x06 /* Public */,
      27,    1,  145,    2, 0x06 /* Public */,
      29,    1,  148,    2, 0x06 /* Public */,
      31,    1,  151,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      33,    0,  154,    2, 0x0a /* Public */,
      34,    0,  155,    2, 0x0a /* Public */,
      35,    0,  156,    2, 0x0a /* Public */,
      36,    1,  157,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Float,    5,
    QMetaType::Void, QMetaType::Float,    7,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void, QMetaType::Bool, QMetaType::Bool, QMetaType::Bool, QMetaType::Bool,   11,   12,   13,   14,
    QMetaType::Void, QMetaType::Bool, QMetaType::Bool,   16,   17,
    QMetaType::Void, QMetaType::Bool,   19,
    QMetaType::Void, QMetaType::Float,   21,
    QMetaType::Void, QMetaType::Float,    5,
    QMetaType::Void, QMetaType::Bool,   24,
    QMetaType::Void, QMetaType::Float,   26,
    QMetaType::Void, QMetaType::Bool,   28,
    QMetaType::Void, QMetaType::Float,   30,
    QMetaType::Void, QMetaType::Float,   32,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   28,

       0        // eod
};

void CANWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CANWorker *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->engineSpeedUpdated((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->coolantTempUpdated((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 2: _t->vehicleSpeedUpdated((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 3: _t->currentGearUpdated((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->doorStatusUpdated((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3])),(*reinterpret_cast< bool(*)>(_a[4]))); break;
        case 5: _t->turnSignalsChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 6: _t->hazardChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->socUpdated((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 8: _t->packTempUpdated((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 9: _t->chargingStateUpdated((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->packVoltageUpdated((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 11: _t->faultStatusUpdated((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 12: _t->fuelLevelUpdated((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 13: _t->batteryVoltageUpdated((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 14: _t->process(); break;
        case 15: _t->run(); break;
        case 16: _t->stop(); break;
        case 17: _t->simulateFault((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CANWorker::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CANWorker::engineSpeedUpdated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CANWorker::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CANWorker::coolantTempUpdated)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CANWorker::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CANWorker::vehicleSpeedUpdated)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CANWorker::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CANWorker::currentGearUpdated)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (CANWorker::*)(bool , bool , bool , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CANWorker::doorStatusUpdated)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (CANWorker::*)(bool , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CANWorker::turnSignalsChanged)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (CANWorker::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CANWorker::hazardChanged)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (CANWorker::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CANWorker::socUpdated)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (CANWorker::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CANWorker::packTempUpdated)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (CANWorker::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CANWorker::chargingStateUpdated)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (CANWorker::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CANWorker::packVoltageUpdated)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (CANWorker::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CANWorker::faultStatusUpdated)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (CANWorker::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CANWorker::fuelLevelUpdated)) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (CANWorker::*)(float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CANWorker::batteryVoltageUpdated)) {
                *result = 13;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CANWorker::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CANWorker.data,
    qt_meta_data_CANWorker,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CANWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CANWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CANWorker.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int CANWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 18;
    }
    return _id;
}

// SIGNAL 0
void CANWorker::engineSpeedUpdated(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CANWorker::coolantTempUpdated(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void CANWorker::vehicleSpeedUpdated(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CANWorker::currentGearUpdated(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void CANWorker::doorStatusUpdated(bool _t1, bool _t2, bool _t3, bool _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void CANWorker::turnSignalsChanged(bool _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void CANWorker::hazardChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void CANWorker::socUpdated(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void CANWorker::packTempUpdated(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void CANWorker::chargingStateUpdated(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void CANWorker::packVoltageUpdated(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void CANWorker::faultStatusUpdated(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void CANWorker::fuelLevelUpdated(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void CANWorker::batteryVoltageUpdated(float _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
