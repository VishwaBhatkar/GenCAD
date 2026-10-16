#include "ui/CadRibbon.h"
#include "document/CadDocument.h"
#include "layers/LayerManager.h"
#include "layers/CadLayer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QStyle>
#include <QKeySequence>

CadRibbon::CadRibbon(CadDocument* document, QWidget* parent)
    : QWidget(parent)
    , m_document(document)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    createActions();

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    buildQuickAccessBar();
    buildRibbonTabs();

    mainLayout->addWidget(m_tabWidget);

    if (m_document) {
        setDocument(m_document);
    }
}

void CadRibbon::setDocument(CadDocument* document) {
    m_document = document;
    if (m_document && m_document->layerManager() && m_layerCombo) {
        auto refreshLayers = [this]() {
            if (!m_document || !m_document->layerManager() || !m_layerCombo) return;
            m_layerCombo->blockSignals(true);
            m_layerCombo->clear();
            const auto& layers = m_document->layerManager()->layers();
            QString active = m_document->layerManager()->activeLayerName();
            int activeIdx = 0;
            for (size_t i = 0; i < layers.size(); ++i) {
                const auto& layer = layers[i];
                // Create a circular color swatch icon
                QPixmap pix(14, 14);
                pix.fill(Qt::transparent);
                QPainter p(&pix);
                p.setRenderHint(QPainter::Antialiasing);
                p.setBrush(layer->color());
                p.setPen(QPen(Qt::white, 1));
                p.drawEllipse(1, 1, 12, 12);
                p.end();

                m_layerCombo->addItem(QIcon(pix), layer->name());
                if (layer->name() == active) {
                    activeIdx = static_cast<int>(i);
                }
            }
            m_layerCombo->setCurrentIndex(activeIdx);
            m_layerCombo->blockSignals(false);
        };

        connect(m_document->layerManager(), &LayerManager::layerAdded, this, refreshLayers);
        connect(m_document->layerManager(), &LayerManager::layerRemoved, this, refreshLayers);
        connect(m_document->layerManager(), &LayerManager::layerUpdated, this, refreshLayers);
        connect(m_document->layerManager(), &LayerManager::activeLayerChanged, this, [this](const QString& name) {
            updateActiveLayer(name);
        });
        connect(m_document->layerManager(), &LayerManager::layersReset, this, refreshLayers);

        refreshLayers();
    }
}

void CadRibbon::updateActiveLayer(const QString& layerName) {
    if (!m_layerCombo) return;
    int idx = m_layerCombo->findText(layerName);
    if (idx >= 0 && idx != m_layerCombo->currentIndex()) {
        m_layerCombo->blockSignals(true);
        m_layerCombo->setCurrentIndex(idx);
        m_layerCombo->blockSignals(false);
    }
}

void CadRibbon::updateDraftingState(bool snap, bool grid, bool ortho, bool polar) {
    if (m_actSnap) m_actSnap->setChecked(snap);
    if (m_actGrid) m_actGrid->setChecked(grid);
    if (m_actOrtho) m_actOrtho->setChecked(ortho);
    if (m_actPolar) m_actPolar->setChecked(polar);
}

