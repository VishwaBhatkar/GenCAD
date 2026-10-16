#pragma once

#include <QToolBar>
#include <QAction>
#include <QActionGroup>

class ToolBar : public QToolBar {
    Q_OBJECT

public:
    explicit ToolBar(QWidget* parent = nullptr);
    ~ToolBar() override;

    // Action accessors
    QAction* actionNew() const { return m_actionNew; }
    QAction* actionOpen() const { return m_actionOpen; }
    QAction* actionSave() const { return m_actionSave; }
    QAction* actionExport() const { return m_actionExport; }

    QAction* actionUndo() const { return m_actionUndo; }
    QAction* actionRedo() const { return m_actionRedo; }

    // Draw tools
    QAction* actionSelect() const { return m_actionSelect; }
    QAction* actionLine() const { return m_actionLine; }
    QAction* actionCircle() const { return m_actionCircle; }
    QAction* actionRectangle() const { return m_actionRectangle; }
    QAction* actionArc() const { return m_actionArc; }
    QAction* actionPolyline() const { return m_actionPolyline; }
    QAction* actionPolygon() const { return m_actionPolygon; }
    QAction* actionEllipse() const { return m_actionEllipse; }
    QAction* actionText() const { return m_actionText; }
    QAction* actionDimension() const { return m_actionDimension; }

    // Modify tools
    QAction* actionMove() const { return m_actionMove; }
    QAction* actionRotate() const { return m_actionRotate; }
    QAction* actionMirror() const { return m_actionMirror; }
    QAction* actionCopy() const { return m_actionCopy; }
    QAction* actionOffset() const { return m_actionOffset; }
    QAction* actionTrim() const { return m_actionTrim; }
    QAction* actionExtend() const { return m_actionExtend; }
    QAction* actionDelete() const { return m_actionDelete; }

    // Navigation
    QAction* actionZoomIn() const { return m_actionZoomIn; }
    QAction* actionZoomOut() const { return m_actionZoomOut; }
    QAction* actionZoomFit() const { return m_actionZoomFit; }

    // Cloud actions
    QAction* actionCloudWorkspace() const { return m_actionCloudWorkspace; }
    QAction* actionCloudUpload() const { return m_actionCloudUpload; }
    QAction* actionCloudDownload() const { return m_actionCloudDownload; }

    // Drafting mode actions
    QAction* actionGrid() const { return m_actionGrid; }
    QAction* actionSnap() const { return m_actionSnap; }
    QAction* actionOrtho() const { return m_actionOrtho; }
    QAction* actionPolar() const { return m_actionPolar; }

    // AI actions
    QAction* actionAiAssistant() const { return m_actionAiAssistant; }

private:
    void createActions();

    QAction* m_actionGrid{nullptr};
    QAction* m_actionSnap{nullptr};
    QAction* m_actionOrtho{nullptr};
    QAction* m_actionPolar{nullptr};

    QAction* m_actionAiAssistant{nullptr};
    QAction* m_actionCloudWorkspace{nullptr};
    QAction* m_actionCloudUpload{nullptr};
    QAction* m_actionCloudDownload{nullptr};

    QAction* m_actionNew{nullptr};
    QAction* m_actionOpen{nullptr};
    QAction* m_actionSave{nullptr};
    QAction* m_actionExport{nullptr};

    QAction* m_actionUndo{nullptr};
    QAction* m_actionRedo{nullptr};

    // Draw
    QAction* m_actionSelect{nullptr};
    QAction* m_actionLine{nullptr};
    QAction* m_actionCircle{nullptr};
    QAction* m_actionRectangle{nullptr};
    QAction* m_actionArc{nullptr};
    QAction* m_actionPolyline{nullptr};
    QAction* m_actionPolygon{nullptr};
    QAction* m_actionEllipse{nullptr};
    QAction* m_actionText{nullptr};
    QAction* m_actionDimension{nullptr};

    // Modify
    QAction* m_actionMove{nullptr};
    QAction* m_actionRotate{nullptr};
    QAction* m_actionMirror{nullptr};
    QAction* m_actionCopy{nullptr};
    QAction* m_actionOffset{nullptr};
    QAction* m_actionTrim{nullptr};
    QAction* m_actionExtend{nullptr};
    QAction* m_actionDelete{nullptr};

    // Zoom
    QAction* m_actionZoomIn{nullptr};
    QAction* m_actionZoomOut{nullptr};
    QAction* m_actionZoomFit{nullptr};

    QActionGroup* m_toolGroup{nullptr};
};
