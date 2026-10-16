/****************************************************************************
** Meta object code from reading C++ file 'CloudWorkspaceDialog.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/CloudWorkspaceDialog.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CloudWorkspaceDialog.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN20CloudWorkspaceDialogE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN20CloudWorkspaceDialogE = QtMocHelpers::stringData(
    "CloudWorkspaceDialog",
    "documentLoaded",
    "",
    "drawingName",
    "onLoginClicked",
    "onRegisterClicked",
    "onLogoutClicked",
    "onRefreshProjects",
    "onProjectSelectionChanged",
    "onCreateProject",
    "onDeleteProject",
    "onRefreshDrawings",
    "onOpenDrawing",
    "onSaveCurrentToCloud",
    "onUpdateSelectedDrawing",
    "onDeleteDrawing",
    "onVersionHistory",
    "onDownloadDrawing",
    "updateAuthUi"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN20CloudWorkspaceDialogE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  110,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       4,    0,  113,    2, 0x08,    3 /* Private */,
       5,    0,  114,    2, 0x08,    4 /* Private */,
       6,    0,  115,    2, 0x08,    5 /* Private */,
       7,    0,  116,    2, 0x08,    6 /* Private */,
       8,    0,  117,    2, 0x08,    7 /* Private */,
       9,    0,  118,    2, 0x08,    8 /* Private */,
      10,    0,  119,    2, 0x08,    9 /* Private */,
      11,    0,  120,    2, 0x08,   10 /* Private */,
      12,    0,  121,    2, 0x08,   11 /* Private */,
      13,    0,  122,    2, 0x08,   12 /* Private */,
      14,    0,  123,    2, 0x08,   13 /* Private */,
      15,    0,  124,    2, 0x08,   14 /* Private */,
      16,    0,  125,    2, 0x08,   15 /* Private */,
      17,    0,  126,    2, 0x08,   16 /* Private */,
      18,    0,  127,    2, 0x08,   17 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,

 // slots: parameters
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

       0        // eod
};

Q_CONSTINIT const QMetaObject CloudWorkspaceDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_ZN20CloudWorkspaceDialogE.offsetsAndSizes,
    qt_meta_data_ZN20CloudWorkspaceDialogE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN20CloudWorkspaceDialogE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<CloudWorkspaceDialog, std::true_type>,
        // method 'documentLoaded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onLoginClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRegisterClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onLogoutClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRefreshProjects'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onProjectSelectionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCreateProject'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDeleteProject'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRefreshDrawings'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onOpenDrawing'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSaveCurrentToCloud'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onUpdateSelectedDrawing'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDeleteDrawing'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onVersionHistory'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDownloadDrawing'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateAuthUi'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void CloudWorkspaceDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CloudWorkspaceDialog *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->documentLoaded((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->onLoginClicked(); break;
        case 2: _t->onRegisterClicked(); break;
        case 3: _t->onLogoutClicked(); break;
        case 4: _t->onRefreshProjects(); break;
        case 5: _t->onProjectSelectionChanged(); break;
        case 6: _t->onCreateProject(); break;
        case 7: _t->onDeleteProject(); break;
        case 8: _t->onRefreshDrawings(); break;
        case 9: _t->onOpenDrawing(); break;
        case 10: _t->onSaveCurrentToCloud(); break;
        case 11: _t->onUpdateSelectedDrawing(); break;
        case 12: _t->onDeleteDrawing(); break;
        case 13: _t->onVersionHistory(); break;
        case 14: _t->onDownloadDrawing(); break;
        case 15: _t->updateAuthUi(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (CloudWorkspaceDialog::*)(const QString & );
            if (_q_method_type _q_method = &CloudWorkspaceDialog::documentLoaded; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *CloudWorkspaceDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CloudWorkspaceDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN20CloudWorkspaceDialogE.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int CloudWorkspaceDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void CloudWorkspaceDialog::documentLoaded(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