void CadRibbon::createActions() {
    // 1. File actions
    m_actNew = new QAction(QIcon(QStringLiteral(":/icons/new.svg")), tr("New"), this);
    m_actNew->setShortcut(QKeySequence::New);
    m_actNew->setToolTip(tr("<b>New Drawing (Ctrl+N)</b><br>Create a new CAD drawing document"));

    m_actOpen = new QAction(QIcon(QStringLiteral(":/icons/open.svg")), tr("Open"), this);
    m_actOpen->setShortcut(QKeySequence::Open);
    m_actOpen->setToolTip(tr("<b>Open Drawing (Ctrl+O)</b><br>Open an existing GenCAD drawing (.gencad)"));

    m_actSave = new QAction(QIcon(QStringLiteral(":/icons/save.svg")), tr("Save"), this);
    m_actSave->setShortcut(QKeySequence::Save);
    m_actSave->setToolTip(tr("<b>Save Drawing (Ctrl+S)</b><br>Save current drawing to disk"));

    m_actSaveAs = new QAction(QIcon(QStringLiteral(":/icons/save_as.svg")), tr("Save As"), this);
    m_actSaveAs->setShortcut(QKeySequence::SaveAs);
    m_actSaveAs->setToolTip(tr("<b>Save Drawing As (Ctrl+Shift+S)</b><br>Save current drawing with a new file name"));

    m_actExport = new QAction(QIcon(QStringLiteral(":/icons/export.svg")), tr("Export"), this);
    m_actExport->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    m_actExport->setToolTip(tr("<b>Export Drawing (Ctrl+E)</b><br>Export drawing to standard DXF, PDF, or PNG"));

    m_actPageSetup = new QAction(QIcon(QStringLiteral(":/icons/settings.svg")), tr("Page Setup"), this);
    m_actPageSetup->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    m_actPageSetup->setToolTip(tr("<b>Page Setup / Drawing Limits</b><br>Configure paper sheet size, orientation, and units"));

    // 2. Clipboard / Undo
    m_actUndo = new QAction(QIcon(QStringLiteral(":/icons/undo.svg")), tr("Undo"), this);
    m_actUndo->setShortcut(QKeySequence::Undo);
    m_actUndo->setToolTip(tr("<b>Undo (Ctrl+Z)</b><br>Revert the last CAD operation"));

    m_actRedo = new QAction(QIcon(QStringLiteral(":/icons/redo.svg")), tr("Redo"), this);
    m_actRedo->setShortcut(QKeySequence::Redo);
    m_actRedo->setToolTip(tr("<b>Redo (Ctrl+Y)</b><br>Reapply the undone CAD operation"));

    // 3. Draw tools group
    m_drawModifyGroup = new QActionGroup(this);
    m_drawModifyGroup->setExclusive(true);

    auto addToolAction = [this](const QString& name, const QString& iconPath, const QKeySequence& shortcut, const QString& desc) -> QAction* {
        auto* act = new QAction(QIcon(iconPath), name, this);
        if (!shortcut.isEmpty()) act->setShortcut(shortcut);
        act->setCheckable(true);
        act->setToolTip(desc);
        m_drawModifyGroup->addAction(act);
        return act;
    };

    m_actSelect = addToolAction(tr("Select"), QStringLiteral(":/icons/select.svg"), QKeySequence(Qt::Key_Escape),
                                tr("<b>Select (Esc)</b><br>Pick objects or drag window/crossing selection boxes"));
    m_actSelect->setChecked(true);

    m_actLine = addToolAction(tr("Line"), QStringLiteral(":/icons/line.svg"), QKeySequence(Qt::Key_L),
                              tr("<b>Line (L)</b><br>Creates contiguous straight line segments"));

    m_actPolyline = addToolAction(tr("Polyline"), QStringLiteral(":/icons/polyline.svg"), QKeySequence(Qt::Key_P),
                                  tr("<b>Polyline (PL)</b><br>Creates 2D connected polyline entities"));

    m_actCircle = addToolAction(tr("Circle"), QStringLiteral(":/icons/circle.svg"), QKeySequence(Qt::Key_C),
                                tr("<b>Circle (C)</b><br>Creates circles using center and radius"));

    m_actRectangle = addToolAction(tr("Rectangle"), QStringLiteral(":/icons/rectangle.svg"), QKeySequence(Qt::Key_R),
                                   tr("<b>Rectangle (REC)</b><br>Creates rectangular polygon boundaries"));

    m_actArc = addToolAction(tr("Arc"), QStringLiteral(":/icons/arc.svg"), QKeySequence(Qt::Key_A),
                             tr("<b>Arc (A)</b><br>Creates circular arcs specified by center, radius, and angles"));

    m_actPolygon = addToolAction(tr("Polygon"), QStringLiteral(":/icons/polygon.svg"), QKeySequence(),
                                 tr("<b>Polygon (POL)</b><br>Creates regular equilateral polygons (3 to 64 sides)"));

    m_actEllipse = addToolAction(tr("Ellipse"), QStringLiteral(":/icons/ellipse.svg"), QKeySequence(Qt::Key_E),
                                 tr("<b>Ellipse (EL)</b><br>Creates true ellipses with major and minor radii"));

    m_actText = addToolAction(tr("Text"), QStringLiteral(":/icons/text.svg"), QKeySequence(Qt::Key_T),
                              tr("<b>Text (T)</b><br>Inserts CAD annotations and labels"));

    m_actDimension = addToolAction(tr("Dimension"), QStringLiteral(":/icons/dimension.svg"), QKeySequence(Qt::Key_D),
                                   tr("<b>Dimension (DIM)</b><br>Adds linear aligned CAD dimensions with witness lines"));

    // 4. Modify tools
    m_actMove = addToolAction(tr("Move"), QStringLiteral(":/icons/move.svg"), QKeySequence(Qt::Key_M),
                              tr("<b>Move (M)</b><br>Displaces selected entities by vector"));

    m_actRotate = addToolAction(tr("Rotate"), QStringLiteral(":/icons/rotate.svg"), QKeySequence(Qt::Key_R),
                                tr("<b>Rotate (RO)</b><br>Rotates entities around base point"));

    m_actMirror = addToolAction(tr("Mirror"), QStringLiteral(":/icons/mirror.svg"), QKeySequence(),
                                tr("<b>Mirror (MI)</b><br>Reflects entities across a symmetry axis"));

    m_actCopy = addToolAction(tr("Copy"), QStringLiteral(":/icons/copy.svg"), QKeySequence(),
                              tr("<b>Copy (CO)</b><br>Duplicates selected entities"));

    m_actOffset = addToolAction(tr("Offset"), QStringLiteral(":/icons/offset.svg"), QKeySequence(Qt::Key_O),
                                tr("<b>Offset (O)</b><br>Creates parallel concentric copies at distance"));

    m_actTrim = addToolAction(tr("Trim"), QStringLiteral(":/icons/trim.svg"), QKeySequence(),
                              tr("<b>Trim (TR)</b><br>Cuts geometry at intersecting cutting edges"));

    m_actExtend = addToolAction(tr("Extend"), QStringLiteral(":/icons/extend.svg"), QKeySequence(),
                                tr("<b>Extend (EX)</b><br>Extends geometry to boundary edges"));

    m_actDelete = new QAction(QIcon(QStringLiteral(":/icons/delete.svg")), tr("Delete"), this);
    m_actDelete->setShortcut(QKeySequence::Delete);
    m_actDelete->setToolTip(tr("<b>Delete (Del)</b><br>Removes selected entities from drawing"));

    // 5. Navigation
    m_actZoomIn = new QAction(QIcon(QStringLiteral(":/icons/zoom_in.svg")), tr("Zoom In"), this);
    m_actZoomIn->setShortcut(QKeySequence::ZoomIn);
    m_actZoomIn->setToolTip(tr("<b>Zoom In (Ctrl++)</b><br>Magnifies the view"));

    m_actZoomOut = new QAction(QIcon(QStringLiteral(":/icons/zoom_out.svg")), tr("Zoom Out"), this);
    m_actZoomOut->setShortcut(QKeySequence::ZoomOut);
    m_actZoomOut->setToolTip(tr("<b>Zoom Out (Ctrl+-)</b><br>Shrinks the view"));

    m_actZoomFit = new QAction(QIcon(QStringLiteral(":/icons/zoom_fit.svg")), tr("Zoom Extents"), this);
    m_actZoomFit->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
    m_actZoomFit->setToolTip(tr("<b>Zoom Extents (Ctrl+F)</b><br>Fits all drawing geometry to current window"));

    m_actResetZoom = new QAction(QIcon(QStringLiteral(":/icons/pan.svg")), tr("Reset 1:1"), this);
    m_actResetZoom->setToolTip(tr("<b>Reset Zoom</b><br>Resets view transform to 100% scale"));

    // 6. Drafting Modes
    m_actGrid = new QAction(QIcon(QStringLiteral(":/icons/grid.svg")), tr("Grid (F9)"), this);
    m_actGrid->setCheckable(true);
    m_actGrid->setShortcut(QKeySequence(Qt::Key_F9));
    m_actGrid->setToolTip(tr("<b>Grid Snap (F9)</b><br>Snaps cursor to discrete grid intervals"));

    m_actSnap = new QAction(QIcon(QStringLiteral(":/icons/snap.svg")), tr("Snap (F3)"), this);
    m_actSnap->setCheckable(true);
    m_actSnap->setChecked(true);
    m_actSnap->setShortcut(QKeySequence(Qt::Key_F3));
    m_actSnap->setToolTip(tr("<b>Object Snap (F3)</b><br>Magnetic snaps to Endpoints, Centers, Midpoints"));

    m_actOrtho = new QAction(QIcon(QStringLiteral(":/icons/ortho.svg")), tr("Ortho (F8)"), this);
    m_actOrtho->setCheckable(true);
    m_actOrtho->setShortcut(QKeySequence(Qt::Key_F8));
    m_actOrtho->setToolTip(tr("<b>ORTHO Mode (F8)</b><br>Constrains cursor movement strictly to 90° axes"));

    m_actPolar = new QAction(QIcon(QStringLiteral(":/icons/polar.svg")), tr("Polar (F10)"), this);
    m_actPolar->setCheckable(true);
    m_actPolar->setShortcut(QKeySequence(Qt::Key_F10));
    m_actPolar->setToolTip(tr("<b>Polar Tracking (F10)</b><br>Guides cursor along incremental angular rays"));

    // 7. Cloud Actions
    m_actCloudWorkspace = new QAction(QIcon(QStringLiteral(":/icons/cloud.svg")), tr("Workspace"), this);
    m_actCloudWorkspace->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    m_actCloudWorkspace->setToolTip(tr("<b>Cloud Workspace</b><br>Manage cloud projects and team models"));

    m_actCloudUpload = new QAction(QIcon(QStringLiteral(":/icons/cloud_upload.svg")), tr("Save Cloud"), this);
    m_actCloudUpload->setToolTip(tr("<b>Save to Cloud</b><br>Commit versioned drawing to cloud repository"));

    m_actCloudDownload = new QAction(QIcon(QStringLiteral(":/icons/cloud_download.svg")), tr("Open Cloud"), this);
    m_actCloudDownload->setToolTip(tr("<b>Open from Cloud</b><br>Pull remote drawing into current session"));

    // 8. AI Actions
    m_actAiAssistant = new QAction(QIcon(QStringLiteral(":/icons/ai.svg")), tr("AI Panel"), this);
    m_actAiAssistant->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A));
    m_actAiAssistant->setToolTip(tr("<b>AI Assistant Dock (Ctrl+Shift+A)</b><br>Open autonomous CAD copilot"));

    m_actAiGenerate = new QAction(QIcon(QStringLiteral(":/icons/ai.svg")), tr("Prompt Gen"), this);
    m_actAiGenerate->setToolTip(tr("<b>AI Generative CAD</b><br>Synthesize 2D engineering models from natural language"));

    m_actAiDimension = new QAction(QIcon(QStringLiteral(":/icons/dimension.svg")), tr("Auto-Dim"), this);
    m_actAiDimension->setToolTip(tr("<b>AI Auto-Dimensioning</b><br>Generate manufacturing dimensions automatically"));

    m_actAiAnalyze = new QAction(QIcon(QStringLiteral(":/icons/check.svg")), tr("Analyze"), this);
    m_actAiAnalyze->setToolTip(tr("<b>Geometry Analyzer</b><br>Inspect drafting errors, non-manifold gaps, and unclosed loops"));

    m_actAiClean = new QAction(QIcon(QStringLiteral(":/icons/delete.svg")), tr("Clean"), this);
    m_actAiClean->setToolTip(tr("<b>Auto-Clean Geometry</b><br>Deduplicate overlapping collinear lines and artifacts"));

    // 9. Theme Toggle
    m_actToggleTheme = new QAction(QIcon(QStringLiteral(":/icons/theme_dark.svg")), tr("Theme"), this);
    m_actToggleTheme->setToolTip(tr("<b>Toggle Theme</b><br>Switch between Dark AutoCAD and Light Engineering themes"));
}

