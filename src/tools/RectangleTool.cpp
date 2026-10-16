#include "tools/RectangleTool.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "geometry/RectangleEntity.h"
#include "geometry/DimensionEntity.h"
#include "commands/CreateEntityCommand.h"
#include "ui/CadCommandLine.h"

#include <cmath>

RectangleTool::RectangleTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine)
    : Tool(doc, view)
    , m_cmdLine(cmdLine)
{
}

void RectangleTool::activate() {
    m_state = State::WaitingForFirstCorner;
    if (m_view) {
        m_view->setCursor(Qt::CrossCursor);
    }
    if (m_cmdLine) {
        m_cmdLine->setPrompt(QObject::tr("RECTANGLE Specify first corner:"));
        m_cmdLine->showFields(QObject::tr("X:"), QObject::tr("Y:"));
    }
}

void RectangleTool::deactivate() {
    cancel();
}

void RectangleTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
    if (!m_document || !m_view) return;

    if (event->button() == Qt::RightButton) {
        cancel();
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    const auto& units = m_document->unitSettings();

    if (m_state == State::WaitingForFirstCorner) {
        m_p1 = worldPos;
        m_state = State::WaitingForSecondCorner;

        if (m_view->snapManager()) {
            m_view->snapManager()->setBasePoint(m_p1, true);
        }

        if (m_cmdLine) {
            m_cmdLine->setPrompt(QObject::tr("RECTANGLE Specify other corner or dimensions:"));
            m_cmdLine->showFields(QObject::tr("Length:"), QObject::tr("Width:"));
        }

        auto preview = std::make_shared<RectangleEntity>(m_p1, worldPos);
        if (m_view->cadScene()) {
            m_view->cadScene()->setPreviewEntity(preview);
        }
    } else if (m_state == State::WaitingForSecondCorner) {
        QPointF p2 = worldPos;

        double w = std::abs(p2.x() - m_p1.x());
        double h = std::abs(p2.y() - m_p1.y());

        if (w > 1e-4 && h > 1e-4) {
            auto rect = std::make_shared<RectangleEntity>(m_p1, p2);
            auto* parentCmd = new QUndoCommand(QStringLiteral("Create Rectangle"));
            new CreateEntityCommand(m_document, rect, parentCmd);

            // Length (horizontal, along top edge)
            double minX = std::min(m_p1.x(), p2.x());
            double maxX = std::max(m_p1.x(), p2.x());
            double minY = std::min(m_p1.y(), p2.y());
            double maxY = std::max(m_p1.y(), p2.y());

            QPointF dimPtW((minX + maxX) * 0.5, minY - 15.0);
            auto dimW = std::make_shared<DimensionEntity>(
                QPointF(minX, minY), QPointF(maxX, minY), dimPtW, DimensionType::Horizontal);
            dimW->setTargetEntityId(rect->id());
            dimW->setTargetFeatureIndex(0);
            dimW->setUnitSuffix(units.unitSuffix());
            dimW->setPrecision(units.precision());
            new CreateEntityCommand(m_document, dimW, parentCmd);

            // Width / Height (vertical, along right edge)
            QPointF dimPtH(maxX + 15.0, (minY + maxY) * 0.5);
            auto dimH = std::make_shared<DimensionEntity>(
                QPointF(maxX, minY), QPointF(maxX, maxY), dimPtH, DimensionType::Vertical);
            dimH->setTargetEntityId(rect->id());
            dimH->setTargetFeatureIndex(1);
            dimH->setUnitSuffix(units.unitSuffix());
            dimH->setPrecision(units.precision());
            new CreateEntityCommand(m_document, dimH, parentCmd);

            m_document->undoStack()->push(parentCmd);
        }

        cancel();
    }
}

