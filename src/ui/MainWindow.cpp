#include "ui/MainWindow.h"
#include "ui/ToolBar.h"
#include "ui/CadRibbon.h"
#include "ui/LayerWidget.h"
#include "canvas/CadScene.h"
#include "canvas/CadView.h"
#include "document/CadDocument.h"
#include "document/CoordinateSystem.h"
#include "commands/DeleteEntityCommand.h"
#include "snap/SnapManager.h"
#include "layers/LayerManager.h"
#include "io/GencadFile.h"
#include "io/DxfFile.h"
#include "io/DrawingExporter.h"
#include "cloud/CloudClient.h"
#include "ui/CloudWorkspaceDialog.h"
#include "ui/AiAssistantDock.h"

#include "tools/SelectTool.h"
#include "tools/LineTool.h"
#include "tools/CircleTool.h"
#include "tools/RectangleTool.h"
#include "tools/ArcTool.h"
#include "tools/PolylineTool.h"
#include "tools/PolygonTool.h"
#include "tools/EllipseTool.h"
#include "tools/TextTool.h"
#include "tools/DimensionTool.h"
#include "tools/MoveTool.h"
#include "tools/RotateTool.h"
#include "tools/MirrorTool.h"
#include "tools/CopyTool.h"
#include "tools/OffsetTool.h"
#include "tools/TrimTool.h"
#include "tools/ExtendTool.h"

#include "ui/CadCommandLine.h"
#include "ui/PropertiesDock.h"
#include "ui/PageSetupDialog.h"

#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QKeySequence>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTimer>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_document(std::make_unique<CadDocument>())
    , m_snapManager(std::make_unique<SnapManager>(this))
    , m_cloudClient(std::make_unique<CloudClient>(this))
{
    setupUi();
    setupLayersDock();
    setupPropertiesDock();
    setupAiDock();
    setupTools();
    setupMenus();
    setupStatusBar();
    connectActions();

    connect(m_cloudClient.get(), &CloudClient::authStateChanged, this, &MainWindow::updateCloudStatusBar);

    applyTheme(CadTheme::ThemeMode::Dark);

    updateTitle();
    resize(1366, 860);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    auto* centralContainer = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(centralContainer);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    // 1. AutoCAD Ribbon Bar at the top of the canvas
    m_ribbon = new CadRibbon(m_document.get(), centralContainer);
    centralLayout->addWidget(m_ribbon, 0);

    // 2. Center Splitter: Drafting Canvas View + Dockable Command Line
    auto* splitter = new QSplitter(Qt::Vertical, centralContainer);
    splitter->setChildrenCollapsible(false);

    m_scene = new CadScene(m_document.get(), this);
    m_view = new CadView(m_scene, this);
    m_view->setSnapManager(m_snapManager.get());
    splitter->addWidget(m_view);

    m_commandLine = new CadCommandLine(splitter);
    splitter->addWidget(m_commandLine);

    splitter->setStretchFactor(0, 8);
    splitter->setStretchFactor(1, 1);

    centralLayout->addWidget(splitter, 1);
    setCentralWidget(centralContainer);

    // 3. Keep ToolBar object instantiated for compatibility and test lookup
    m_toolBar = new ToolBar(this);
    m_toolBar->setObjectName(QStringLiteral("GenCadToolBar"));
    m_toolBar->setVisible(false); // Ribbon is the primary AutoCAD modern interface
    addToolBar(Qt::TopToolBarArea, m_toolBar);
}

void MainWindow::applyTheme(CadTheme::ThemeMode theme) {
    m_currentTheme = theme;
    setStyleSheet(CadTheme::getStyleSheet(theme));
    if (m_view) {
        m_view->setThemeMode(theme);
    }
    if (m_btnTheme) {
        if (theme == CadTheme::ThemeMode::Dark) {
            m_btnTheme->setIcon(QIcon(QStringLiteral(":/icons/theme_light.svg")));
            m_btnTheme->setToolTip(tr("Switch to Light Engineering Theme"));
        } else {
            m_btnTheme->setIcon(QIcon(QStringLiteral(":/icons/theme_dark.svg")));
            m_btnTheme->setToolTip(tr("Switch to Dark AutoCAD Theme"));
        }
    }
}

void MainWindow::toggleTheme() {
    if (m_currentTheme == CadTheme::ThemeMode::Dark) {
        applyTheme(CadTheme::ThemeMode::Light);
    } else {
        applyTheme(CadTheme::ThemeMode::Dark);
    }
}

void MainWindow::setupLayersDock() {
    m_layerDock = new QDockWidget(tr("Layers Manager"), this);
    m_layerDock->setObjectName(QStringLiteral("LayersDockWidget"));
    m_layerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_layerWidget = new LayerWidget(m_document.get(), m_layerDock);
    m_layerDock->setWidget(m_layerWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_layerDock);
}

void MainWindow::setupPropertiesDock() {
    m_propertiesDock = new PropertiesDock(m_document.get(), this);
    m_propertiesDock->setObjectName(QStringLiteral("PropertiesDockWidget"));
    m_propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_propertiesDock);
}

void MainWindow::setupCommandLine() {
}

void MainWindow::setupAiDock() {
    m_aiDock = new AiAssistantDock(m_document.get(), this, this);
    m_aiDock->setObjectName(QStringLiteral("AiAssistantDockWidget"));
    addDockWidget(Qt::RightDockWidgetArea, m_aiDock);
    tabifyDockWidget(m_layerDock, m_aiDock);
    m_layerDock->raise();
}

void MainWindow::setupTools() {
    m_selectTool = std::make_unique<SelectTool>(m_document.get(), m_view);
    m_lineTool = std::make_unique<LineTool>(m_document.get(), m_view, m_commandLine);
    m_circleTool = std::make_unique<CircleTool>(m_document.get(), m_view, m_commandLine);
    m_rectangleTool = std::make_unique<RectangleTool>(m_document.get(), m_view, m_commandLine);
    m_arcTool = std::make_unique<ArcTool>(m_document.get(), m_view, m_commandLine);
    m_polylineTool = std::make_unique<PolylineTool>(m_document.get(), m_view, m_commandLine);
    m_polygonTool = std::make_unique<PolygonTool>(m_document.get(), m_view, m_commandLine);
    m_ellipseTool = std::make_unique<EllipseTool>(m_document.get(), m_view, m_commandLine);
    m_textTool = std::make_unique<TextTool>(m_document.get(), m_view);
    m_dimensionTool = std::make_unique<DimensionTool>(m_document.get(), m_view, m_commandLine);

    m_moveTool = std::make_unique<MoveTool>(m_document.get(), m_view);
    m_rotateTool = std::make_unique<RotateTool>(m_document.get(), m_view);
    m_mirrorTool = std::make_unique<MirrorTool>(m_document.get(), m_view);
    m_copyTool = std::make_unique<CopyTool>(m_document.get(), m_view);
    m_offsetTool = std::make_unique<OffsetTool>(m_document.get(), m_view);
    m_trimTool = std::make_unique<TrimTool>(m_document.get(), m_view);
    m_extendTool = std::make_unique<ExtendTool>(m_document.get(), m_view);

    selectTool(m_selectTool.get());
}