void CadRibbon::buildQuickAccessBar() {
    // Quick access bar is built directly into top of main window or ribbon header
}

void CadRibbon::buildRibbonTabs() {
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setMovable(false);

    // Create Tabs
    m_tabWidget->addTab(createHomeTab(), tr("  Home  "));
    m_tabWidget->addTab(createAnnotateTab(), tr("  Annotate  "));
    m_tabWidget->addTab(createViewTab(), tr("  View  "));
    m_tabWidget->addTab(createCloudTab(), tr("  Cloud  "));
    m_tabWidget->addTab(createAiTab(), tr("  AI Assistant  "));

    // Collapse toggle button in upper corner of ribbon
    auto* collapseBtn = new QToolButton(m_tabWidget);
    collapseBtn->setText(QStringLiteral("▲"));
    collapseBtn->setToolTip(tr("Minimize or Expand Ribbon"));
    collapseBtn->setStyleSheet(QStringLiteral("QToolButton { border: none; font-size: 10px; color: #888; padding: 2px 6px; } QToolButton:hover { color: #fff; }"));
    connect(collapseBtn, &QToolButton::clicked, this, &CadRibbon::toggleRibbonCollapse);
    m_tabWidget->setCornerWidget(collapseBtn, Qt::TopRightCorner);
}