void RectangleTool::mouseMove(QMouseEvent* /*event*/, const QPointF& worldPos) {
    if (m_state == State::WaitingForFirstCorner) {
        if (m_cmdLine) {
            const auto& units = m_document->unitSettings();
            m_cmdLine->updateFieldValues(worldPos.x() * units.scaleFromMm(),
                                         worldPos.y() * units.scaleFromMm(),
                                         0.0, units.precision());
        }
    } else if (m_state == State::WaitingForSecondCorner && m_view && m_view->cadScene()) {
        auto preview = std::make_shared<RectangleEntity>(m_p1, worldPos);
        m_view->cadScene()->setPreviewEntity(preview);

        const auto& units = m_document->unitSettings();
        double w = std::abs(worldPos.x() - m_p1.x());
        double h = std::abs(worldPos.y() - m_p1.y());

        bool isOrtho = (m_view && m_view->isOrthoEnabled());
        QString hudText;
        if (isOrtho) {
            hudText = QStringLiteral("ORTHO RECTANGLE\nLength: %1\nWidth: %2")
                .arg(units.formatLinear(w))
                .arg(units.formatLinear(h));
        } else {
            hudText = QStringLiteral("Length: %1\nWidth: %2")
                .arg(units.formatLinear(w))
                .arg(units.formatLinear(h));
        }
        m_view->cadScene()->setDynamicDimension(hudText, worldPos);

        if (m_cmdLine) {
            m_cmdLine->updateFieldValues(w * units.scaleFromMm(),
                                         h * units.scaleFromMm(),
                                         0.0, units.precision());
        }
    }
}

void RectangleTool::commitExact(double width, double height) {
    if (!m_document) return;
    const auto& units = m_document->unitSettings();

    if (m_state == State::WaitingForFirstCorner) {
        m_p1 = QPointF(width * units.scaleToMm(), height * units.scaleToMm());
        m_state = State::WaitingForSecondCorner;
        if (m_view && m_view->snapManager()) {
            m_view->snapManager()->setBasePoint(m_p1, true);
        }
        if (m_cmdLine) {
            m_cmdLine->setPrompt(QObject::tr("RECTANGLE Specify other corner or dimensions:"));
            m_cmdLine->showFields(QObject::tr("Length:"), QObject::tr("Width:"));
        }
        return;
    }

    double wMm = width * units.scaleToMm();
    double hMm = height * units.scaleToMm();

    if (wMm <= 1e-4 || hMm <= 1e-4 || std::isnan(wMm) || std::isnan(hMm) || std::isinf(wMm) || std::isinf(hMm)) {
        if (m_cmdLine) {
            m_cmdLine->setPrompt(QObject::tr("Invalid dimension. Please enter values greater than 0."));
        }
        return;
    }

    QPointF p2(m_p1.x() + wMm, m_p1.y() + hMm);
    auto rect = std::make_shared<RectangleEntity>(m_p1, p2);
    auto* parentCmd = new QUndoCommand(QStringLiteral("Create Rectangle"));
    new CreateEntityCommand(m_document, rect, parentCmd);

    double minX = std::min(m_p1.x(), p2.x());
    double maxX = std::max(m_p1.x(), p2.x());
    double minY = std::min(m_p1.y(), p2.y());
    double maxY = std::max(m_p1.y(), p2.y());

    QPointF dimPtW((minX + maxX) * 0.5, minY - 15.0);
    auto dimW = std::make_shared<DimensionEntity>(
        QPointF(minX, minY), QPointF(maxX, minY), dimPtW, DimensionType::Horizontal);
    dimW->setTargetEntityId(rect->id());
    dimW->setTargetFeatureIndex(0);
    dimW->setUnitSuffix(units.unitSuffix());
    dimW->setPrecision(units.precision());
    new CreateEntityCommand(m_document, dimW, parentCmd);

    QPointF dimPtH(maxX + 15.0, (minY + maxY) * 0.5);
    auto dimH = std::make_shared<DimensionEntity>(
        QPointF(maxX, minY), QPointF(maxX, maxY), dimPtH, DimensionType::Vertical);
    dimH->setTargetEntityId(rect->id());
    dimH->setTargetFeatureIndex(1);
    dimH->setUnitSuffix(units.unitSuffix());
    dimH->setPrecision(units.precision());
    new CreateEntityCommand(m_document, dimH, parentCmd);

    m_document->undoStack()->push(parentCmd);
    cancel();
}

void RectangleTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        cancel();
    }
}

void RectangleTool::cancel() {
    m_state = State::WaitingForFirstCorner;
    if (m_view) {
        if (m_view->cadScene()) {
            m_view->cadScene()->clearPreview();
            m_view->cadScene()->clearDynamicDimension();
        }
        if (m_view->snapManager()) {
            m_view->snapManager()->clearBasePoint();
        }
    }
    if (m_cmdLine) {
        m_cmdLine->clearPrompt();
    }
}
