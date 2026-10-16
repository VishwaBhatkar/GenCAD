#include "tools/LineTool.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "document/CoordinateSystem.h"
#include "geometry/LineEntity.h"
#include "geometry/DimensionEntity.h"
#include "commands/CreateEntityCommand.h"
#include "ui/CadCommandLine.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LineTool::LineTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine)
    : Tool(doc, view)
    , m_cmdLine(cmdLine)
{
}

void LineTool::activate() {
    m_state = State::WaitingForFirstPoint;
    if (m_view) {
        m_view->setCursor(Qt::CrossCursor);
    }
    if (m_cmdLine) {
        m_cmdLine->setPrompt(QObject::tr("LINE Specify first point:"));
        m_cmdLine->showFields(QObject::tr("X:"), QObject::tr("Y:"));
    }
}

void LineTool::deactivate() {
    cancel();
}

void LineTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
    if (!m_document || !m_view) return;

    if (event->button() == Qt::RightButton) {
        cancel();
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    const auto& units = m_document->unitSettings();

    if (m_state == State::WaitingForFirstPoint) {
        m_p1 = worldPos;
        m_state = State::WaitingForSecondPoint;

        if (m_view->snapManager()) {
            m_view->snapManager()->setBasePoint(m_p1, true);
        }

        if (m_cmdLine) {
            QString prompt = QStringLiteral("LINE Specify next point: Start X: %1  Y: %2")
                .arg(units.formatLinear(m_p1.x()))
                .arg(units.formatLinear(m_p1.y()));
            m_cmdLine->setPrompt(prompt);
            m_cmdLine->showFields(QObject::tr("Length:"), QObject::tr("Angle:"));
        }

        auto preview = std::make_shared<LineEntity>(m_p1, worldPos);
        if (m_view->cadScene()) {
            m_view->cadScene()->setPreviewEntity(preview);
        }
    } else if (m_state == State::WaitingForSecondPoint) {
        QPointF p2 = worldPos;

        double dx = p2.x() - m_p1.x();
        double dy = p2.y() - m_p1.y();
        double len = std::hypot(dx, dy);
        if (len > 1e-4) {
            auto line = std::make_shared<LineEntity>(m_p1, p2);
            auto* parentCmd = new QUndoCommand(QStringLiteral("Create Line"));
            new CreateEntityCommand(m_document, line, parentCmd);

            QPointF normal(-dy / len, dx / len);
            QPointF dimPt = (m_p1 + p2) * 0.5 + normal * 15.0;
            DimensionType dimType = DimensionType::Aligned;
            if (m_view && m_view->isOrthoEnabled()) {
                dimType = std::abs(dx) >= std::abs(dy) ? DimensionType::Horizontal : DimensionType::Vertical;
            }
            auto dim = std::make_shared<DimensionEntity>(m_p1, p2, dimPt, dimType);
            dim->setTargetEntityId(line->id());
            dim->setTargetFeatureIndex(0);
            dim->setUnitSuffix(units.unitSuffix());
            dim->setPrecision(units.precision());
            new CreateEntityCommand(m_document, dim, parentCmd);

            m_document->undoStack()->push(parentCmd);
        }

        cancel();
    }
}

void LineTool::mouseMove(QMouseEvent* /*event*/, const QPointF& worldPos) {
    if (m_state == State::WaitingForFirstPoint) {
        if (m_cmdLine) {
            const auto& units = m_document->unitSettings();
            m_cmdLine->updateFieldValues(worldPos.x() * units.scaleFromMm(),
                                         worldPos.y() * units.scaleFromMm(),
                                         0.0, units.precision());
        }
    } else if (m_state == State::WaitingForSecondPoint && m_view && m_view->cadScene()) {
        auto preview = std::make_shared<LineEntity>(m_p1, worldPos);
        m_view->cadScene()->setPreviewEntity(preview);

        const auto& units = m_document->unitSettings();
        double dx = worldPos.x() - m_p1.x();
        double dy = worldPos.y() - m_p1.y();
        bool isOrtho = (m_view && m_view->isOrthoEnabled());

        double length = 0.0;
        double angle = 0.0;
        QString hudText;

        if (isOrtho) {
            bool isH = CoordinateSystem::isOrthoHorizontal(m_p1, worldPos);
            angle = CoordinateSystem::orthoAngleDeg(m_p1, worldPos);
            length = isH ? std::abs(dx) : std::abs(dy);
            hudText = QStringLiteral("ORTHO\nDirection: %1\nAngle: %2°\nLength: %3")
                .arg(isH ? QStringLiteral("Horizontal") : QStringLiteral("Vertical"))
                .arg(angle, 0, 'f', 0)
                .arg(units.formatLinear(length));
        } else {
            length = std::hypot(dx, dy);
            angle = CoordinateSystem::angleDeg(m_p1, worldPos);
            hudText = QStringLiteral("Length: %1  Angle: %2\nΔX: %3  ΔY: %4")
                .arg(units.formatLinear(length))
                .arg(units.formatAngle(angle))
                .arg(units.formatLinear(std::abs(dx)))
                .arg(units.formatLinear(std::abs(dy)));
        }

        m_view->cadScene()->setDynamicDimension(hudText, worldPos);

        if (m_cmdLine) {
            double displayLen = length * units.scaleFromMm();
            m_cmdLine->updateFieldValues(displayLen, angle, 0.0, units.precision());
        }
    }
}

