#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "snap/SnapManager.h"
#include "document/CoordinateSystem.h"
#include "tools/Tool.h"

#include <QScrollBar>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFontMetrics>
#include <QMenu>
#include <algorithm>
#include <cmath>

CadView::CadView(CadScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , m_scene(scene)
{
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setDragMode(QGraphicsView::NoDrag);
    setMouseTracking(true);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(QFrame::NoFrame);
    setFocusPolicy(Qt::StrongFocus);
    viewport()->setMouseTracking(true);
}

CadView::~CadView() = default;

void CadView::setThemeMode(CadTheme::ThemeMode mode) {
    m_themeMode = mode;
    if (m_scene) {
        m_scene->setThemeMode(mode);
    }
    viewport()->update();
}

void CadView::setTool(Tool* tool) {
    if (m_currentTool) {
        m_currentTool->deactivate();
    }
    m_currentTool = tool;
    if (m_currentTool) {
        m_currentTool->activate();
    }
    viewport()->update();
}

void CadView::setSnapManager(SnapManager* snapManager) {
    m_snapManager = snapManager;
}

void CadView::setOrthoEnabled(bool enabled) {
    if (m_orthoEnabled != enabled) {
        m_orthoEnabled = enabled;
        emit orthoToggled(m_orthoEnabled);
        viewport()->update();
    }
}

void CadView::setCrosshairVisible(bool visible) {
    m_crosshairVisible = visible;
    viewport()->update();
}

void CadView::zoomIn() {
    scale(1.25, 1.25);
    viewport()->update();
}

void CadView::zoomOut() {
    scale(0.8, 0.8);
    viewport()->update();
}

void CadView::resetZoom() {
    resetTransform();
    viewport()->update();
}

void CadView::zoomFit() {
    if (!m_scene || !m_scene->document()) return;

    QRectF box = m_scene->document()->boundingBox();
    if (m_scene->document()->entityCount() == 0) {
        box = QRectF(-100, -100, 200, 200);
    }

    qreal margin = std::max(20.0, std::max(box.width(), box.height()) * 0.1);
    QRectF fitBox = box.adjusted(-margin, -margin, margin, margin);
    fitInView(fitBox, Qt::KeepAspectRatio);
    viewport()->update();
}

QPointF CadView::screenToWorld(const QPoint& screenPt) const {
    return mapToScene(screenPt);
}

QPoint CadView::worldToScreen(const QPointF& worldPt) const {
    return mapFromScene(worldPt);
}

QPointF CadView::getSnappedPosition(const QPointF& rawPos) {
    if (!m_scene || !m_scene->document()) {
        return rawPos;
    }

    QPointF pos = rawPos;
    SnapResult snap;
    bool objectSnapped = false;

    double zoom = transform().m11();
    if (zoom < 1e-4) zoom = 1.0;
    double snapRadius = m_snapManager ? (m_snapManager->snapTolerance() / zoom) : 10.0;

    bool isDirectionalTool = (m_currentTool && m_currentTool->isDirectionalOrthoConstrained());
    bool hasBase = (m_snapManager && m_snapManager->hasBasePoint());
    QPointF base = hasBase ? m_snapManager->basePoint() : QPointF(0.0, 0.0);

    // 1. Object Snap Resolution
    if (m_snapManager && m_snapManager->isSnapEnabled()) {
        snap = m_snapManager->findSnap(rawPos, m_scene->document(), snapRadius);
        if (snap.snapped && snap.type != SnapType::Polar && snap.type != SnapType::Grid) {
            objectSnapped = true;
            pos = snap.point;
        }
    }

    // 2. ORTHO / Polar Direction Constraint
    if (m_orthoEnabled && isDirectionalTool && hasBase) {
        pos = CoordinateSystem::snapToOrtho(base, pos);
        if (snap.hasPolarLine) {
            snap.hasPolarLine = false;
        }
    } else if (m_snapManager && m_snapManager->isPolarEnabled() && isDirectionalTool && hasBase && !objectSnapped) {
        bool polarSnapped = false;
        QPointF polarPt = CoordinateSystem::snapToPolar(base, rawPos, m_snapManager->polarAngleIncrement(), 3.5, &polarSnapped);
        if (polarSnapped) {
            pos = polarPt;
            snap.snapped = true;
            snap.point = polarPt;
            snap.type = SnapType::Polar;
            snap.hasPolarLine = true;
            snap.polarStart = base;
            snap.polarEnd = polarPt;
            snap.polarAngleDeg = CoordinateSystem::angleDeg(base, polarPt);
            snap.description = QStringLiteral("Polar %1°").arg(snap.polarAngleDeg, 0, 'f', 1);
        }
    }

    // 3. Grid Snap (works simultaneously with ORTHO)
    if (m_snapManager && m_snapManager->isGridSnapEnabled() && !objectSnapped && m_snapManager->gridSize() > 0.01) {
        double gSize = m_snapManager->gridSize();
        if (m_orthoEnabled && isDirectionalTool && hasBase) {
            double dx = pos.x() - base.x();
            double dy = pos.y() - base.y();
            if (std::abs(dx) >= std::abs(dy)) {
                pos.setX(std::round(pos.x() / gSize) * gSize);
                pos.setY(base.y());
            } else {
                pos.setX(base.x());
                pos.setY(std::round(pos.y() / gSize) * gSize);
            }
        } else {
            pos.setX(std::round(pos.x() / gSize) * gSize);
            pos.setY(std::round(pos.y() / gSize) * gSize);
        }

        if (!objectSnapped) {
            snap.snapped = true;
            snap.point = pos;
            snap.type = SnapType::Grid;
            snap.description = QStringLiteral("Grid (%1, %2)").arg(pos.x(), 0, 'f', 1).arg(pos.y(), 0, 'f', 1);
        }
    }

    if (snap.snapped) {
        m_scene->setSnapResult(snap);
    } else {
        m_scene->clearSnapResult();
    }

    return pos;
}

void CadView::enterEvent(QEnterEvent* /*event*/) {
    m_mouseInsideViewport = true;
    if (m_crosshairVisible && !m_isPanning) {
        setCursor(Qt::BlankCursor);
    }
    viewport()->update();
}

void CadView::leaveEvent(QEvent* /*event*/) {
    m_mouseInsideViewport = false;
    setCursor(Qt::ArrowCursor);
    viewport()->update();
}

void CadView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // Check if clicked navigation overlay or space tabs
        int vw = viewport()->width();
        int vh = viewport()->height();

        // Check Top-Right Nav Bar click (vw - 190 to vw - 10, y: 10 to 36)
        QRect navRect(vw - 195, 8, 185, 28);
        if (navRect.contains(event->pos())) {
            handleNavOverlayClick(event->pos());
            event->accept();
            return;
        }

        // Check Bottom-Left Model/Layout tabs click (x: 10 to 140, vh - 26 to vh - 4)
        QRect modelTabRect(8, vh - 24, 60, 20);
        QRect layoutTabRect(72, vh - 24, 65, 20);
        if (modelTabRect.contains(event->pos())) {
            m_activeSpaceTab = 0;
            if (m_scene) m_scene->setPageBorderVisible(false);
            viewport()->update();
            event->accept();
            return;
        } else if (layoutTabRect.contains(event->pos())) {
            m_activeSpaceTab = 1;
            if (m_scene) m_scene->setPageBorderVisible(true);
            zoomFit();
            viewport()->update();
            event->accept();
            return;
        }
    }

    QPointF rawPos = mapToScene(event->pos());
    QPointF worldPos = getSnappedPosition(rawPos);

    if (event->button() == Qt::MiddleButton || (m_spacePressed && event->button() == Qt::LeftButton)) {
        m_isPanning = true;
        m_panStartPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (m_currentTool) {
        m_currentTool->mousePress(event, worldPos);
    }

    QGraphicsView::mousePressEvent(event);
    viewport()->update();
}