void CadRibbon::toggleRibbonCollapse() {
    m_isCollapsed = !m_isCollapsed;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        QWidget* page = m_tabWidget->widget(i);
        if (page) {
            page->setVisible(!m_isCollapsed);
        }
    }
    if (m_isCollapsed) {
        setFixedHeight(m_tabWidget->tabBar()->height() + 4);
    } else {
        setFixedHeight(m_expandedHeight);
    }
}

QToolButton* CadRibbon::createLargeRibbonButton(QAction* action) {
    auto* btn = new QToolButton(this);
    btn->setDefaultAction(action);
    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btn->setIconSize(QSize(28, 28));
    btn->setFixedSize(54, 76);
    return btn;
}

QToolButton* CadRibbon::createSmallRibbonButton(QAction* action) {
    auto* btn = new QToolButton(this);
    btn->setDefaultAction(action);
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btn->setIconSize(QSize(16, 16));
    btn->setFixedHeight(24);
    btn->setMinimumWidth(80);
    return btn;
}

QWidget* CadRibbon::createRibbonGroup(const QString& title, QWidget* content) {
    auto* group = new QWidget(this);
    auto* vlayout = new QVBoxLayout(group);
    vlayout->setContentsMargins(4, 2, 4, 2);
    vlayout->setSpacing(2);

    content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    vlayout->addWidget(content, 1);

    auto* lblTitle = new QLabel(title, group);
    lblTitle->setAlignment(Qt::AlignCenter);
    lblTitle->setStyleSheet(QStringLiteral("font-size: 10px; color: #828997; padding-top: 2px; border-top: 1px solid #313640;"));
    vlayout->addWidget(lblTitle, 0);

    group->setStyleSheet(QStringLiteral(
        "QWidget { background-color: #21252b; border-right: 1px solid #181a1f; }"
    ));

    return group;
}