void MainWindow::setupMenus() {
    auto addMenuAction = [this](QMenu* menu, const QIcon& icon, const QString& text, const QKeySequence& shortcut, auto slot) -> QAction* {
        auto* act = new QAction(icon, text, this);
        if (!shortcut.isEmpty()) act->setShortcut(shortcut);
        connect(act, &QAction::triggered, this, slot);
        menu->addAction(act);
        return act;
    };

    // 1. File Menu
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    addMenuAction(fileMenu, QIcon(QStringLiteral(":/icons/new.svg")), tr("&New Drawing"), QKeySequence::New, &MainWindow::newDrawing);
    addMenuAction(fileMenu, QIcon(QStringLiteral(":/icons/settings.svg")), tr("&Page Setup / Limits..."), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P), &MainWindow::showPageSetup);
    addMenuAction(fileMenu, QIcon(QStringLiteral(":/icons/open.svg")), tr("&Open..."), QKeySequence::Open, &MainWindow::openDrawing);
    addMenuAction(fileMenu, QIcon(QStringLiteral(":/icons/save.svg")), tr("&Save"), QKeySequence::Save, &MainWindow::saveDrawing);
    addMenuAction(fileMenu, QIcon(QStringLiteral(":/icons/save_as.svg")), tr("Save &As..."), QKeySequence::SaveAs, &MainWindow::saveDrawingAs);
    fileMenu->addSeparator();
    m_actExportDrawing = addMenuAction(fileMenu, QIcon(QStringLiteral(":/icons/export.svg")), tr("&Export Drawing..."), QKeySequence(Qt::CTRL | Qt::Key_E), &MainWindow::exportDrawing);
    fileMenu->addSeparator();
    m_actCloudWorkspace = addMenuAction(fileMenu, QIcon(QStringLiteral(":/icons/cloud.svg")), tr("&Cloud Workspace..."), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C), &MainWindow::openCloudWorkspace);
    m_actCloudSave = addMenuAction(fileMenu, QIcon(QStringLiteral(":/icons/cloud_upload.svg")), tr("Save to &Cloud..."), QKeySequence(), &MainWindow::saveToCloud);
    m_actCloudOpen = addMenuAction(fileMenu, QIcon(QStringLiteral(":/icons/cloud_download.svg")), tr("Open from C&loud..."), QKeySequence(), &MainWindow::openFromCloud);
    fileMenu->addSeparator();
    addMenuAction(fileMenu, QIcon(), tr("Import &DXF..."), QKeySequence(Qt::CTRL | Qt::Key_I), &MainWindow::importDxf);
    addMenuAction(fileMenu, QIcon(), tr("Export &DXF..."), QKeySequence(), &MainWindow::exportDxf);
    fileMenu->addSeparator();
    auto* actExit = new QAction(tr("E&xit"), this);
    actExit->setShortcut(QKeySequence::Quit);
    connect(actExit, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(actExit);

    // 2. Edit Menu
    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
    QAction* actUndo = m_document->undoStack()->createUndoAction(this, tr("&Undo"));
    actUndo->setShortcut(QKeySequence::Undo);
    actUndo->setIcon(QIcon(QStringLiteral(":/icons/undo.svg")));
    editMenu->addAction(actUndo);

    QAction* actRedo = m_document->undoStack()->createRedoAction(this, tr("&Redo"));
    actRedo->setShortcut(QKeySequence::Redo);
    actRedo->setIcon(QIcon(QStringLiteral(":/icons/redo.svg")));
    editMenu->addAction(actRedo);

    editMenu->addSeparator();
    addMenuAction(editMenu, QIcon(QStringLiteral(":/icons/delete.svg")), tr("&Delete"), QKeySequence::Delete, &MainWindow::deleteSelected);
    addMenuAction(editMenu, QIcon(), tr("Select &All"), QKeySequence::SelectAll, &MainWindow::selectAll);
    addMenuAction(editMenu, QIcon(), tr("Clear &Selection"), QKeySequence(Qt::Key_Escape), &MainWindow::clearSelection);

    connect(m_ribbon->actionUndo(), &QAction::triggered, actUndo, &QAction::trigger);
    connect(m_ribbon->actionRedo(), &QAction::triggered, actRedo, &QAction::trigger);
    connect(m_document->undoStack(), &QUndoStack::canUndoChanged, m_ribbon->actionUndo(), &QAction::setEnabled);
    connect(m_document->undoStack(), &QUndoStack::canRedoChanged, m_ribbon->actionRedo(), &QAction::setEnabled);
    m_ribbon->actionUndo()->setEnabled(false);
    m_ribbon->actionRedo()->setEnabled(false);

    // 3. View Menu
    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    auto* actZoomIn = new QAction(QIcon(QStringLiteral(":/icons/zoom_in.svg")), tr("Zoom &In"), this);
    actZoomIn->setShortcut(QKeySequence::ZoomIn);
    connect(actZoomIn, &QAction::triggered, m_view, &CadView::zoomIn);
    viewMenu->addAction(actZoomIn);

    auto* actZoomOut = new QAction(QIcon(QStringLiteral(":/icons/zoom_out.svg")), tr("Zoom &Out"), this);
    actZoomOut->setShortcut(QKeySequence::ZoomOut);
    connect(actZoomOut, &QAction::triggered, m_view, &CadView::zoomOut);
    viewMenu->addAction(actZoomOut);

    addMenuAction(viewMenu, QIcon(QStringLiteral(":/icons/zoom_fit.svg")), tr("&Fit Drawing (Extents)"), QKeySequence(Qt::CTRL | Qt::Key_F), &MainWindow::zoomFit);

    auto* actResetZoom = new QAction(tr("&Reset Zoom (1:1)"), this);
    connect(actResetZoom, &QAction::triggered, m_view, &CadView::resetZoom);
    viewMenu->addAction(actResetZoom);

    viewMenu->addSeparator();
    QAction* actGrid = viewMenu->addAction(QIcon(QStringLiteral(":/icons/grid.svg")), tr("Toggle &Grid (F9)"));
    actGrid->setCheckable(true);
    actGrid->setChecked(true);
    connect(actGrid, &QAction::triggered, this, &MainWindow::toggleGridSnap);

    QAction* actOSnap = viewMenu->addAction(QIcon(QStringLiteral(":/icons/snap.svg")), tr("Toggle &Object Snap (F3)"));
    actOSnap->setCheckable(true);
    actOSnap->setChecked(m_snapManager->isSnapEnabled());
    actOSnap->setShortcut(QKeySequence(Qt::Key_F3));
    connect(actOSnap, &QAction::triggered, this, &MainWindow::toggleSnap);

    QAction* actOrtho = viewMenu->addAction(QIcon(QStringLiteral(":/icons/ortho.svg")), tr("Toggle &ORTHO Mode (F8)"));
    actOrtho->setCheckable(true);
    actOrtho->setChecked(m_view->isOrthoEnabled());
    actOrtho->setShortcut(QKeySequence(Qt::Key_F8));
    connect(actOrtho, &QAction::triggered, this, &MainWindow::toggleOrtho);

    QAction* actPolar = viewMenu->addAction(QIcon(QStringLiteral(":/icons/polar.svg")), tr("Toggle &Polar Tracking (F10)"));
    actPolar->setCheckable(true);
    actPolar->setChecked(m_snapManager->isPolarTrackingEnabled());
    actPolar->setShortcut(QKeySequence(Qt::Key_F10));
    connect(actPolar, &QAction::triggered, this, &MainWindow::togglePolar);

    viewMenu->addSeparator();
    viewMenu->addAction(m_propertiesDock->toggleViewAction());
    viewMenu->addAction(m_layerDock->toggleViewAction());
    viewMenu->addAction(m_aiDock->toggleViewAction());

    // 4. Draw Menu
    QMenu* drawMenu = menuBar()->addMenu(tr("&Draw"));
    auto addDrawTool = [this, drawMenu](const QString& name, const QString& iconPath, const QKeySequence& shortcut, Tool* tool, QAction* ribbonAct) {
        auto* act = new QAction(QIcon(iconPath), name, this);
        if (!shortcut.isEmpty()) act->setShortcut(shortcut);
        connect(act, &QAction::triggered, this, [this, tool, ribbonAct]() {
            if (ribbonAct) ribbonAct->setChecked(true);
            selectTool(tool);
        });
        drawMenu->addAction(act);
    };

    addDrawTool(tr("&Line"), QStringLiteral(":/icons/line.svg"), QKeySequence(Qt::Key_L), m_lineTool.get(), m_ribbon->actionLine());
    addDrawTool(tr("&Polyline"), QStringLiteral(":/icons/polyline.svg"), QKeySequence(Qt::Key_P), m_polylineTool.get(), m_ribbon->actionPolyline());
    addDrawTool(tr("&Circle"), QStringLiteral(":/icons/circle.svg"), QKeySequence(Qt::Key_C), m_circleTool.get(), m_ribbon->actionCircle());
    addDrawTool(tr("&Rectangle"), QStringLiteral(":/icons/rectangle.svg"), QKeySequence(Qt::Key_R), m_rectangleTool.get(), m_ribbon->actionRectangle());
    addDrawTool(tr("&Arc"), QStringLiteral(":/icons/arc.svg"), QKeySequence(Qt::Key_A), m_arcTool.get(), m_ribbon->actionArc());
    addDrawTool(tr("P&olygon"), QStringLiteral(":/icons/polygon.svg"), QKeySequence(), m_polygonTool.get(), m_ribbon->actionPolygon());
    addDrawTool(tr("&Ellipse"), QStringLiteral(":/icons/ellipse.svg"), QKeySequence(Qt::Key_E), m_ellipseTool.get(), m_ribbon->actionEllipse());
    addDrawTool(tr("&Text"), QStringLiteral(":/icons/text.svg"), QKeySequence(Qt::Key_T), m_textTool.get(), m_ribbon->actionText());
    addDrawTool(tr("&Dimension"), QStringLiteral(":/icons/dimension.svg"), QKeySequence(Qt::Key_D), m_dimensionTool.get(), m_ribbon->actionDimension());

    // 5. Modify Menu
    QMenu* modifyMenu = menuBar()->addMenu(tr("&Modify"));
    auto addModifyTool = [this, modifyMenu](const QString& name, const QString& iconPath, Tool* tool, QAction* ribbonAct) {
        auto* act = new QAction(QIcon(iconPath), name, this);
        connect(act, &QAction::triggered, this, [this, tool, ribbonAct]() {
            if (ribbonAct) ribbonAct->setChecked(true);
            selectTool(tool);
        });
        modifyMenu->addAction(act);
    };

    addModifyTool(tr("&Move"), QStringLiteral(":/icons/move.svg"), m_moveTool.get(), m_ribbon->actionMove());
    addModifyTool(tr("&Rotate"), QStringLiteral(":/icons/rotate.svg"), m_rotateTool.get(), m_ribbon->actionRotate());
    addModifyTool(tr("&Mirror"), QStringLiteral(":/icons/mirror.svg"), m_mirrorTool.get(), m_ribbon->actionMirror());
    addModifyTool(tr("&Copy"), QStringLiteral(":/icons/copy.svg"), m_copyTool.get(), m_ribbon->actionCopy());
    addModifyTool(tr("&Offset"), QStringLiteral(":/icons/offset.svg"), m_offsetTool.get(), m_ribbon->actionOffset());
    addModifyTool(tr("&Trim"), QStringLiteral(":/icons/trim.svg"), m_trimTool.get(), m_ribbon->actionTrim());
    addModifyTool(tr("&Extend"), QStringLiteral(":/icons/extend.svg"), m_extendTool.get(), m_ribbon->actionExtend());
    modifyMenu->addSeparator();
    addMenuAction(modifyMenu, QIcon(QStringLiteral(":/icons/delete.svg")), tr("&Delete"), QKeySequence::Delete, &MainWindow::deleteSelected);

    // 6. Tools Menu
    QMenu* toolsMenu = menuBar()->addMenu(tr("&Tools"));
    auto* actSelect = new QAction(QIcon(QStringLiteral(":/icons/select.svg")), tr("&Select"), this);
    connect(actSelect, &QAction::triggered, this, [this]() {
        m_ribbon->actionSelect()->setChecked(true);
        selectTool(m_selectTool.get());
    });
    toolsMenu->addAction(actSelect);
    toolsMenu->addSeparator();
    toolsMenu->addAction(tr("Drafting Settings / Limits..."), this, &MainWindow::showPageSetup);

    // 7. Cloud Menu
    QMenu* cloudMenu = menuBar()->addMenu(tr("&Cloud"));
    cloudMenu->addAction(m_actCloudWorkspace);
    cloudMenu->addAction(m_actCloudSave);
    cloudMenu->addAction(m_actCloudOpen);

    // 8. AI Menu (Phase 4)
    m_actAiAssistant = new QAction(QIcon(QStringLiteral(":/icons/ai.svg")), tr("&AI Assistant Panel"), this);
    m_actAiAssistant->setShortcut(QKeySequence(tr("Ctrl+Shift+A")));
    connect(m_actAiAssistant, &QAction::triggered, this, &MainWindow::openAiAssistant);

    m_actAiGenerate = new QAction(tr("&Generate from Prompt..."), this);
    connect(m_actAiGenerate, &QAction::triggered, this, &MainWindow::aiGenerateFromPrompt);

    m_actAiDimension = new QAction(tr("Auto-&Dimension Drawing"), this);
    connect(m_actAiDimension, &QAction::triggered, this, &MainWindow::aiAutoDimension);

    m_actAiAnalyze = new QAction(tr("Analyze &Geometry"), this);
    connect(m_actAiAnalyze, &QAction::triggered, this, &MainWindow::aiAnalyzeDrawing);

    m_actAiClean = new QAction(tr("&Auto-Clean Duplicates"), this);
    connect(m_actAiClean, &QAction::triggered, this, &MainWindow::aiCleanDrawing);

    QMenu* aiMenu = menuBar()->addMenu(tr("&AI"));
    aiMenu->addAction(m_actAiAssistant);
    aiMenu->addSeparator();
    aiMenu->addAction(m_actAiGenerate);
    aiMenu->addAction(m_actAiDimension);
    aiMenu->addAction(m_actAiAnalyze);
    aiMenu->addAction(m_actAiClean);

    // 9. Theme Menu
    QMenu* themeMenu = menuBar()->addMenu(tr("&Theme"));
    m_actThemeDark = themeMenu->addAction(QIcon(QStringLiteral(":/icons/theme_dark.svg")), tr("&Dark Theme (AutoCAD)"));
    connect(m_actThemeDark, &QAction::triggered, this, [this]() { applyTheme(CadTheme::ThemeMode::Dark); });

    m_actThemeLight = themeMenu->addAction(QIcon(QStringLiteral(":/icons/theme_light.svg")), tr("&Light Theme (Engineering)"));
    connect(m_actThemeLight, &QAction::triggered, this, [this]() { applyTheme(CadTheme::ThemeMode::Light); });

    // 10. Help Menu
    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About GenCAD..."), this, &MainWindow::showAbout);
}