void CadView::mouseMoveEvent(QMouseEvent* event) {
    m_mouseScreenPos = event->pos();

    QPointF rawPos = mapToScene(event->pos());
    QPointF worldPos = getSnappedPosition(rawPos);

    emit mouseCoordinatesChanged(worldPos);

    if (m_isPanning) {
        QPoint delta = event->pos() - m_panStartPoint;
        m_panStartPoint = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        viewport()->update();
        return;
    }

    if (m_currentTool) {
        m_currentTool->mouseMove(event, worldPos);
    }

    QGraphicsView::mouseMoveEvent(event);
    viewport()->update();
}

void CadView::mouseReleaseEvent(QMouseEvent* event) {
    QPointF rawPos = mapToScene(event->pos());
    QPointF worldPos = getSnappedPosition(rawPos);

    if (m_isPanning && (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton)) {
        m_isPanning = false;
        if (m_crosshairVisible && m_mouseInsideViewport) {
            setCursor(m_spacePressed ? Qt::OpenHandCursor : Qt::BlankCursor);
        } else {
            setCursor(m_spacePressed ? Qt::OpenHandCursor : Qt::ArrowCursor);
        }
        event->accept();
        viewport()->update();
        return;
    }

    if (m_currentTool) {
        m_currentTool->mouseRelease(event, worldPos);
    }

    QGraphicsView::mouseReleaseEvent(event);
    viewport()->update();
}

