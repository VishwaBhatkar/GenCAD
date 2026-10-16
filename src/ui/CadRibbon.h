#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QToolButton>
#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QLabel>
#include <memory>

class CadDocument;

class CadRibbon : public QWidget {
    Q_OBJECT

public:
    explicit CadRibbon(CadDocument* document, QWidget* parent = nullptr);
    ~CadRibbon() override = default;

    void setDocument(CadDocument* document);
    void updateActiveLayer(const QString& layerName);
    void updateDraftingState(bool snap, bool grid, bool ortho, bool polar);

    // Drawing tool actions
    QAction* actionSelect() const { return m_actSelect; }
    QAction* actionLine() const { return m_actLine; }
    QAction* actionPolyline() const { return m_actPolyline; }
    QAction* actionCircle() const { return m_actCircle; }
    QAction* actionRectangle() const { return m_actRectangle; }
    QAction* actionArc() const { return m_actArc; }
    QAction* actionPolygon() const { return m_actPolygon; }
    QAction* actionEllipse() const { return m_actEllipse; }
    QAction* actionText() const { return m_actText; }
    QAction* actionDimension() const { return m_actDimension; }

    // Modify tool actions
    QAction* actionMove() const { return m_actMove; }
    QAction* actionRotate() const { return m_actRotate; }
    QAction* actionMirror() const { return m_actMirror; }
    QAction* actionCopy() const { return m_actCopy; }
    QAction* actionOffset() const { return m_actOffset; }
    QAction* actionTrim() const { return m_actTrim; }
    QAction* actionExtend() const { return m_actExtend; }
    QAction* actionDelete() const { return m_actDelete; }

    // File actions
    QAction* actionNew() const { return m_actNew; }
    QAction* actionOpen() const { return m_actOpen; }
    QAction* actionSave() const { return m_actSave; }
    QAction* actionSaveAs() const { return m_actSaveAs; }
    QAction* actionExport() const { return m_actExport; }
    QAction* actionPageSetup() const { return m_actPageSetup; }

    // Clipboard / History
    QAction* actionUndo() const { return m_actUndo; }
    QAction* actionRedo() const { return m_actRedo; }

    // View Navigation
    QAction* actionZoomIn() const { return m_actZoomIn; }
    QAction* actionZoomOut() const { return m_actZoomOut; }
    QAction* actionZoomFit() const { return m_actZoomFit; }
    QAction* actionResetZoom() const { return m_actResetZoom; }

    // Drafting mode actions
    QAction* actionGrid() const { return m_actGrid; }
    QAction* actionSnap() const { return m_actSnap; }
    QAction* actionOrtho() const { return m_actOrtho; }
    QAction* actionPolar() const { return m_actPolar; }

    // Cloud actions
    QAction* actionCloudWorkspace() const { return m_actCloudWorkspace; }
    QAction* actionCloudUpload() const { return m_actCloudUpload; }
    QAction* actionCloudDownload() const { return m_actCloudDownload; }

    // AI actions
    QAction* actionAiAssistant() const { return m_actAiAssistant; }
    QAction* actionAiGenerate() const { return m_actAiGenerate; }
    QAction* actionAiDimension() const { return m_actAiDimension; }
    QAction* actionAiAnalyze() const { return m_actAiAnalyze; }
    QAction* actionAiClean() const { return m_actAiClean; }

    // Theme Toggle
    QAction* actionToggleTheme() const { return m_actToggleTheme; }

    // Layer selection combo
    QComboBox* layerComboBox() const { return m_layerCombo; }

signals:
    void activeLayerChanged(const QString& name);
    void newLayerRequested();
    void layerPropertiesRequested();
    void propertiesDockRequested();
    void commandLineRequested();

private slots:
    void onLayerComboChanged(int index);
    void toggleRibbonCollapse();

private:
    void createActions();
    void buildQuickAccessBar();
    void buildRibbonTabs();
    QWidget* createHomeTab();
    QWidget* createAnnotateTab();
    QWidget* createViewTab();
    QWidget* createCloudTab();
    QWidget* createAiTab();

    QWidget* createRibbonGroup(const QString& title, QWidget* content);
    QToolButton* createLargeRibbonButton(QAction* action);
    QToolButton* createSmallRibbonButton(QAction* action);

    CadDocument* m_document{nullptr};
    QTabWidget* m_tabWidget{nullptr};
    QComboBox* m_layerCombo{nullptr};
    QActionGroup* m_drawModifyGroup{nullptr};
    bool m_isCollapsed{false};
    int m_expandedHeight{130};

    // Actions
    QAction* m_actNew{nullptr};
    QAction* m_actOpen{nullptr};
    QAction* m_actSave{nullptr};
    QAction* m_actSaveAs{nullptr};
    QAction* m_actExport{nullptr};
    QAction* m_actPageSetup{nullptr};
    QAction* m_actUndo{nullptr};
    QAction* m_actRedo{nullptr};

    QAction* m_actSelect{nullptr};
    QAction* m_actLine{nullptr};
    QAction* m_actPolyline{nullptr};
    QAction* m_actCircle{nullptr};
    QAction* m_actRectangle{nullptr};
    QAction* m_actArc{nullptr};
    QAction* m_actPolygon{nullptr};
    QAction* m_actEllipse{nullptr};
    QAction* m_actText{nullptr};
    QAction* m_actDimension{nullptr};

    QAction* m_actMove{nullptr};
    QAction* m_actRotate{nullptr};
    QAction* m_actMirror{nullptr};
    QAction* m_actCopy{nullptr};
    QAction* m_actOffset{nullptr};
    QAction* m_actTrim{nullptr};
    QAction* m_actExtend{nullptr};
    QAction* m_actDelete{nullptr};

    QAction* m_actZoomIn{nullptr};
    QAction* m_actZoomOut{nullptr};
    QAction* m_actZoomFit{nullptr};
    QAction* m_actResetZoom{nullptr};

    QAction* m_actGrid{nullptr};
    QAction* m_actSnap{nullptr};
    QAction* m_actOrtho{nullptr};
    QAction* m_actPolar{nullptr};

    QAction* m_actCloudWorkspace{nullptr};
    QAction* m_actCloudUpload{nullptr};
    QAction* m_actCloudDownload{nullptr};

    QAction* m_actAiAssistant{nullptr};
    QAction* m_actAiGenerate{nullptr};
    QAction* m_actAiDimension{nullptr};
    QAction* m_actAiAnalyze{nullptr};
    QAction* m_actAiClean{nullptr};

    QAction* m_actToggleTheme{nullptr};
};