void MainWindow::setupStatusBar() {
    QStatusBar* bar = statusBar();

    // 1. Left Readouts: Coordinates, Tool, Selection, Layer
    m_coordLabel = new QLabel(QStringLiteral("X: 0.00  Y: 0.00  Z: 0.00"), this);
    m_coordLabel->setMinimumWidth(180);
    m_coordLabel->setStyleSheet(QStringLiteral("font-family: 'Consolas', monospace; color: #abb2bf; padding: 2px 4px;"));
    bar->addWidget(m_coordLabel);

    m_toolLabel = new QLabel(QStringLiteral("Tool: Select"), this);
    m_toolLabel->setMinimumWidth(110);
    m_toolLabel->setStyleSheet(QStringLiteral("font-weight: 500; color: #00e5ff; padding: 2px 4px;"));
    bar->addWidget(m_toolLabel);

    m_selectionLabel = new QLabel(QStringLiteral("0 selected"), this);
    m_selectionLabel->setMinimumWidth(90);
    m_selectionLabel->setStyleSheet(QStringLiteral("color: #abb2bf; padding: 2px 4px;"));
    bar->addWidget(m_selectionLabel);

    m_layerStatusLabel = new QLabel(QStringLiteral("Layer: 0"), this);
    m_layerStatusLabel->setMinimumWidth(90);
    m_layerStatusLabel->setStyleSheet(QStringLiteral("color: #abb2bf; padding: 2px 4px;"));
    bar->addWidget(m_layerStatusLabel);

    // 2. Right Interactive Toggle Buttons (AutoCAD Status Bar)
    auto createStatusBtn = [this, bar](const QString& text, const QString& iconPath, const QString& tip) -> QPushButton* {
        auto* btn = new QPushButton(QIcon(iconPath), text, this);
        btn->setCheckable(true);
        btn->setToolTip(tip);
        btn->setFixedHeight(20);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { background: transparent; border: 1px solid transparent; border-radius: 2px; padding: 1px 6px; font-weight: 600; font-size: 10px; color: #828997; }"
            "QPushButton:hover { background: #2c313a; border: 1px solid #3e4451; color: #ffffff; }"
            "QPushButton:checked { background: #0e639c; border: 1px solid #0098ff; color: #00e5ff; }"
        ));
        bar->addPermanentWidget(btn);
        return btn;
    };

    m_btnGrid = createStatusBtn(QStringLiteral("GRID"), QStringLiteral(":/icons/grid.svg"), tr("Toggle Grid Snap (F9)"));
    connect(m_btnGrid, &QPushButton::clicked, this, &MainWindow::toggleGridSnap);

    m_btnSnap = createStatusBtn(QStringLiteral("SNAP"), QStringLiteral(":/icons/snap.svg"), tr("Toggle Object Snap (F3)"));
    connect(m_btnSnap, &QPushButton::clicked, this, &MainWindow::toggleSnap);

    m_btnOrtho = createStatusBtn(QStringLiteral("ORTHO"), QStringLiteral(":/icons/ortho.svg"), tr("Toggle ORTHO Mode (F8)"));
    connect(m_btnOrtho, &QPushButton::clicked, this, &MainWindow::toggleOrtho);

    m_btnPolar = createStatusBtn(QStringLiteral("POLAR"), QStringLiteral(":/icons/polar.svg"), tr("Toggle Polar Tracking (F10)"));
    connect(m_btnPolar, &QPushButton::clicked, this, &MainWindow::togglePolar);

    // Units selector dropdown
    m_unitsCombo = new QComboBox(this);
    m_unitsCombo->addItems({QStringLiteral("mm"), QStringLiteral("cm"), QStringLiteral("m"), QStringLiteral("in"), QStringLiteral("ft")});
    m_unitsCombo->setToolTip(tr("Current Engineering Unit System"));
    m_unitsCombo->setFixedHeight(20);
    connect(m_unitsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onUnitsComboChanged);
    bar->addPermanentWidget(m_unitsCombo);

    // Theme toggle button
    m_btnTheme = new QPushButton(QIcon(QStringLiteral(":/icons/theme_light.svg")), QString(), this);
    m_btnTheme->setToolTip(tr("Switch Dark / Light CAD Theme"));
    m_btnTheme->setFixedSize(22, 20);
    m_btnTheme->setStyleSheet(QStringLiteral("QPushButton { background: transparent; border: none; } QPushButton:hover { background: #2c313a; border-radius: 2px; }"));
    connect(m_btnTheme, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    bar->addPermanentWidget(m_btnTheme);

    // Cloud status indicator
    m_cloudStatusLabel = new QLabel(QStringLiteral("Cloud: Offline"), this);
    m_cloudStatusLabel->setMinimumWidth(90);
    m_cloudStatusLabel->setAlignment(Qt::AlignCenter);
    m_cloudStatusLabel->setStyleSheet(QStringLiteral("color: #828997; padding: 2px 4px; font-size: 10px;"));
    bar->addPermanentWidget(m_cloudStatusLabel);

    updateStatusBarStatus();
    updateCloudStatusBar();
}

