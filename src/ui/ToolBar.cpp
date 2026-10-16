#include "ui/ToolBar.h"
#include <QIcon>

ToolBar::ToolBar(QWidget* parent)
    : QToolBar(QStringLiteral("GenCAD Tools"), parent)
{
    setIconSize(QSize(22, 22));
    setMovable(false);
    createActions();
}

ToolBar::~ToolBar() = default;

void ToolBar::createActions() {
    // File group
    m_actionNew = addAction(QIcon(QStringLiteral(":/icons/new.svg")), QStringLiteral("New"));
    m_actionNew->setToolTip(QStringLiteral("New Drawing (Ctrl+N)"));

    m_actionOpen = addAction(QIcon(QStringLiteral(":/icons/open.svg")), QStringLiteral("Open"));
    m_actionOpen->setToolTip(QStringLiteral("Open Drawing (Ctrl+O)"));

    m_actionSave = addAction(QIcon(QStringLiteral(":/icons/save.svg")), QStringLiteral("Save"));
    m_actionSave->setToolTip(QStringLiteral("Save Drawing (Ctrl+S)"));

    m_actionExport = addAction(QIcon(QStringLiteral(":/icons/export.svg")), QStringLiteral("Export"));
    m_actionExport->setToolTip(QStringLiteral("Export Drawing (Ctrl+E)"));

    addSeparator();

    // Undo / Redo
    m_actionUndo = addAction(QIcon(QStringLiteral(":/icons/undo.svg")), QStringLiteral("Undo"));
    m_actionUndo->setToolTip(QStringLiteral("Undo (Ctrl+Z)"));

    m_actionRedo = addAction(QIcon(QStringLiteral(":/icons/redo.svg")), QStringLiteral("Redo"));
    m_actionRedo->setToolTip(QStringLiteral("Redo (Ctrl+Y)"));

    addSeparator();

    // Tools Group (Exclusive)
    m_toolGroup = new QActionGroup(this);
    m_toolGroup->setExclusive(true);

    m_actionSelect = addAction(QIcon(QStringLiteral(":/icons/select.svg")), QStringLiteral("Select"));
    m_actionSelect->setToolTip(QStringLiteral("Select Entities (Esc)"));
    m_actionSelect->setCheckable(true);
    m_actionSelect->setChecked(true);
    m_toolGroup->addAction(m_actionSelect);

    // Draw Primitives
    m_actionLine = addAction(QIcon(QStringLiteral(":/icons/line.svg")), QStringLiteral("Line"));
    m_actionLine->setToolTip(QStringLiteral("Draw Line"));
    m_actionLine->setCheckable(true);
    m_toolGroup->addAction(m_actionLine);

    m_actionCircle = addAction(QIcon(QStringLiteral(":/icons/circle.svg")), QStringLiteral("Circle"));
    m_actionCircle->setToolTip(QStringLiteral("Draw Circle"));
    m_actionCircle->setCheckable(true);
    m_toolGroup->addAction(m_actionCircle);

    m_actionRectangle = addAction(QIcon(QStringLiteral(":/icons/rectangle.svg")), QStringLiteral("Rectangle"));
    m_actionRectangle->setToolTip(QStringLiteral("Draw Rectangle"));
    m_actionRectangle->setCheckable(true);
    m_toolGroup->addAction(m_actionRectangle);

    m_actionArc = addAction(QIcon(QStringLiteral(":/icons/arc.svg")), QStringLiteral("Arc"));
    m_actionArc->setToolTip(QStringLiteral("Draw Arc"));
    m_actionArc->setCheckable(true);
    m_toolGroup->addAction(m_actionArc);

    m_actionPolyline = addAction(QIcon(QStringLiteral(":/icons/polyline.svg")), QStringLiteral("Polyline"));
    m_actionPolyline->setToolTip(QStringLiteral("Draw Polyline (Enter to finish, C to close)"));
    m_actionPolyline->setCheckable(true);
    m_toolGroup->addAction(m_actionPolyline);

    m_actionPolygon = addAction(QIcon(QStringLiteral(":/icons/polygon.svg")), QStringLiteral("Polygon"));
    m_actionPolygon->setToolTip(QStringLiteral("Draw Regular Polygon (Sides, Radius, Rotation)"));
    m_actionPolygon->setCheckable(true);
    m_toolGroup->addAction(m_actionPolygon);

    m_actionEllipse = addAction(QIcon(QStringLiteral(":/icons/ellipse.svg")), QStringLiteral("Ellipse"));
    m_actionEllipse->setToolTip(QStringLiteral("Draw Ellipse (Center, Major Axis, Minor Axis)"));
    m_actionEllipse->setCheckable(true);
    m_toolGroup->addAction(m_actionEllipse);

    m_actionText = addAction(QIcon(QStringLiteral(":/icons/text.svg")), QStringLiteral("Text"));
    m_actionText->setToolTip(QStringLiteral("Place Text"));
    m_actionText->setCheckable(true);
    m_toolGroup->addAction(m_actionText);

    m_actionDimension = addAction(QIcon(QStringLiteral(":/icons/dimension.svg")), QStringLiteral("Dimension"));
    m_actionDimension->setToolTip(QStringLiteral("Add Linear Dimension"));
    m_actionDimension->setCheckable(true);
    m_toolGroup->addAction(m_actionDimension);

    addSeparator();

    // Modify Operations
    m_actionMove = addAction(QIcon(QStringLiteral(":/icons/move.svg")), QStringLiteral("Move"));
    m_actionMove->setToolTip(QStringLiteral("Move Entities"));
    m_actionMove->setCheckable(true);
    m_toolGroup->addAction(m_actionMove);

    m_actionRotate = addAction(QIcon(QStringLiteral(":/icons/rotate.svg")), QStringLiteral("Rotate"));
    m_actionRotate->setToolTip(QStringLiteral("Rotate Entities"));
    m_actionRotate->setCheckable(true);
    m_toolGroup->addAction(m_actionRotate);

    m_actionMirror = addAction(QIcon(QStringLiteral(":/icons/mirror.svg")), QStringLiteral("Mirror"));
    m_actionMirror->setToolTip(QStringLiteral("Mirror Entities"));
    m_actionMirror->setCheckable(true);
    m_toolGroup->addAction(m_actionMirror);

    m_actionCopy = addAction(QIcon(QStringLiteral(":/icons/copy.svg")), QStringLiteral("Copy"));
    m_actionCopy->setToolTip(QStringLiteral("Copy Entities"));
    m_actionCopy->setCheckable(true);
    m_toolGroup->addAction(m_actionCopy);

    m_actionOffset = addAction(QIcon(QStringLiteral(":/icons/offset.svg")), QStringLiteral("Offset"));
    m_actionOffset->setToolTip(QStringLiteral("Offset Entity"));
    m_actionOffset->setCheckable(true);
    m_toolGroup->addAction(m_actionOffset);

    m_actionTrim = addAction(QIcon(QStringLiteral(":/icons/trim.svg")), QStringLiteral("Trim"));
    m_actionTrim->setToolTip(QStringLiteral("Trim Entity"));
    m_actionTrim->setCheckable(true);
    m_toolGroup->addAction(m_actionTrim);

    m_actionExtend = addAction(QIcon(QStringLiteral(":/icons/extend.svg")), QStringLiteral("Extend"));
    m_actionExtend->setToolTip(QStringLiteral("Extend Entity"));
    m_actionExtend->setCheckable(true);
    m_toolGroup->addAction(m_actionExtend);

    m_actionDelete = addAction(QIcon(QStringLiteral(":/icons/delete.svg")), QStringLiteral("Delete"));
    m_actionDelete->setToolTip(QStringLiteral("Delete Selected Entities (Delete)"));

    addSeparator();

    // View Navigation
    m_actionZoomIn = addAction(QIcon(QStringLiteral(":/icons/zoom_in.svg")), QStringLiteral("Zoom In"));
    m_actionZoomIn->setToolTip(QStringLiteral("Zoom In (Ctrl++)"));

    m_actionZoomOut = addAction(QIcon(QStringLiteral(":/icons/zoom_out.svg")), QStringLiteral("Zoom Out"));
    m_actionZoomOut->setToolTip(QStringLiteral("Zoom Out (Ctrl+-)"));

    m_actionZoomFit = addAction(QIcon(QStringLiteral(":/icons/zoom_fit.svg")), QStringLiteral("Fit Drawing"));
    m_actionZoomFit->setToolTip(QStringLiteral("Zoom to Fit Drawing (Ctrl+F)"));

    addSeparator();

    // Drafting Modes: [ GRID ] [ SNAP ] [ ORTHO ] [ POLAR ]
    m_actionGrid = addAction(QStringLiteral("GRID"));
    m_actionGrid->setToolTip(QStringLiteral("Toggle Grid Snap (F9)"));
    m_actionGrid->setCheckable(true);

    m_actionSnap = addAction(QStringLiteral("SNAP"));
    m_actionSnap->setToolTip(QStringLiteral("Toggle Object Snap (F3)"));
    m_actionSnap->setCheckable(true);

    m_actionOrtho = addAction(QStringLiteral("ORTHO"));
    m_actionOrtho->setToolTip(QStringLiteral("Toggle ORTHO Mode (F8)"));
    m_actionOrtho->setCheckable(true);

    m_actionPolar = addAction(QStringLiteral("POLAR"));
    m_actionPolar->setToolTip(QStringLiteral("Toggle Polar Tracking (F10)"));
    m_actionPolar->setCheckable(true);

    addSeparator();

    // Cloud Workspace
    m_actionCloudWorkspace = addAction(QIcon(QStringLiteral(":/icons/cloud.svg")), QStringLiteral("Cloud Workspace"));
    m_actionCloudWorkspace->setToolTip(QStringLiteral("Cloud Workspace & Projects (Ctrl+Shift+C)"));

    m_actionCloudUpload = addAction(QIcon(QStringLiteral(":/icons/cloud_upload.svg")), QStringLiteral("Save to Cloud"));
    m_actionCloudUpload->setToolTip(QStringLiteral("Save Drawing to Cloud"));

    m_actionCloudDownload = addAction(QIcon(QStringLiteral(":/icons/cloud_download.svg")), QStringLiteral("Open from Cloud"));
    m_actionCloudDownload->setToolTip(QStringLiteral("Open Drawing from Cloud"));

    addSeparator();

    // AI Assistant
    m_actionAiAssistant = addAction(QIcon(QStringLiteral(":/icons/ai.svg")), QStringLiteral("AI Assistant"));
    m_actionAiAssistant->setToolTip(QStringLiteral("AI Assistant Panel (Ctrl+Shift+A)"));
}