void CadView::wheelEvent(QWheelEvent* event) {
    const double factor = 1.15;
    if (event->angleDelta().y() > 0) {
        scale(factor, factor);
    } else {
        scale(1.0 / factor, 1.0 / factor);
    }
    event->accept();

    QPointF worldPos = mapToScene(event->position().toPoint());
    emit mouseCoordinatesChanged(worldPos);
    viewport()->update();
}

void CadView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = true;
        setCursor(Qt::OpenHandCursor);
    } else if (event->key() == Qt::Key_F3) {
        if (m_snapManager) {
            m_snapManager->setSnapEnabled(!m_snapManager->isSnapEnabled());
            emit snapToggled(m_snapManager->isSnapEnabled());
            if (!m_snapManager->isSnapEnabled() && m_scene) {
                m_scene->clearSnapResult();
            }
        }
    } else if (event->key() == Qt::Key_F8) {
        setOrthoEnabled(!m_orthoEnabled);
    } else if (event->key() == Qt::Key_F9) {
        if (m_snapManager) {
            m_snapManager->setGridSnapEnabled(!m_snapManager->isGridSnapEnabled());
            emit gridSnapToggled(m_snapManager->isGridSnapEnabled());
        }
    } else if (event->key() == Qt::Key_F10) {
        if (m_snapManager) {
            m_snapManager->setPolarEnabled(!m_snapManager->isPolarEnabled());
            emit polarToggled(m_snapManager->isPolarEnabled());
        }
    }

    if (m_currentTool) {
        m_currentTool->keyPress(event);
    }

    QGraphicsView::keyPressEvent(event);
    viewport()->update();
}

void CadView::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = false;
        if (!m_isPanning) {
            setCursor(m_crosshairVisible && m_mouseInsideViewport ? Qt::BlankCursor : Qt::ArrowCursor);
        }
    }

    QGraphicsView::keyReleaseEvent(event);
    viewport()->update();
}

void CadView::paintEvent(QPaintEvent* event) {
    QGraphicsView::paintEvent(event);

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1. Draw AutoCAD Crosshair cursor
    if (m_mouseInsideViewport && m_crosshairVisible && !m_isPanning) {
        drawCrosshair(&painter);
    }

    // 2. Draw HUD Viewport Overlays (Navigation bar, space tabs, view indicator)
    drawViewportOverlays(&painter);
}

void CadView::drawCrosshair(QPainter* painter) {
    int vw = viewport()->width();
    int vh = viewport()->height();
    int cx = m_mouseScreenPos.x();
    int cy = m_mouseScreenPos.y();

    QColor chColor = CadTheme::crosshairColor(m_themeMode);
    QPen chPen(chColor, 1.0, Qt::SolidLine);
    chPen.setCosmetic(true);
    painter->setPen(chPen);

    // Full viewport infinite crosshair lines
    painter->drawLine(0, cy, vw, cy);
    painter->drawLine(cx, 0, cx, vh);

    // Pick box in center (e.g. 7x7 pixel square)
    int pb = 4;
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRect(cx - pb, cy - pb, pb * 2, pb * 2));
}

