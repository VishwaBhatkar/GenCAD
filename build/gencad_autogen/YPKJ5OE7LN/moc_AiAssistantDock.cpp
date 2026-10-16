/****************************************************************************
** Meta object code from reading C++ file 'AiAssistantDock.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/AiAssistantDock.h"
#include <QtGui/qtextcursor.h>
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AiAssistantDock.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN15AiAssistantDockE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN15AiAssistantDockE = QtMocHelpers::stringData(
    "AiAssistantDock",
    "promptForGeneration",
    "",
    "samplePrompt",
    "runAnalysis",
    "runAutoDimension",
    "runAutoClean",
    "onGenerateClicked",
    "onPreviewPlanClicked",
    "onApplyProposalClicked",
    "onDiscardProposalClicked",
    "onAnalyzeClicked",
    "onCleanClicked",
    "onDimensionAllClicked",
    "onDimensionSelectedClicked",
    "onRagSearchClicked",
    "onQuickPromptClicked",
    "onQuickRagTopicClicked",
    "onProviderChanged",
    "index",
    "onClarificationOptionClicked"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN15AiAssistantDockE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      18,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  122,    2, 0x0a,    1 /* Public */,
       1,    0,  125,    2, 0x2a,    3 /* Public | MethodCloned */,
       4,    0,  126,    2, 0x0a,    4 /* Public */,
       5,    0,  127,    2, 0x0a,    5 /* Public */,
       6,    0,  128,    2, 0x0a,    6 /* Public */,
       7,    0,  129,    2, 0x08,    7 /* Private */,
       8,    0,  130,    2, 0x08,    8 /* Private */,
       9,    0,  131,    2, 0x08,    9 /* Private */,
      10,    0,  132,    2, 0x08,   10 /* Private */,
      11,    0,  133,    2, 0x08,   11 /* Private */,
      12,    0,  134,    2, 0x08,   12 /* Private */,
      13,    0,  135,    2, 0x08,   13 /* Private */,
      14,    0,  136,    2, 0x08,   14 /* Private */,
      15,    0,  137,    2, 0x08,   15 /* Private */,
      16,    0,  138,    2, 0x08,   16 /* Private */,
      17,    0,  139,    2, 0x08,   17 /* Private */,
      18,    1,  140,    2, 0x08,   18 /* Private */,
      20,    0,  143,    2, 0x08,   20 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
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
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   19,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject AiAssistantDock::staticMetaObject = { {
    QMetaObject::SuperData::link<QDockWidget::staticMetaObject>(),
    qt_meta_stringdata_ZN15AiAssistantDockE.offsetsAndSizes,
    qt_meta_data_ZN15AiAssistantDockE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN15AiAssistantDockE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AiAssistantDock, std::true_type>,
        // method 'promptForGeneration'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'promptForGeneration'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'runAnalysis'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'runAutoDimension'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'runAutoClean'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onGenerateClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPreviewPlanClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onApplyProposalClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDiscardProposalClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAnalyzeClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCleanClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDimensionAllClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDimensionSelectedClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRagSearchClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onQuickPromptClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onQuickRagTopicClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onProviderChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onClarificationOptionClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void AiAssistantDock::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AiAssistantDock *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->promptForGeneration((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->promptForGeneration(); break;
        case 2: _t->runAnalysis(); break;
        case 3: _t->runAutoDimension(); break;
        case 4: _t->runAutoClean(); break;
        case 5: _t->onGenerateClicked(); break;
        case 6: _t->onPreviewPlanClicked(); break;
        case 7: _t->onApplyProposalClicked(); break;
        case 8: _t->onDiscardProposalClicked(); break;
        case 9: _t->onAnalyzeClicked(); break;
        case 10: _t->onCleanClicked(); break;
        case 11: _t->onDimensionAllClicked(); break;
        case 12: _t->onDimensionSelectedClicked(); break;
        case 13: _t->onRagSearchClicked(); break;
        case 14: _t->onQuickPromptClicked(); break;
        case 15: _t->onQuickRagTopicClicked(); break;
        case 16: _t->onProviderChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 17: _t->onClarificationOptionClicked(); break;
        default: ;
        }
    }
}

const QMetaObject *AiAssistantDock::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AiAssistantDock::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN15AiAssistantDockE.stringdata0))
        return static_cast<void*>(this);
    return QDockWidget::qt_metacast(_clname);
}

int AiAssistantDock::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDockWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 18;
    }
    return _id;
}
QT_WARNING_POP