void MainWindow::onUnitsComboChanged(int index) {
    if (!m_document) return;
    UnitType u = UnitType::Millimeter;
    switch (index) {
        case 0: u = UnitType::Millimeter; break;
        case 1: u = UnitType::Centimeter; break;
        case 2: u = UnitType::Meter; break;
        case 3: u = UnitType::Inch; break;
        case 4: u = UnitType::Foot; break;
    }
    m_document->unitSettings().setType(u);
    m_scene->update();
    updateStatusBarStatus();
}

void MainWindow::connectActions() {
    // Ribbon file actions
    connect(m_ribbon->actionNew(), &QAction::triggered, this, &MainWindow::newDrawing);
    connect(m_ribbon->actionOpen(), &QAction::triggered, this, &MainWindow::openDrawing);
    connect(m_ribbon->actionSave(), &QAction::triggered, this, &MainWindow::saveDrawing);
    connect(m_ribbon->actionSaveAs(), &QAction::triggered, this, &MainWindow::saveDrawingAs);
    connect(m_ribbon->actionExport(), &QAction::triggered, this, &MainWindow::exportDrawing);
    connect(m_ribbon->actionPageSetup(), &QAction::triggered, this, &MainWindow::showPageSetup);

    // Ribbon navigation actions
    connect(m_ribbon->actionZoomIn(), &QAction::triggered, m_view, &CadView::zoomIn);
    connect(m_ribbon->actionZoomOut(), &QAction::triggered, m_view, &CadView::zoomOut);
    connect(m_ribbon->actionZoomFit(), &QAction::triggered, this, &MainWindow::zoomFit);
    connect(m_ribbon->actionResetZoom(), &QAction::triggered, m_view, &CadView::resetZoom);

    // Ribbon Draw tools
    connect(m_ribbon->actionSelect(), &QAction::triggered, [this]() { selectTool(m_selectTool.get()); });
    connect(m_ribbon->actionLine(), &QAction::triggered, [this]() { selectTool(m_lineTool.get()); });
    connect(m_ribbon->actionPolyline(), &QAction::triggered, [this]() { selectTool(m_polylineTool.get()); });
    connect(m_ribbon->actionCircle(), &QAction::triggered, [this]() { selectTool(m_circleTool.get()); });
    connect(m_ribbon->actionRectangle(), &QAction::triggered, [this]() { selectTool(m_rectangleTool.get()); });
    connect(m_ribbon->actionArc(), &QAction::triggered, [this]() { selectTool(m_arcTool.get()); });
    connect(m_ribbon->actionPolygon(), &QAction::triggered, [this]() { selectTool(m_polygonTool.get()); });
    connect(m_ribbon->actionEllipse(), &QAction::triggered, [this]() { selectTool(m_ellipseTool.get()); });
    connect(m_ribbon->actionText(), &QAction::triggered, [this]() { selectTool(m_textTool.get()); });
    connect(m_ribbon->actionDimension(), &QAction::triggered, [this]() { selectTool(m_dimensionTool.get()); });

    // Ribbon Modify tools
    connect(m_ribbon->actionMove(), &QAction::triggered, [this]() { selectTool(m_moveTool.get()); });
    connect(m_ribbon->actionRotate(), &QAction::triggered, [this]() { selectTool(m_rotateTool.get()); });
    connect(m_ribbon->actionMirror(), &QAction::triggered, [this]() { selectTool(m_mirrorTool.get()); });
    connect(m_ribbon->actionCopy(), &QAction::triggered, [this]() { selectTool(m_copyTool.get()); });
    connect(m_ribbon->actionOffset(), &QAction::triggered, [this]() { selectTool(m_offsetTool.get()); });
    connect(m_ribbon->actionTrim(), &QAction::triggered, [this]() { selectTool(m_trimTool.get()); });
    connect(m_ribbon->actionExtend(), &QAction::triggered, [this]() { selectTool(m_extendTool.get()); });
    connect(m_ribbon->actionDelete(), &QAction::triggered, this, &MainWindow::deleteSelected);

    // Ribbon Drafting Modes
    connect(m_ribbon->actionGrid(), &QAction::triggered, this, &MainWindow::toggleGridSnap);
    connect(m_ribbon->actionSnap(), &QAction::triggered, this, &MainWindow::toggleSnap);
    connect(m_ribbon->actionOrtho(), &QAction::triggered, this, &MainWindow::toggleOrtho);
    connect(m_ribbon->actionPolar(), &QAction::triggered, this, &MainWindow::togglePolar);

    // Ribbon Cloud actions
    connect(m_ribbon->actionCloudWorkspace(), &QAction::triggered, this, &MainWindow::openCloudWorkspace);
    connect(m_ribbon->actionCloudUpload(), &QAction::triggered, this, &MainWindow::saveToCloud);
    connect(m_ribbon->actionCloudDownload(), &QAction::triggered, this, &MainWindow::openFromCloud);

    // Ribbon AI action (Phase 4)
    connect(m_ribbon->actionAiAssistant(), &QAction::triggered, this, &MainWindow::openAiAssistant);
    connect(m_ribbon->actionAiGenerate(), &QAction::triggered, this, &MainWindow::aiGenerateFromPrompt);
    connect(m_ribbon->actionAiDimension(), &QAction::triggered, this, &MainWindow::aiAutoDimension);
    connect(m_ribbon->actionAiAnalyze(), &QAction::triggered, this, &MainWindow::aiAnalyzeDrawing);
    connect(m_ribbon->actionAiClean(), &QAction::triggered, this, &MainWindow::aiCleanDrawing);

    // Ribbon Layer signals
    connect(m_ribbon, &CadRibbon::activeLayerChanged, this, [this](const QString& name) {
        if (m_document && m_document->layerManager()) {
            m_document->layerManager()->setActiveLayer(name);
        }
    });
    connect(m_ribbon, &CadRibbon::newLayerRequested, this, [this]() {
        if (m_layerWidget) m_layerWidget->showNewLayerDialog();
    });
    connect(m_ribbon, &CadRibbon::layerPropertiesRequested, this, [this]() {
        if (m_layerDock) m_layerDock->setVisible(!m_layerDock->isVisible());
    });
    connect(m_ribbon, &CadRibbon::propertiesDockRequested, this, [this]() {
        if (m_propertiesDock) m_propertiesDock->setVisible(!m_propertiesDock->isVisible());
    });
    connect(m_ribbon, &CadRibbon::commandLineRequested, this, [this]() {
        if (m_commandLine) m_commandLine->setFocusToInput();
    });

    // Also mirror to ToolBar for compatibility
    if (m_toolBar) {
        connect(m_toolBar->actionNew(), &QAction::triggered, this, &MainWindow::newDrawing);
        connect(m_toolBar->actionOpen(), &QAction::triggered, this, &MainWindow::openDrawing);
        connect(m_toolBar->actionSave(), &QAction::triggered, this, &MainWindow::saveDrawing);
        connect(m_toolBar->actionExport(), &QAction::triggered, this, &MainWindow::exportDrawing);
        connect(m_toolBar->actionSelect(), &QAction::triggered, [this]() { selectTool(m_selectTool.get()); });
        connect(m_toolBar->actionLine(), &QAction::triggered, [this]() { selectTool(m_lineTool.get()); });
        connect(m_toolBar->actionCircle(), &QAction::triggered, [this]() { selectTool(m_circleTool.get()); });
        connect(m_toolBar->actionRectangle(), &QAction::triggered, [this]() { selectTool(m_rectangleTool.get()); });
        connect(m_toolBar->actionArc(), &QAction::triggered, [this]() { selectTool(m_arcTool.get()); });
        connect(m_toolBar->actionPolyline(), &QAction::triggered, [this]() { selectTool(m_polylineTool.get()); });
        connect(m_toolBar->actionPolygon(), &QAction::triggered, [this]() { selectTool(m_polygonTool.get()); });
        connect(m_toolBar->actionEllipse(), &QAction::triggered, [this]() { selectTool(m_ellipseTool.get()); });
        connect(m_toolBar->actionText(), &QAction::triggered, [this]() { selectTool(m_textTool.get()); });
        connect(m_toolBar->actionDimension(), &QAction::triggered, [this]() { selectTool(m_dimensionTool.get()); });
        connect(m_toolBar->actionDelete(), &QAction::triggered, this, &MainWindow::deleteSelected);
    }

    // View coordinate & snap updates
    connect(m_view, &CadView::mouseCoordinatesChanged, this, &MainWindow::onCoordinatesChanged);
    connect(m_view, &CadView::snapToggled, this, &MainWindow::updateStatusBarStatus);
    connect(m_view, &CadView::gridSnapToggled, this, &MainWindow::updateStatusBarStatus);
    connect(m_view, &CadView::orthoToggled, this, &MainWindow::updateStatusBarStatus);
    connect(m_view, &CadView::polarToggled, this, &MainWindow::updateStatusBarStatus);
    connect(m_view, &CadView::commandExecuted, this, &MainWindow::executeCadCommand);
    connect(m_view, &CadView::pageSetupRequested, this, &MainWindow::showPageSetup);
    connect(m_view, &CadView::propertiesRequested, this, [this]() {
        if (m_propertiesDock) {
            m_propertiesDock->show();
            m_propertiesDock->raise();
        }
    });

    connect(m_snapManager.get(), &SnapManager::snapSettingsChanged, this, &MainWindow::updateStatusBarStatus);

    // Dynamic Command Line connections
    connect(m_commandLine, &CadCommandLine::commandEntered, this, &MainWindow::executeCadCommand);
    connect(m_commandLine, &CadCommandLine::valuesCommitted, this, &MainWindow::onExactDimensionsCommitted);
    connect(m_commandLine, &CadCommandLine::cancelled, this, [this]() {
        m_ribbon->actionSelect()->setChecked(true);
        selectTool(m_selectTool.get());
    });

    // Document state updates
    connect(m_document.get(), &CadDocument::selectionChanged, this, &MainWindow::onSelectionChanged);
    connect(m_document.get(), &CadDocument::modifiedChanged, this, &MainWindow::onModifiedChanged);
    connect(m_document.get(), &CadDocument::filePathChanged, this, &MainWindow::onFilePathChanged);
    connect(m_document.get(), &CadDocument::pageSettingsChanged, this, [this]() {
        m_scene->update();
        updateStatusBarStatus();
    });
    connect(m_document.get(), &CadDocument::unitSettingsChanged, this, [this]() {
        m_scene->update();
        updateStatusBarStatus();
    });

    if (m_document->layerManager()) {
        connect(m_document->layerManager(), &LayerManager::activeLayerChanged, this, [this](const QString& name) {
            if (m_layerStatusLabel) {
                m_layerStatusLabel->setText(QStringLiteral("Layer: %1").arg(name));
            }
            if (m_ribbon) {
                m_ribbon->updateActiveLayer(name);
            }
        });
    }
}