QWidget* CadRibbon::createHomeTab() {
    auto* tab = new QWidget(this);
    auto* hlayout = new QHBoxLayout(tab);
    hlayout->setContentsMargins(4, 2, 4, 2);
    hlayout->setSpacing(2);

    // 0. Project / File Group
    auto* projectContent = new QWidget(tab);
    auto* projGrid = new QGridLayout(projectContent);
    projGrid->setContentsMargins(0, 0, 0, 0);
    projGrid->setSpacing(2);
    projGrid->addWidget(createSmallRibbonButton(m_actNew), 0, 0);
    projGrid->addWidget(createSmallRibbonButton(m_actOpen), 0, 1);
    projGrid->addWidget(createSmallRibbonButton(m_actSave), 1, 0);
    projGrid->addWidget(createSmallRibbonButton(m_actExport), 1, 1);
    hlayout->addWidget(createRibbonGroup(tr("Project"), projectContent));

    // 1. Draw Group
    auto* drawContent = new QWidget(tab);
    auto* drawLayout = new QHBoxLayout(drawContent);
    drawLayout->setContentsMargins(0, 0, 0, 0);
    drawLayout->setSpacing(3);

    drawLayout->addWidget(createLargeRibbonButton(m_actLine));
    drawLayout->addWidget(createLargeRibbonButton(m_actPolyline));

    auto* drawGrid = new QWidget(drawContent);
    auto* glayout = new QGridLayout(drawGrid);
    glayout->setContentsMargins(0, 0, 0, 0);
    glayout->setSpacing(2);
    glayout->addWidget(createSmallRibbonButton(m_actCircle), 0, 0);
    glayout->addWidget(createSmallRibbonButton(m_actArc), 0, 1);
    glayout->addWidget(createSmallRibbonButton(m_actRectangle), 1, 0);
    glayout->addWidget(createSmallRibbonButton(m_actPolygon), 1, 1);
    glayout->addWidget(createSmallRibbonButton(m_actEllipse), 2, 0);
    glayout->addWidget(createSmallRibbonButton(m_actSelect), 2, 1);
    drawLayout->addWidget(drawGrid);

    hlayout->addWidget(createRibbonGroup(tr("Draw"), drawContent));

    // 2. Modify Group
    auto* modContent = new QWidget(tab);
    auto* modGrid = new QGridLayout(modContent);
    modGrid->setContentsMargins(0, 0, 0, 0);
    modGrid->setSpacing(2);
    modGrid->addWidget(createSmallRibbonButton(m_actMove), 0, 0);
    modGrid->addWidget(createSmallRibbonButton(m_actCopy), 0, 1);
    modGrid->addWidget(createSmallRibbonButton(m_actRotate), 1, 0);
    modGrid->addWidget(createSmallRibbonButton(m_actMirror), 1, 1);
    modGrid->addWidget(createSmallRibbonButton(m_actTrim), 2, 0);
    modGrid->addWidget(createSmallRibbonButton(m_actExtend), 2, 1);
    modGrid->addWidget(createSmallRibbonButton(m_actOffset), 0, 2);
    modGrid->addWidget(createSmallRibbonButton(m_actDelete), 1, 2);

    hlayout->addWidget(createRibbonGroup(tr("Modify"), modContent));

    // 3. Annotation Group
    auto* annotContent = new QWidget(tab);
    auto* annotLayout = new QVBoxLayout(annotContent);
    annotLayout->setContentsMargins(0, 0, 0, 0);
    annotLayout->setSpacing(2);
    annotLayout->addWidget(createSmallRibbonButton(m_actDimension));
    annotLayout->addWidget(createSmallRibbonButton(m_actText));
    annotLayout->addStretch();

    hlayout->addWidget(createRibbonGroup(tr("Annotation"), annotContent));

    // 4. Layers Group
    auto* layerContent = new QWidget(tab);
    auto* layerLayout = new QVBoxLayout(layerContent);
    layerLayout->setContentsMargins(2, 0, 2, 0);
    layerLayout->setSpacing(4);

    m_layerCombo = new QComboBox(layerContent);
    m_layerCombo->setMinimumWidth(130);
    m_layerCombo->setToolTip(tr("Active Drawing Layer"));
    connect(m_layerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CadRibbon::onLayerComboChanged);
    layerLayout->addWidget(m_layerCombo);

    auto* layerBtnLayout = new QHBoxLayout();
    layerBtnLayout->setSpacing(3);

    auto* btnLayerProps = new QToolButton(layerContent);
    btnLayerProps->setIcon(QIcon(QStringLiteral(":/icons/layers.svg")));
    btnLayerProps->setText(tr("Properties"));
    btnLayerProps->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btnLayerProps->setToolTip(tr("Toggle Layers Management Panel"));
    connect(btnLayerProps, &QToolButton::clicked, this, &CadRibbon::layerPropertiesRequested);
    layerBtnLayout->addWidget(btnLayerProps);

    auto* btnNewLayer = new QToolButton(layerContent);
    btnNewLayer->setText(tr("+ New"));
    btnNewLayer->setToolTip(tr("Create a new CAD layer"));
    connect(btnNewLayer, &QToolButton::clicked, this, &CadRibbon::newLayerRequested);
    layerBtnLayout->addWidget(btnNewLayer);

    layerLayout->addLayout(layerBtnLayout);
    layerLayout->addStretch();

    hlayout->addWidget(createRibbonGroup(tr("Layers"), layerContent));

    // 5. Drafting Modes
    auto* draftContent = new QWidget(tab);
    auto* draftGrid = new QGridLayout(draftContent);
    draftGrid->setContentsMargins(0, 0, 0, 0);
    draftGrid->setSpacing(2);
    draftGrid->addWidget(createSmallRibbonButton(m_actSnap), 0, 0);
    draftGrid->addWidget(createSmallRibbonButton(m_actGrid), 0, 1);
    draftGrid->addWidget(createSmallRibbonButton(m_actOrtho), 1, 0);
    draftGrid->addWidget(createSmallRibbonButton(m_actPolar), 1, 1);

    hlayout->addWidget(createRibbonGroup(tr("Snaps & Modes"), draftContent));

    // 6. Clipboard / History Group
    auto* clipContent = new QWidget(tab);
    auto* clipLayout = new QVBoxLayout(clipContent);
    clipLayout->setContentsMargins(0, 0, 0, 0);
    clipLayout->setSpacing(2);
    clipLayout->addWidget(createSmallRibbonButton(m_actUndo));
    clipLayout->addWidget(createSmallRibbonButton(m_actRedo));
    clipLayout->addStretch();

    hlayout->addWidget(createRibbonGroup(tr("Clipboard"), clipContent));

    hlayout->addStretch();
    return tab;
}

