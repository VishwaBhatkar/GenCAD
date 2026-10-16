/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/MainWindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.2. It"
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
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN10MainWindowE = QtMocHelpers::stringData(
    "MainWindow",
    "newDrawing",
    "",
    "openDrawing",
    "saveDrawing",
    "saveDrawingAs",
    "exportDxf",
    "importDxf",
    "deleteSelected",
    "selectAll",
    "clearSelection",
    "zoomFit",
    "showAbout",
    "toggleSnap",
    "toggleGridSnap",
    "toggleOrtho",
    "togglePolar",
    "showPageSetup",
    "executeCadCommand",
    "command",
    "onExactDimensionsCommitted",
    "v1",
    "v2",
    "v3",
    "openCloudWorkspace",
    "saveToCloud",
    "openFromCloud",
    "openAiAssistant",
    "aiGenerateFromPrompt",
    "aiAutoDimension",
    "aiAnalyzeDrawing",
    "aiCleanDrawing",
    "onCoordinatesChanged",
    "worldPos",
    "onSelectionChanged",
    "onModifiedChanged",
    "modified",
    "onFilePathChanged",
    "path",
    "updateStatusBarStatus",
    "updateCloudStatusBar"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN10MainWindowE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      32,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  206,    2, 0x0a,    1 /* Public */,
       3,    0,  207,    2, 0x0a,    2 /* Public */,
       4,    0,  208,    2, 0x0a,    3 /* Public */,
       5,    0,  209,    2, 0x0a,    4 /* Public */,
       6,    0,  210,    2, 0x0a,    5 /* Public */,
       7,    0,  211,    2, 0x0a,    6 /* Public */,
       8,    0,  212,    2, 0x0a,    7 /* Public */,
       9,    0,  213,    2, 0x0a,    8 /* Public */,
      10,    0,  214,    2, 0x0a,    9 /* Public */,
      11,    0,  215,    2, 0x0a,   10 /* Public */,
      12,    0,  216,    2, 0x0a,   11 /* Public */,
      13,    0,  217,    2, 0x0a,   12 /* Public */,
      14,    0,  218,    2, 0x0a,   13 /* Public */,
      15,    0,  219,    2, 0x0a,   14 /* Public */,
      16,    0,  220,    2, 0x0a,   15 /* Public */,
      17,    0,  221,    2, 0x0a,   16 /* Public */,
      18,    1,  222,    2, 0x0a,   17 /* Public */,
      20,    3,  225,    2, 0x0a,   19 /* Public */,
      24,    0,  232,    2, 0x0a,   23 /* Public */,
      25,    0,  233,    2, 0x0a,   24 /* Public */,
      26,    0,  234,    2, 0x0a,   25 /* Public */,
      27,    0,  235,    2, 0x0a,   26 /* Public */,
      28,    0,  236,    2, 0x0a,   27 /* Public */,
      29,    0,  237,    2, 0x0a,   28 /* Public */,
      30,    0,  238,    2, 0x0a,   29 /* Public */,
      31,    0,  239,    2, 0x0a,   30 /* Public */,
      32,    1,  240,    2, 0x08,   31 /* Private */,
      34,    0,  243,    2, 0x08,   33 /* Private */,
      35,    1,  244,    2, 0x08,   34 /* Private */,
      37,    1,  247,    2, 0x08,   36 /* Private */,
      39,    0,  250,    2, 0x08,   38 /* Private */,
      40,    0,  251,    2, 0x08,   39 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   19,
    QMetaType::Void, QMetaType::Double, QMetaType::Double, QMetaType::Double,   21,   22,   23,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPointF,   33,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   36,
    QMetaType::Void, QMetaType::QString,   38,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_ZN10MainWindowE.offsetsAndSizes,
    qt_meta_data_ZN10MainWindowE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN10MainWindowE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'newDrawing'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openDrawing'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveDrawing'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'saveDrawingAs'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'exportDxf'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'importDxf'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'deleteSelected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'selectAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'zoomFit'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showAbout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleSnap'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleGridSnap'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleOrtho'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'togglePolar'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showPageSetup'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'executeCadCommand'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onExactDimensionsCommitted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'openCloudWorkspace'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveToCloud'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openFromCloud'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openAiAssistant'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'aiGenerateFromPrompt'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'aiAutoDimension'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'aiAnalyzeDrawing'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'aiCleanDrawing'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCoordinatesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>,
        // method 'onSelectionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onModifiedChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onFilePathChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateStatusBarStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateCloudStatusBar'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->newDrawing(); break;
        case 1: _t->openDrawing(); break;
        case 2: { bool _r = _t->saveDrawing();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 3: { bool _r = _t->saveDrawingAs();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 4: _t->exportDxf(); break;
        case 5: _t->importDxf(); break;
        case 6: _t->deleteSelected(); break;
        case 7: _t->selectAll(); break;
        case 8: _t->clearSelection(); break;
        case 9: _t->zoomFit(); break;
        case 10: _t->showAbout(); break;
        case 11: _t->toggleSnap(); break;
        case 12: _t->toggleGridSnap(); break;
        case 13: _t->toggleOrtho(); break;
        case 14: _t->togglePolar(); break;
        case 15: _t->showPageSetup(); break;
        case 16: _t->executeCadCommand((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 17: _t->onExactDimensionsCommitted((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[3]))); break;
        case 18: _t->openCloudWorkspace(); break;
        case 19: _t->saveToCloud(); break;
        case 20: _t->openFromCloud(); break;
        case 21: _t->openAiAssistant(); break;
        case 22: _t->aiGenerateFromPrompt(); break;
        case 23: _t->aiAutoDimension(); break;
        case 24: _t->aiAnalyzeDrawing(); break;
        case 25: _t->aiCleanDrawing(); break;
        case 26: _t->onCoordinatesChanged((*reinterpret_cast< std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 27: _t->onSelectionChanged(); break;
        case 28: _t->onModifiedChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 29: _t->onFilePathChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 30: _t->updateStatusBarStatus(); break;
        case 31: _t->updateCloudStatusBar(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN10MainWindowE.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 32)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 32;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 32)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 32;
    }
    return _id;
}
QT_WARNING_POP