void MainWindow::updateStatusBarStatus() {
    bool snapOn = m_snapManager && m_snapManager->isSnapEnabled();
    bool gridOn = m_snapManager && m_snapManager->isGridSnapEnabled();
    bool orthoOn = m_view && m_view->isOrthoEnabled();
    bool polarOn = m_snapManager && m_snapManager->isPolarTrackingEnabled();

    if (m_btnSnap) m_btnSnap->setChecked(snapOn);
    if (m_btnGrid) m_btnGrid->setChecked(gridOn);
    if (m_btnOrtho) m_btnOrtho->setChecked(orthoOn);
    if (m_btnPolar) m_btnPolar->setChecked(polarOn);

    if (m_ribbon) {
        m_ribbon->updateDraftingState(snapOn, gridOn, orthoOn, polarOn);
    }

    if (m_unitsCombo && m_document) {
        int idx = 0;
        switch (m_document->unitSettings().type()) {
            case UnitType::Millimeter: idx = 0; break;
            case UnitType::Centimeter: idx = 1; break;
            case UnitType::Meter: idx = 2; break;
            case UnitType::Inch: idx = 3; break;
            case UnitType::Foot: idx = 4; break;
            default: idx = 0; break;
        }
        if (m_unitsCombo->currentIndex() != idx) {
            m_unitsCombo->blockSignals(true);
            m_unitsCombo->setCurrentIndex(idx);
            m_unitsCombo->blockSignals(false);
        }
    }
}

