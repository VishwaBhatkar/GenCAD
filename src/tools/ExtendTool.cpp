#include "tools/ExtendTool.h"
#include "canvas/CadView.h"
#include "document/CadDocument.h"
#include "geometry/LineEntity.h"
#include "commands/DeleteEntityCommand.h"
#include "commands/CreateEntityCommand.h"

#include <cmath>

ExtendTool::ExtendTool(CadDocument* doc, CadView* view)
    : Tool(doc, view)
{
}

void ExtendTool::activate() {
    if (m_view) {
        m_view->setCursor(Qt::CrossCursor);
    }
}

void ExtendTool::deactivate() {
    if (m_view) {
        m_view->setCursor(Qt::ArrowCursor);
    }
}

void ExtendTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
    if (!m_document || !m_view) return;

    if (event->button() != Qt::LeftButton) return;

    const double tolerance = 8.0;
    for (const auto& entity : m_document->entities()) {
        if (entity && entity->type() == EntityType::Line && entity->hitTest(worldPos, tolerance)) {
            const auto* line = static_cast<const LineEntity*>(entity.get());
            double d1 = std::hypot(worldPos.x() - line->p1().x(), worldPos.y() - line->p1().y());
            double d2 = std::hypot(worldPos.x() - line->p2().x(), worldPos.y() - line->p2().y());

            double dx = line->p2().x() - line->p1().x();
            double dy = line->p2().y() - line->p1().y();
            double len = std::hypot(dx, dy);
            if (len < 1e-4) return;

            double extLen = std::max(20.0, len * 0.25);
            double ux = dx / len;
            double uy = dy / len;

            QPointF newP1 = line->p1();
            QPointF newP2 = line->p2();

            if (d1 < d2) {
                // Extend p1 backwards
                newP1 = QPointF(line->p1().x() - ux * extLen, line->p1().y() - uy * extLen);
            } else {
                // Extend p2 forwards
                newP2 = QPointF(line->p2().x() + ux * extLen, line->p2().y() + uy * extLen);
            }

            auto newLine = std::make_shared<LineEntity>(newP1, newP2);
            newLine->setColor(line->color());
            newLine->setStrokeWidth(line->strokeWidth());
            newLine->setLayer(line->layer());

            m_document->undoStack()->beginMacro(QStringLiteral("Extend Line"));
            m_document->undoStack()->push(new DeleteEntityCommand(m_document, entity));
            m_document->undoStack()->push(new CreateEntityCommand(m_document, newLine));
            m_document->undoStack()->endMacro();
            break;
        }
    }
}

void ExtendTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        deactivate();
    }
}