void CadView::drawViewportOverlays(QPainter* painter) {
    int vw = viewport()->width();
    int vh = viewport()->height();

    // 1. Top-Left: View Style Badge "[-][Top][2D Wireframe]"
    QFont font = painter->font();
    font.setPixelSize(10);
    painter->setFont(font);

    QRect styleBadge(8, 8, 140, 22);
    painter->setPen(QPen(QColor(60, 68, 80, 140), 1));
    painter->setBrush(QColor(24, 28, 36, 180));
    painter->drawRoundedRect(styleBadge, 3, 3);

    painter->setPen(QColor(160, 175, 200));
    painter->drawText(styleBadge, Qt::AlignCenter, QStringLiteral("[-][Top][2D Wireframe]"));

    // 2. Top-Right: Mini Navigation Bar [Extents] [+] [-] [1:1] [Pan]
    QRect navRect(vw - 195, 8, 185, 24);
    painter->setPen(QPen(QColor(60, 68, 80, 140), 1));
    painter->setBrush(QColor(24, 28, 36, 200));
    painter->drawRoundedRect(navRect, 3, 3);

    painter->setPen(QColor(220, 230, 245));
    QRect btn1(vw - 195, 8, 55, 24);
    QRect btn2(vw - 140, 8, 25, 24);
    QRect btn3(vw - 115, 8, 25, 24);
    QRect btn4(vw - 90, 8, 35, 24);
    QRect btn5(vw - 55, 8, 45, 24);

    painter->drawText(btn1, Qt::AlignCenter, QStringLiteral("Extents"));
    painter->drawText(btn2, Qt::AlignCenter, QStringLiteral("+"));
    painter->drawText(btn3, Qt::AlignCenter, QStringLiteral("-"));
    painter->drawText(btn4, Qt::AlignCenter, QStringLiteral("1:1"));
    painter->drawText(btn5, Qt::AlignCenter, QStringLiteral("Pan"));

    painter->setPen(QColor(60, 68, 80, 100));
    painter->drawLine(vw - 140, 12, vw - 140, 28);
    painter->drawLine(vw - 115, 12, vw - 115, 28);
    painter->drawLine(vw - 90, 12, vw - 90, 28);
    painter->drawLine(vw - 55, 12, vw - 55, 28);

    // 3. Bottom-Left: Model / Layout Space Tabs
    QRect modelTab(8, vh - 24, 60, 20);
    QRect layoutTab(72, vh - 24, 65, 20);

    // Model Tab
    painter->setPen(m_activeSpaceTab == 0 ? QPen(QColor(0, 122, 204), 1) : QPen(QColor(60, 68, 80, 100), 1));
    painter->setBrush(m_activeSpaceTab == 0 ? QColor(0, 122, 204, 220) : QColor(30, 34, 42, 200));
    painter->drawRoundedRect(modelTab, 2, 2);
    painter->setPen(m_activeSpaceTab == 0 ? Qt::white : QColor(140, 150, 165));
    painter->drawText(modelTab, Qt::AlignCenter, QStringLiteral("Model"));

    // Layout Tab
    painter->setPen(m_activeSpaceTab == 1 ? QPen(QColor(0, 122, 204), 1) : QPen(QColor(60, 68, 80, 100), 1));
    painter->setBrush(m_activeSpaceTab == 1 ? QColor(0, 122, 204, 220) : QColor(30, 34, 42, 200));
    painter->drawRoundedRect(layoutTab, 2, 2);
    painter->setPen(m_activeSpaceTab == 1 ? Qt::white : QColor(140, 150, 165));
    painter->drawText(layoutTab, Qt::AlignCenter, QStringLiteral("Layout 1"));
}

void CadView::handleNavOverlayClick(const QPoint& pt) {
    int vw = viewport()->width();
    QRect btn1(vw - 195, 8, 55, 24);
    QRect btn2(vw - 140, 8, 25, 24);
    QRect btn3(vw - 115, 8, 25, 24);
    QRect btn4(vw - 90, 8, 35, 24);
    QRect btn5(vw - 55, 8, 45, 24);

    if (btn1.contains(pt)) {
        zoomFit();
    } else if (btn2.contains(pt)) {
        zoomIn();
    } else if (btn3.contains(pt)) {
        zoomOut();
    } else if (btn4.contains(pt)) {
        resetZoom();
    } else if (btn5.contains(pt)) {
        // Toggle Pan mode
        setCursor(Qt::OpenHandCursor);
    }
}