void MainWindow::toggleSnap() {
    if (m_snapManager) {
        m_snapManager->setSnapEnabled(!m_snapManager->isSnapEnabled());
        updateStatusBarStatus();
        m_commandLine->appendMessage(m_snapManager->isSnapEnabled() ? tr("Object Snap: ON") : tr("Object Snap: OFF"));
    }
}

void MainWindow::toggleGridSnap() {
    if (m_snapManager) {
        m_snapManager->setGridSnapEnabled(!m_snapManager->isGridSnapEnabled());
        updateStatusBarStatus();
        m_commandLine->appendMessage(m_snapManager->isGridSnapEnabled() ? tr("Grid Snap: ON") : tr("Grid Snap: OFF"));
    }
}

void MainWindow::toggleOrtho() {
    if (m_view) {
        m_view->setOrthoEnabled(!m_view->isOrthoEnabled());
        updateStatusBarStatus();
        m_commandLine->appendMessage(m_view->isOrthoEnabled() ? tr("ORTHO mode: ON") : tr("ORTHO mode: OFF"));
    }
}

void MainWindow::togglePolar() {
    if (m_snapManager) {
        m_snapManager->setPolarTrackingEnabled(!m_snapManager->isPolarTrackingEnabled());
        updateStatusBarStatus();
        m_commandLine->appendMessage(m_snapManager->isPolarTrackingEnabled() ? tr("Polar Tracking: ON") : tr("Polar Tracking: OFF"));
    }
}

void MainWindow::showPageSetup() {
    PageSetupDialog dlg(m_document.get(), this);
    if (dlg.exec() == QDialog::Accepted) {
        dlg.applySettings(m_document.get());
        m_scene->update();
        updateStatusBarStatus();
        m_commandLine->appendMessage(tr("Drawing limits and units updated."));
    }
}

void MainWindow::selectTool(Tool* tool) {
    if (!tool) return;
    m_view->setTool(tool);
    if (m_toolLabel) {
        m_toolLabel->setText(QStringLiteral("Tool: %1").arg(tool->name()));
    }
    if (m_commandLine) {
        m_commandLine->setPrompt(QStringLiteral("Command [%1]:").arg(tool->name()));
    }
}

void MainWindow::onCoordinatesChanged(const QPointF& worldPos) {
    if (m_coordLabel && m_document) {
        const auto& units = m_document->unitSettings();
        double x = units.fromInternal(worldPos.x());
        double y = units.fromInternal(worldPos.y());
        int prec = units.precision();
        QString suffix = units.unitSuffix();
        QString text = QStringLiteral("X: %1 %3   Y: %2 %3   Z: 0.00 %3")
            .arg(x, 0, 'f', prec)
            .arg(y, 0, 'f', prec)
            .arg(suffix);

        m_coordLabel->setText(text);
    }
}

void MainWindow::onSelectionChanged() {
    if (m_selectionLabel) {
        m_selectionLabel->setText(QStringLiteral("%1 selected").arg(m_document->selectionCount()));
    }
}

void MainWindow::onModifiedChanged(bool /*modified*/) {
    updateTitle();
}

void MainWindow::onFilePathChanged(const QString& /*path*/) {
    updateTitle();
}

void MainWindow::updateTitle() {
    QString title = QStringLiteral("GenCAD - %1").arg(m_document->fileName());
    if (m_document->isModified()) {
        title += QStringLiteral("*");
    }
    setWindowTitle(title);
}