void LineTool::commitExact(double length, double angleDeg) {
    if (!m_document) return;

    if (m_state == State::WaitingForFirstPoint) {
        commitExactCoords(length, angleDeg);
        return;
    }

    const auto& units = m_document->unitSettings();
    double lenMm = length * units.scaleToMm();

    if (lenMm <= 1e-4 || std::isnan(lenMm) || std::isinf(lenMm) || std::isnan(angleDeg) || std::isinf(angleDeg)) {
        if (m_cmdLine) {
            m_cmdLine->setPrompt(QObject::tr("Invalid dimension. Please enter a value greater than 0."));
        }
        return;
    }

    double finalAngle = CoordinateSystem::normalizeAngleDeg(angleDeg);
    bool isOrtho = (m_view && m_view->isOrthoEnabled());
    if (isOrtho) {
        finalAngle = CoordinateSystem::constrainToOrthoAngle(finalAngle);
    }

    QPointF p2 = CoordinateSystem::polarPoint(m_p1, lenMm, finalAngle);
    auto line = std::make_shared<LineEntity>(m_p1, p2);
    auto* parentCmd = new QUndoCommand(QStringLiteral("Create Line"));
    new CreateEntityCommand(m_document, line, parentCmd);

    double dx = p2.x() - m_p1.x();
    double dy = p2.y() - m_p1.y();
    double l = std::hypot(dx, dy);
    if (l > 1e-4) {
        QPointF normal(-dy / l, dx / l);
        QPointF dimPt = (m_p1 + p2) * 0.5 + normal * 15.0;
        DimensionType dimType = DimensionType::Aligned;
        if (isOrtho) {
            dimType = std::abs(dx) >= std::abs(dy) ? DimensionType::Horizontal : DimensionType::Vertical;
        }
        auto dim = std::make_shared<DimensionEntity>(m_p1, p2, dimPt, dimType);
        dim->setTargetEntityId(line->id());
        dim->setTargetFeatureIndex(0);
        dim->setUnitSuffix(units.unitSuffix());
        dim->setPrecision(units.precision());
        new CreateEntityCommand(m_document, dim, parentCmd);
    }

    m_document->undoStack()->push(parentCmd);
    cancel();
}

void LineTool::commitExactCoords(double x, double y) {
    if (!m_document) return;
    const auto& units = m_document->unitSettings();
    QPointF pt(x * units.scaleToMm(), y * units.scaleToMm());

    if (m_state == State::WaitingForFirstPoint) {
        m_p1 = pt;
        m_state = State::WaitingForSecondPoint;
        if (m_view && m_view->snapManager()) {
            m_view->snapManager()->setBasePoint(m_p1, true);
        }
        if (m_cmdLine) {
            QString prompt = QStringLiteral("LINE Specify next point: Start X: %1  Y: %2")
                .arg(units.formatLinear(m_p1.x()))
                .arg(units.formatLinear(m_p1.y()));
            m_cmdLine->setPrompt(prompt);
            m_cmdLine->showFields(QObject::tr("Length:"), QObject::tr("Angle:"));
        }
    } else if (m_state == State::WaitingForSecondPoint) {
        if (std::hypot(pt.x() - m_p1.x(), pt.y() - m_p1.y()) > 1e-4) {
            auto line = std::make_shared<LineEntity>(m_p1, pt);
            m_document->undoStack()->push(new CreateEntityCommand(m_document, line));
        }
        cancel();
    }
}

void LineTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        cancel();
    }
}

void LineTool::cancel() {
    m_state = State::WaitingForFirstPoint;
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