void CadView::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);

    bool hasSelection = (m_scene && m_scene->document() && m_scene->document()->selectionCount() > 0);

    if (hasSelection) {
        menu.addAction(tr("Repeat Last Command"), this, [this]() {
            emit commandExecuted(QStringLiteral(""));
        });
        menu.addSeparator();

        auto* actMove = menu.addAction(QIcon(QStringLiteral(":/icons/move.svg")), tr("Move"));
        connect(actMove, &QAction::triggered, this, [this]() { emit commandExecuted(QStringLiteral("MOVE")); });

        auto* actCopy = menu.addAction(QIcon(QStringLiteral(":/icons/copy.svg")), tr("Copy"));
        connect(actCopy, &QAction::triggered, this, [this]() { emit commandExecuted(QStringLiteral("COPY")); });

        auto* actRotate = menu.addAction(QIcon(QStringLiteral(":/icons/rotate.svg")), tr("Rotate"));
        connect(actRotate, &QAction::triggered, this, [this]() { emit commandExecuted(QStringLiteral("ROTATE")); });

        auto* actMirror = menu.addAction(QIcon(QStringLiteral(":/icons/mirror.svg")), tr("Mirror"));
        connect(actMirror, &QAction::triggered, this, [this]() { emit commandExecuted(QStringLiteral("MIRROR")); });

        auto* actDelete = menu.addAction(QIcon(QStringLiteral(":/icons/delete.svg")), tr("Delete"));
        connect(actDelete, &QAction::triggered, this, [this]() { emit commandExecuted(QStringLiteral("DELETE")); });

        menu.addSeparator();
        auto* actProps = menu.addAction(QIcon(QStringLiteral(":/icons/properties.svg")), tr("Properties..."));
        connect(actProps, &QAction::triggered, this, &CadView::propertiesRequested);

        auto* actClear = menu.addAction(tr("Clear Selection (Esc)"));
        connect(actClear, &QAction::triggered, this, [this]() {
            if (m_scene && m_scene->document()) {
                m_scene->document()->clearSelection();
            }
        });
    } else {
        menu.addAction(tr("Repeat Last Command"), this, [this]() {
            emit commandExecuted(QStringLiteral(""));
        });
        menu.addSeparator();

        auto* actFit = menu.addAction(QIcon(QStringLiteral(":/icons/zoom_fit.svg")), tr("Zoom Extents"));
        connect(actFit, &QAction::triggered, this, &CadView::zoomFit);

        auto* actIn = menu.addAction(QIcon(QStringLiteral(":/icons/zoom_in.svg")), tr("Zoom In"));
        connect(actIn, &QAction::triggered, this, &CadView::zoomIn);

        auto* actOut = menu.addAction(QIcon(QStringLiteral(":/icons/zoom_out.svg")), tr("Zoom Out"));
        connect(actOut, &QAction::triggered, this, &CadView::zoomOut);

        menu.addSeparator();
        auto* actSnap = menu.addAction(QIcon(QStringLiteral(":/icons/snap.svg")), tr("Object Snap (F3)"));
        actSnap->setCheckable(true);
        actSnap->setChecked(m_snapManager ? m_snapManager->isSnapEnabled() : false);
        connect(actSnap, &QAction::triggered, this, [this]() {
            if (m_snapManager) {
                m_snapManager->setSnapEnabled(!m_snapManager->isSnapEnabled());
                emit snapToggled(m_snapManager->isSnapEnabled());
            }
        });

        auto* actGrid = menu.addAction(QIcon(QStringLiteral(":/icons/grid.svg")), tr("Grid Snap (F9)"));
        actGrid->setCheckable(true);
        actGrid->setChecked(m_snapManager ? m_snapManager->isGridSnapEnabled() : false);
        connect(actGrid, &QAction::triggered, this, [this]() {
            if (m_snapManager) {
                m_snapManager->setGridSnapEnabled(!m_snapManager->isGridSnapEnabled());
                emit gridSnapToggled(m_snapManager->isGridSnapEnabled());
            }
        });

        auto* actOrtho = menu.addAction(QIcon(QStringLiteral(":/icons/ortho.svg")), tr("ORTHO Mode (F8)"));
        actOrtho->setCheckable(true);
        actOrtho->setChecked(m_orthoEnabled);
        connect(actOrtho, &QAction::triggered, this, [this]() {
            setOrthoEnabled(!m_orthoEnabled);
        });

        menu.addSeparator();
        auto* actPage = menu.addAction(QIcon(QStringLiteral(":/icons/settings.svg")), tr("Drawing Limits / Page Setup..."));
        connect(actPage, &QAction::triggered, this, &CadView::pageSetupRequested);
    }

    menu.exec(event->globalPos());
}