bool MainWindow::maybeSave() {
    if (!m_document->isModified()) return true;

    QMessageBox::StandardButton ret = QMessageBox::warning(
        this,
        tr("GenCAD"),
        tr("The document has unsaved changes.\nDo you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    if (ret == QMessageBox::Save) {
        return saveDrawing();
    } else if (ret == QMessageBox::Cancel) {
        return false;
    }
    return true;
}

void MainWindow::executeCadCommand(const QString& command) {
    QString cmd = command.trimmed().toUpper();
    if (cmd.isEmpty()) return;

    if (cmd == QStringLiteral("LINE") || cmd == QStringLiteral("L")) {
        m_ribbon->actionLine()->setChecked(true);
        selectTool(m_lineTool.get());
    } else if (cmd == QStringLiteral("CIRCLE") || cmd == QStringLiteral("C")) {
        m_ribbon->actionCircle()->setChecked(true);
        selectTool(m_circleTool.get());
    } else if (cmd == QStringLiteral("RECTANGLE") || cmd == QStringLiteral("REC") || cmd == QStringLiteral("RECT")) {
        m_ribbon->actionRectangle()->setChecked(true);
        selectTool(m_rectangleTool.get());
    } else if (cmd == QStringLiteral("ARC") || cmd == QStringLiteral("A")) {
        m_ribbon->actionArc()->setChecked(true);
        selectTool(m_arcTool.get());
    } else if (cmd == QStringLiteral("POLYLINE") || cmd == QStringLiteral("PL") || cmd == QStringLiteral("PLINE")) {
        m_ribbon->actionPolyline()->setChecked(true);
        selectTool(m_polylineTool.get());
    } else if (cmd == QStringLiteral("POLYGON") || cmd == QStringLiteral("POL")) {
        m_ribbon->actionPolygon()->setChecked(true);
        selectTool(m_polygonTool.get());
    } else if (cmd == QStringLiteral("ELLIPSE") || cmd == QStringLiteral("EL")) {
        m_ribbon->actionEllipse()->setChecked(true);
        selectTool(m_ellipseTool.get());
    } else if (cmd == QStringLiteral("DIMENSION") || cmd == QStringLiteral("DIM") || cmd == QStringLiteral("DIMALIGNED")) {
        m_ribbon->actionDimension()->setChecked(true);
        selectTool(m_dimensionTool.get());
    } else if (cmd == QStringLiteral("TEXT") || cmd == QStringLiteral("T") || cmd == QStringLiteral("MT") || cmd == QStringLiteral("MTEXT")) {
        m_ribbon->actionText()->setChecked(true);
        selectTool(m_textTool.get());
    } else if (cmd == QStringLiteral("MOVE") || cmd == QStringLiteral("M")) {
        m_ribbon->actionMove()->setChecked(true);
        selectTool(m_moveTool.get());
    } else if (cmd == QStringLiteral("ROTATE") || cmd == QStringLiteral("RO")) {
        m_ribbon->actionRotate()->setChecked(true);
        selectTool(m_rotateTool.get());
    } else if (cmd == QStringLiteral("MIRROR") || cmd == QStringLiteral("MI")) {
        m_ribbon->actionMirror()->setChecked(true);
        selectTool(m_mirrorTool.get());
    } else if (cmd == QStringLiteral("COPY") || cmd == QStringLiteral("CO") || cmd == QStringLiteral("CP")) {
        m_ribbon->actionCopy()->setChecked(true);
        selectTool(m_copyTool.get());
    } else if (cmd == QStringLiteral("OFFSET") || cmd == QStringLiteral("O")) {
        m_ribbon->actionOffset()->setChecked(true);
        selectTool(m_offsetTool.get());
    } else if (cmd == QStringLiteral("TRIM") || cmd == QStringLiteral("TR")) {
        m_ribbon->actionTrim()->setChecked(true);
        selectTool(m_trimTool.get());
    } else if (cmd == QStringLiteral("EXTEND") || cmd == QStringLiteral("EX")) {
        m_ribbon->actionExtend()->setChecked(true);
        selectTool(m_extendTool.get());
    } else if (cmd == QStringLiteral("SELECT") || cmd == QStringLiteral("SEL") || cmd == QStringLiteral("ESC")) {
        m_ribbon->actionSelect()->setChecked(true);
        selectTool(m_selectTool.get());
    } else if (cmd == QStringLiteral("DELETE") || cmd == QStringLiteral("DEL") || cmd == QStringLiteral("ERASE") || cmd == QStringLiteral("E")) {
        deleteSelected();
    } else if (cmd == QStringLiteral("ZOOM") || cmd == QStringLiteral("Z") || cmd == QStringLiteral("FIT")) {
        zoomFit();
    } else if (cmd == QStringLiteral("PAGESETUP") || cmd == QStringLiteral("LIMITS")) {
        showPageSetup();
    } else if (cmd == QStringLiteral("ORTHO") || cmd == QStringLiteral("ORTHO TOGGLE") || cmd == QStringLiteral("ORTHOMODE")) {
        toggleOrtho();
    } else if (cmd == QStringLiteral("ORTHO ON") || cmd == QStringLiteral("ORTHOMODE 1")) {
        if (m_view) m_view->setOrthoEnabled(true);
        updateStatusBarStatus();
        m_commandLine->appendMessage(tr("ORTHO mode: ON"));
    } else if (cmd == QStringLiteral("ORTHO OFF") || cmd == QStringLiteral("ORTHOMODE 0")) {
        if (m_view) m_view->setOrthoEnabled(false);
        updateStatusBarStatus();
        m_commandLine->appendMessage(tr("ORTHO mode: OFF"));
    } else if (cmd == QStringLiteral("POLAR")) {
        togglePolar();
    } else if (cmd == QStringLiteral("SNAP") || cmd == QStringLiteral("OSNAP")) {
        toggleSnap();
    } else if (cmd == QStringLiteral("GRID")) {
        toggleGridSnap();
    } else if (cmd == QStringLiteral("UNDO") || cmd == QStringLiteral("U")) {
        if (m_document->undoStack()->canUndo()) m_document->undoStack()->undo();
    } else if (cmd == QStringLiteral("REDO")) {
        if (m_document->undoStack()->canRedo()) m_document->undoStack()->redo();
    } else if (cmd == QStringLiteral("PROPERTIES") || cmd == QStringLiteral("PR")) {
        if (m_propertiesDock) { m_propertiesDock->show(); m_propertiesDock->raise(); }
    } else if (cmd == QStringLiteral("LAYER") || cmd == QStringLiteral("LA")) {
        if (m_layerDock) { m_layerDock->show(); m_layerDock->raise(); }
    } else if (cmd == QStringLiteral("EXPORT") || cmd == QStringLiteral("EXP")) {
        exportDrawing();
    } else if (cmd == QStringLiteral("EXPORTDXF") || cmd == QStringLiteral("DXFOUT")) {
        exportDxf();
    } else if (cmd == QStringLiteral("DXFIN")) {
        importDxf();
    } else if (cmd == QStringLiteral("AI")) {
        openAiAssistant();
    } else {
        m_commandLine->appendMessage(tr("Unknown command: %1. Type 'HELP' for commands.").arg(cmd));
    }
}

void MainWindow::onExactDimensionsCommitted(double v1, double v2, double v3) {
    Tool* active = m_view->currentTool();
    if (!active) return;

    if (active == m_lineTool.get()) {
        m_lineTool->commitExact(v1, v2);
    } else if (active == m_rectangleTool.get()) {
        m_rectangleTool->commitExact(v1, v2);
    } else if (active == m_circleTool.get()) {
        m_circleTool->commitExact(v1, v2);
    } else if (active == m_polygonTool.get()) {
        m_polygonTool->commitExact(v1, v2, v3);
    } else if (active == m_ellipseTool.get()) {
        m_ellipseTool->commitExact(v1, v2, v3);
    } else if (active == m_arcTool.get()) {
        m_arcTool->commitExact(v1, v2, v3);
    } else if (active == m_polylineTool.get()) {
        m_polylineTool->commitExactSegment(v1, v2);
    }
}

void MainWindow::newDrawing() {
    if (!maybeSave()) return;
    PageSetupDialog dlg(m_document.get(), this);
    if (dlg.exec() == QDialog::Accepted) {
        m_document->clear();
        dlg.applySettings(m_document.get());
        m_ribbon->actionSelect()->setChecked(true);
        selectTool(m_selectTool.get());
        m_view->resetZoom();
        m_scene->update();
        updateStatusBarStatus();
        updateTitle();
    }
}

void MainWindow::openDrawing() {
    if (!maybeSave()) return;

    QString filter = tr("All Supported Files (*.gencad *.dxf);;GenCAD Project (*.gencad);;DXF Drawing (*.dxf);;All Files (*.*)");
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open Drawing"),
        QString(),
        filter
    );

    if (fileName.isEmpty()) return;

    QString error;
    QStringList warnings;
    bool isDxf = fileName.endsWith(QStringLiteral(".dxf"), Qt::CaseInsensitive);

    if (isDxf) {
        if (!DxfFile::load(fileName, m_document.get(), &error, &warnings)) {
            QMessageBox::critical(this, tr("Open Error"), tr("Failed to open DXF file:\n%1").arg(error));
            return;
        }
        m_document->setFilePath(QString());
        m_document->setModified(false);
        m_commandLine->appendMessage(tr("DXF drawing imported: %1").arg(fileName));
    } else {
        if (!GencadFile::load(fileName, m_document.get(), &error)) {
            // If failed as JSON, check if it could be a DXF file with non-standard extension
            if (DxfFile::load(fileName, m_document.get(), nullptr, &warnings)) {
                m_document->setFilePath(QString());
                m_document->setModified(false);
                m_commandLine->appendMessage(tr("DXF drawing imported: %1").arg(fileName));
            } else {
                QMessageBox::critical(this, tr("Open Error"), tr("Failed to open file:\n%1").arg(error));
                return;
            }
        } else {
            m_commandLine->appendMessage(tr("GenCAD project loaded: %1").arg(fileName));
        }
    }

    m_ribbon->actionSelect()->setChecked(true);
    selectTool(m_selectTool.get());
    m_view->zoomFit();
    m_scene->update();
    updateStatusBarStatus();
    updateTitle();
    if (!warnings.isEmpty()) {
        m_commandLine->appendMessage(tr("Opened with warnings: %1").arg(warnings.join(QStringLiteral(", "))));
    }
}

bool MainWindow::saveDrawing() {
    if (m_document->filePath().isEmpty()) {
        return saveDrawingAs();
    }

    // Ensure filePath does not have a .dxf or .dwg extension
    QFileInfo fi(m_document->filePath());
    QString ext = fi.suffix().toLower();
    if (ext == QStringLiteral("dxf") || ext == QStringLiteral("dwg")) {
        return saveDrawingAs();
    }

    QString error;
    if (!GencadFile::save(m_document->filePath(), m_document.get(), &error)) {
        QMessageBox::critical(this, tr("Save Error"), tr("Failed to save file:\n%1").arg(error));
        return false;
    }

    updateTitle();
    m_commandLine->appendMessage(tr("Project saved: %1").arg(m_document->filePath()));
    return true;
}

bool MainWindow::saveDrawingAs() {
    QString defaultName = m_document->filePath();
    if (defaultName.isEmpty()) {
        defaultName = QStringLiteral("drawing.gencad");
    } else {
        QFileInfo fi(defaultName);
        if (fi.suffix().toLower() == QStringLiteral("dxf") || fi.suffix().toLower() == QStringLiteral("dwg")) {
            defaultName = fi.completeBaseName() + QStringLiteral(".gencad");
        }
    }

    QString filter = GencadFile::fileFilter();
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save GenCAD Project As"),
        defaultName,
        filter
    );

    if (fileName.isEmpty()) return false;

    // Never save JSON content with .dxf or .dwg
    QFileInfo fi(fileName);
    QString ext = fi.suffix().toLower();
    if (ext == QStringLiteral("dxf") || ext == QStringLiteral("dwg")) {
        QMessageBox::warning(this, tr("Invalid File Extension"),
            tr("Cannot save native GenCAD project as .%1.\nNative GenCAD projects must use the .gencad extension.\nTo export a DXF drawing, use File -> Export Drawing.").arg(ext));
        return false;
    }

    // If no extension, append .gencad
    if (ext.isEmpty()) {
        fileName += QStringLiteral(".gencad");
    }

    QString error;
    if (!GencadFile::save(fileName, m_document.get(), &error)) {
        QMessageBox::critical(this, tr("Save Error"), tr("Failed to save file:\n%1").arg(error));
        return false;
    }

    updateTitle();
    m_commandLine->appendMessage(tr("Project saved as: %1").arg(fileName));
    return true;
}