QWidget* CadRibbon::createAnnotateTab() {
    auto* tab = new QWidget(this);
    auto* hlayout = new QHBoxLayout(tab);
    hlayout->setContentsMargins(4, 2, 4, 2);
    hlayout->setSpacing(2);

    // Dimension Group
    auto* dimContent = new QWidget(tab);
    auto* dimLayout = new QHBoxLayout(dimContent);
    dimLayout->setContentsMargins(0, 0, 0, 0);
    dimLayout->setSpacing(3);
    dimLayout->addWidget(createLargeRibbonButton(m_actDimension));
    dimLayout->addWidget(createLargeRibbonButton(m_actAiDimension));
    hlayout->addWidget(createRibbonGroup(tr("Dimensions"), dimContent));

    // Text Group
    auto* textContent = new QWidget(tab);
    auto* textLayout = new QHBoxLayout(textContent);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(3);
    textLayout->addWidget(createLargeRibbonButton(m_actText));
    hlayout->addWidget(createRibbonGroup(tr("Text & Notes"), textContent));

    hlayout->addStretch();
    return tab;
}

QWidget* CadRibbon::createViewTab() {
    auto* tab = new QWidget(this);
    auto* hlayout = new QHBoxLayout(tab);
    hlayout->setContentsMargins(4, 2, 4, 2);
    hlayout->setSpacing(2);

    // Navigation Group
    auto* navContent = new QWidget(tab);
    auto* navGrid = new QGridLayout(navContent);
    navGrid->setContentsMargins(0, 0, 0, 0);
    navGrid->setSpacing(2);
    navGrid->addWidget(createSmallRibbonButton(m_actZoomFit), 0, 0);
    navGrid->addWidget(createSmallRibbonButton(m_actResetZoom), 0, 1);
    navGrid->addWidget(createSmallRibbonButton(m_actZoomIn), 1, 0);
    navGrid->addWidget(createSmallRibbonButton(m_actZoomOut), 1, 1);
    hlayout->addWidget(createRibbonGroup(tr("Navigate"), navContent));

    // Page / Document limits
    auto* pageContent = new QWidget(tab);
    auto* pageLayout = new QVBoxLayout(pageContent);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(2);
    pageLayout->addWidget(createSmallRibbonButton(m_actPageSetup));
    pageLayout->addStretch();
    hlayout->addWidget(createRibbonGroup(tr("Sheet Setup"), pageContent));

    // Palettes Group
    auto* palContent = new QWidget(tab);
    auto* palLayout = new QVBoxLayout(palContent);
    palLayout->setContentsMargins(0, 0, 0, 0);
    palLayout->setSpacing(2);

    auto* btnProps = new QToolButton(palContent);
    btnProps->setIcon(QIcon(QStringLiteral(":/icons/properties.svg")));
    btnProps->setText(tr("Properties"));
    btnProps->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btnProps->setFixedHeight(24);
    connect(btnProps, &QToolButton::clicked, this, &CadRibbon::propertiesDockRequested);
    palLayout->addWidget(btnProps);

    auto* btnCmd = new QToolButton(palContent);
    btnCmd->setText(tr("Command Line"));
    btnCmd->setFixedHeight(24);
    connect(btnCmd, &QToolButton::clicked, this, &CadRibbon::commandLineRequested);
    palLayout->addWidget(btnCmd);

    palLayout->addStretch();
    hlayout->addWidget(createRibbonGroup(tr("Palettes"), palContent));

    hlayout->addStretch();
    return tab;
}

