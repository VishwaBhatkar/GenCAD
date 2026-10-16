#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDockWidget>
#include <memory>
#include "ui/CadTheme.h"

class CadDocument;
class CadScene;
class CadView;
class ToolBar;
class CadRibbon;
class Tool;
class SelectTool;
class LineTool;
class CircleTool;
class RectangleTool;
class ArcTool;
class PolylineTool;
class PolygonTool;
class EllipseTool;
class TextTool;
class DimensionTool;
class MoveTool;
class RotateTool;
class MirrorTool;
class CopyTool;
class OffsetTool;
class TrimTool;
class ExtendTool;
class LayerWidget;
class SnapManager;
class CloudClient;
class AiAssistantDock;
class CadCommandLine;
class PropertiesDock;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    CadDocument* document() const { return m_document.get(); }
    CadView* view() const { return m_view; }
    SnapManager* snapManager() const { return m_snapManager.get(); }
    AiAssistantDock* aiDock() const { return m_aiDock; }
    CadRibbon* ribbon() const { return m_ribbon; }
    CadTheme::ThemeMode currentTheme() const { return m_currentTheme; }

public slots:
    void newDrawing();
    void openDrawing();
    bool saveDrawing();
    bool saveDrawingAs();
    void exportDrawing();
    void exportDxf();
    void importDxf();

    void deleteSelected();
    void selectAll();
    void clearSelection();
    void zoomFit();
    void showAbout();

    void toggleSnap();
    void toggleGridSnap();
    void toggleOrtho();
    void togglePolar();
    void toggleTheme();
    void showPageSetup();
    void executeCadCommand(const QString& command);
    void onExactDimensionsCommitted(double v1, double v2, double v3);

    // Cloud Workspace
    void openCloudWorkspace();
    void saveToCloud();
    void openFromCloud();

    // AI Assistant (Phase 4)
    void openAiAssistant();
    void aiGenerateFromPrompt();
    void aiAutoDimension();
    void aiAnalyzeDrawing();
    void aiCleanDrawing();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onCoordinatesChanged(const QPointF& worldPos);
    void onSelectionChanged();
    void onModifiedChanged(bool modified);
    void onFilePathChanged(const QString& path);
    void updateStatusBarStatus();
    void updateCloudStatusBar();
    void onUnitsComboChanged(int index);

private:
    void setupUi();
    void setupQuickAccessBar();
    void setupMenus();
    void setupStatusBar();
    void setupTools();
    void setupLayersDock();
    void setupPropertiesDock();
    void setupAiDock();
    void setupCommandLine();
    void connectActions();
    void applyTheme(CadTheme::ThemeMode theme);
    bool maybeSave();
    void updateTitle();
    void selectTool(Tool* tool);

    std::unique_ptr<CadDocument> m_document;
    std::unique_ptr<SnapManager> m_snapManager;
    std::unique_ptr<CloudClient> m_cloudClient;
    CadScene* m_scene{nullptr};
    CadView* m_view{nullptr};
    ToolBar* m_toolBar{nullptr};
    CadRibbon* m_ribbon{nullptr};

    // Draw Tools
    std::unique_ptr<SelectTool> m_selectTool;
    std::unique_ptr<LineTool> m_lineTool;
    std::unique_ptr<CircleTool> m_circleTool;
    std::unique_ptr<RectangleTool> m_rectangleTool;
    std::unique_ptr<ArcTool> m_arcTool;
    std::unique_ptr<PolylineTool> m_polylineTool;
    std::unique_ptr<PolygonTool> m_polygonTool;
    std::unique_ptr<EllipseTool> m_ellipseTool;
    std::unique_ptr<TextTool> m_textTool;
    std::unique_ptr<DimensionTool> m_dimensionTool;

    // Modify Tools
    std::unique_ptr<MoveTool> m_moveTool;
    std::unique_ptr<RotateTool> m_rotateTool;
    std::unique_ptr<MirrorTool> m_mirrorTool;
    std::unique_ptr<CopyTool> m_copyTool;
    std::unique_ptr<OffsetTool> m_offsetTool;
    std::unique_ptr<TrimTool> m_trimTool;
    std::unique_ptr<ExtendTool> m_extendTool;

    // Docks and command line
    PropertiesDock* m_propertiesDock{nullptr};
    CadCommandLine* m_commandLine{nullptr};

    // Layer dock
    QDockWidget* m_layerDock{nullptr};
    LayerWidget* m_layerWidget{nullptr};

    // AI dock (Phase 4)
    AiAssistantDock* m_aiDock{nullptr};

    // Status bar interactive widgets
    QLabel* m_coordLabel{nullptr};
    QLabel* m_toolLabel{nullptr};
    QLabel* m_selectionLabel{nullptr};
    QLabel* m_layerStatusLabel{nullptr};

    QPushButton* m_btnModel{nullptr};
    QPushButton* m_btnGrid{nullptr};
    QPushButton* m_btnSnap{nullptr};
    QPushButton* m_btnOrtho{nullptr};
    QPushButton* m_btnPolar{nullptr};
    QComboBox* m_unitsCombo{nullptr};
    QPushButton* m_btnTheme{nullptr};
    QLabel* m_cloudStatusLabel{nullptr};

    // Theme state
    CadTheme::ThemeMode m_currentTheme{CadTheme::ThemeMode::Dark};

    // Export Action
    QAction* m_actExportDrawing{nullptr};

    // Cloud Actions
    QAction* m_actCloudWorkspace{nullptr};
    QAction* m_actCloudSave{nullptr};
    QAction* m_actCloudOpen{nullptr};

    // AI Actions
    QAction* m_actAiAssistant{nullptr};
    QAction* m_actAiGenerate{nullptr};
    QAction* m_actAiDimension{nullptr};
    QAction* m_actAiAnalyze{nullptr};
    QAction* m_actAiClean{nullptr};

    // Theme action
    QAction* m_actThemeDark{nullptr};
    QAction* m_actThemeLight{nullptr};
};