void MainWindow::exportDrawing() {
    if (!m_document || m_document->entityCount() == 0) {
        QMessageBox::warning(this, tr("Export Drawing"), tr("The current drawing is empty. Cannot export an empty drawing."));
        return;
    }

    QString defaultBase = m_document->filePath().isEmpty() ? QStringLiteral("drawing") 
        : QFileInfo(m_document->filePath()).completeBaseName();
    QString defaultName = defaultBase + QStringLiteral(".dxf");

    QString selectedFilter;
    QString filter = DrawingExporter::exportFilter();
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export Drawing"),
        defaultName,
        filter,
        &selectedFilter
    );

    if (fileName.isEmpty()) return;

    // Resolve extension if omitted by user
    QFileInfo fi(fileName);
    if (fi.suffix().isEmpty()) {
        if (selectedFilter.contains(QStringLiteral("*.pdf"))) {
            fileName += QStringLiteral(".pdf");
        } else if (selectedFilter.contains(QStringLiteral("*.png"))) {
            fileName += QStringLiteral(".png");
        } else {
            fileName += QStringLiteral(".dxf");
        }
    }

    QString error;
    QStringList warnings;
    ExportOptions options;
    options.backgroundColor = (m_currentTheme == CadTheme::ThemeMode::Dark) ? QColor(33, 37, 43) : Qt::white;
    if (fileName.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive)) {
        options.backgroundColor = Qt::white;
    }

    if (!DrawingExporter::exportDrawing(fileName, m_document.get(), options, &error, &warnings)) {
        QMessageBox::critical(this, tr("Export Failed"), tr("Failed to export drawing to %1:\n%2").arg(fileName, error));
        return;
    }

    if (!warnings.isEmpty()) {
        m_commandLine->appendMessage(tr("Export completed with warnings:\n%1").arg(warnings.join(QStringLiteral(", "))));
    }
    m_commandLine->appendMessage(tr("Drawing exported successfully: %1").arg(fileName));
}

void MainWindow::exportDxf() {
    if (!m_document || m_document->entityCount() == 0) {
        QMessageBox::warning(this, tr("Export DXF"), tr("The current drawing is empty. Cannot export an empty drawing."));
        return;
    }

    QString defaultBase = m_document->filePath().isEmpty() ? QStringLiteral("drawing") 
        : QFileInfo(m_document->filePath()).completeBaseName();
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export DXF Drawing"),
        defaultBase + QStringLiteral(".dxf"),
        QStringLiteral("DXF Drawing (*.dxf);;All Files (*.*)")
    );

    if (fileName.isEmpty()) return;

    if (!fileName.endsWith(QStringLiteral(".dxf"), Qt::CaseInsensitive)) {
        fileName += QStringLiteral(".dxf");
    }

    QString error;
    QStringList warnings;
    if (!DrawingExporter::exportDxf(fileName, m_document.get(), &error, &warnings)) {
        QMessageBox::critical(this, tr("DXF Export Error"), tr("Failed to export DXF:\n%1").arg(error));
        return;
    }

    if (!warnings.isEmpty()) {
        m_commandLine->appendMessage(tr("DXF exported with warnings: %1").arg(warnings.join(QStringLiteral(", "))));
    }
    m_commandLine->appendMessage(tr("Exported DXF: %1").arg(fileName));
}

void MainWindow::importDxf() {
    if (!maybeSave()) return;

    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Import DXF Drawing"),
        QString(),
        QStringLiteral("DXF Drawing (*.dxf);;All Files (*.*)")
    );

    if (fileName.isEmpty()) return;

    QString error;
    QStringList warnings;
    if (!DxfFile::load(fileName, m_document.get(), &error, &warnings)) {
        QMessageBox::critical(this, tr("DXF Import Error"), tr("Failed to import DXF:\n%1").arg(error));
        return;
    }

    // Clear filePath to prevent saving JSON into the DXF file
    m_document->setFilePath(QString());
    m_document->setModified(false);

    m_ribbon->actionSelect()->setChecked(true);
    selectTool(m_selectTool.get());
    m_view->zoomFit();
    m_scene->update();
    updateStatusBarStatus();
    updateTitle();
    if (!warnings.isEmpty()) {
        m_commandLine->appendMessage(tr("Imported with warnings: %1").arg(warnings.join(QStringLiteral(", "))));
    }
    m_commandLine->appendMessage(tr("Imported DXF: %1").arg(fileName));
}

void MainWindow::deleteSelected() {
    auto selected = m_document->selectedEntities();
    if (!selected.empty()) {
        m_document->undoStack()->push(new DeleteEntityCommand(m_document.get(), selected));
        m_commandLine->appendMessage(tr("Deleted %1 entities.").arg(selected.size()));
    }
}

void MainWindow::selectAll() {
    for (const auto& entity : m_document->entities()) {
        if (entity) {
            m_document->select(entity->id(), true);
        }
    }
}

void MainWindow::clearSelection() {
    m_document->clearSelection();
}

void MainWindow::zoomFit() {
    m_view->zoomFit();
}

void MainWindow::showAbout() {
    QMessageBox::about(
        this,
        tr("About GenCAD"),
        tr("<h3>GenCAD 4.0 - AutoCAD Edition</h3>"
           "<p>Professional Desktop 2D CAD Workstation</p>"
           "<p>Built with C++17, Qt 6, and Autonomous AI Geometry Tools.</p>"
           "<p>Features AutoCAD-inspired drafting ribbon, dynamic crosshair cursor, "
           "multi-line command line with auto-completion, dockable layer and properties managers, "
           "and parametric dimensioning.</p>")
    );
}

void MainWindow::openCloudWorkspace() {
    CloudWorkspaceDialog dlg(m_cloudClient.get(), m_document.get(), this);
    connect(&dlg, &CloudWorkspaceDialog::documentLoaded, this, [this](const QString& name) {
        m_view->zoomFit();
        m_scene->update();
        updateStatusBarStatus();
        updateTitle();
        m_commandLine->appendMessage(tr("Cloud drawing opened: %1").arg(name));
    });
    dlg.exec();
}

void MainWindow::saveToCloud() {
    if (!m_cloudClient->isLoggedIn()) {
        openCloudWorkspace();
        return;
    }
    openCloudWorkspace();
}

void MainWindow::openFromCloud() {
    openCloudWorkspace();
}

void MainWindow::updateCloudStatusBar() {
    if (!m_cloudStatusLabel) return;
    if (m_cloudClient && m_cloudClient->isLoggedIn()) {
        m_cloudStatusLabel->setText(QStringLiteral("Cloud: %1").arg(m_cloudClient->username()));
        m_cloudStatusLabel->setStyleSheet(QStringLiteral("color: #00ff7f; font-weight: bold; font-size: 10px;"));
    } else {
        m_cloudStatusLabel->setText(QStringLiteral("Cloud: Offline"));
        m_cloudStatusLabel->setStyleSheet(QStringLiteral("color: #828997; font-size: 10px;"));
    }
}

void MainWindow::openAiAssistant() {
    if (m_aiDock) {
        m_aiDock->show();
        m_aiDock->raise();
    }
}

void MainWindow::aiGenerateFromPrompt() {
    openAiAssistant();
    if (m_aiDock) {
        m_aiDock->promptForGeneration();
    }
}

void MainWindow::aiAutoDimension() {
    openAiAssistant();
    if (m_aiDock) {
        m_aiDock->runAutoDimension();
    }
}

void MainWindow::aiAnalyzeDrawing() {
    openAiAssistant();
    if (m_aiDock) {
        m_aiDock->runAnalysis();
    }
}

void MainWindow::aiCleanDrawing() {
    openAiAssistant();
    if (m_aiDock) {
        m_aiDock->runAutoClean();
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}