QWidget* CadRibbon::createCloudTab() {
    auto* tab = new QWidget(this);
    auto* hlayout = new QHBoxLayout(tab);
    hlayout->setContentsMargins(4, 2, 4, 2);
    hlayout->setSpacing(2);

    auto* cloudContent = new QWidget(tab);
    auto* clayout = new QHBoxLayout(cloudContent);
    clayout->setContentsMargins(0, 0, 0, 0);
    clayout->setSpacing(3);
    clayout->addWidget(createLargeRibbonButton(m_actCloudWorkspace));
    clayout->addWidget(createLargeRibbonButton(m_actCloudUpload));
    clayout->addWidget(createLargeRibbonButton(m_actCloudDownload));

    hlayout->addWidget(createRibbonGroup(tr("Cloud Collaboration"), cloudContent));
    hlayout->addStretch();
    return tab;
}

QWidget* CadRibbon::createAiTab() {
    auto* tab = new QWidget(this);
    auto* hlayout = new QHBoxLayout(tab);
    hlayout->setContentsMargins(4, 2, 4, 2);
    hlayout->setSpacing(2);

    auto* aiContent = new QWidget(tab);
    auto* alayout = new QHBoxLayout(aiContent);
    alayout->setContentsMargins(0, 0, 0, 0);
    alayout->setSpacing(3);
    alayout->addWidget(createLargeRibbonButton(m_actAiAssistant));
    alayout->addWidget(createLargeRibbonButton(m_actAiGenerate));
    alayout->addWidget(createLargeRibbonButton(m_actAiDimension));
    alayout->addWidget(createLargeRibbonButton(m_actAiAnalyze));
    alayout->addWidget(createLargeRibbonButton(m_actAiClean));

    hlayout->addWidget(createRibbonGroup(tr("Autonomous CAD Copilot"), aiContent));
    hlayout->addStretch();
    return tab;
}

void CadRibbon::onLayerComboChanged(int index) {
    if (!m_layerCombo || index < 0) return;
    QString name = m_layerCombo->itemText(index);
    emit activeLayerChanged(name);
}
