#include "ui/PropertiesDock.h"
#include "document/CadDocument.h"
#include "document/UnitSystem.h"
#include "document/PageSettings.h"
#include "layers/LayerManager.h"
#include "layers/CadLayer.h"
#include "commands/ModifyEntityPropertiesCommand.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/RectangleEntity.h"
#include "geometry/ArcEntity.h"
#include "geometry/PolylineEntity.h"
#include "geometry/PolygonEntity.h"
#include "geometry/EllipseEntity.h"
#include "geometry/TextEntity.h"
#include "geometry/DimensionEntity.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QColorDialog>
#include <QPainter>
#include <QPixmap>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PropertiesDock::PropertiesDock(CadDocument* document, QWidget* parent)
    : QDockWidget(tr("Properties"), parent)
    , m_document(document)
{
    setObjectName(QStringLiteral("PropertiesDockWidget"));
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    setupUi();

    if (m_document) {
        connect(m_document, &CadDocument::selectionChanged, this, &PropertiesDock::updateProperties);
        connect(m_document, &CadDocument::entityModified, this, [this](std::shared_ptr<CadEntity> ent) {
            if (m_currentEntity && ent && m_currentEntity->id() == ent->id()) {
                updateProperties();
            }
        });
        connect(m_document, &CadDocument::unitSettingsChanged, this, &PropertiesDock::updateProperties);
        connect(m_document, &CadDocument::pageSettingsChanged, this, &PropertiesDock::updateProperties);
    }

    updateProperties();
}

void PropertiesDock::setupUi() {
    auto* container = new QWidget(this);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    m_headerLabel = new QLabel(tr("No Selection"), container);
    m_headerLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #00e5ff; padding: 4px 6px; background-color: #282c34; border-radius: 2px;"));
    layout->addWidget(m_headerLabel);

    m_table = new QTableWidget(container);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(QStringLiteral(
        "QTableWidget { background-color: #1e1e24; color: #f0f0f0; gridline-color: #2e323b; border: 1px solid #333842; }"
        "QTableWidget::item:selected { background-color: #007acc; }"
    ));

    connect(m_table, &QTableWidget::cellChanged, this, &PropertiesDock::onPropertyItemChanged);

    layout->addWidget(m_table, 1);
    setWidget(container);
}

void PropertiesDock::addPropertyRow(const QString& category, const QString& name, const QString& value, bool editable) {
    int row = m_table->rowCount();
    m_table->insertRow(row);

    QString propTitle = category.isEmpty() ? name : QStringLiteral("[%1] %2").arg(category, name);
    auto* nameItem = new QTableWidgetItem(propTitle);
    nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    nameItem->setForeground(QColor(170, 185, 205));
    m_table->setItem(row, 0, nameItem);

    auto* valItem = new QTableWidgetItem(value);
    if (editable) {
        valItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        valItem->setForeground(QColor(255, 255, 255));
    } else {
        valItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        valItem->setForeground(QColor(215, 225, 235));
    }
    m_table->setItem(row, 1, valItem);
}

void PropertiesDock::addWidgetRow(const QString& category, const QString& name, QWidget* widget) {
    int row = m_table->rowCount();
    m_table->insertRow(row);

    QString propTitle = category.isEmpty() ? name : QStringLiteral("[%1] %2").arg(category, name);
    auto* nameItem = new QTableWidgetItem(propTitle);
    nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    nameItem->setForeground(QColor(170, 185, 205));
    m_table->setItem(row, 0, nameItem);

    m_table->setCellWidget(row, 1, widget);
}

void PropertiesDock::updateProperties() {
    if (!m_document) return;

    m_isUpdating = true;
    m_table->setRowCount(0);

    auto selected = m_document->selectedEntities();
    if (selected.empty()) {
        m_currentEntity.reset();
        m_headerLabel->setText(tr("Document Summary"));
        populateDocumentSummary();
    } else if (selected.size() == 1) {
        m_currentEntity = selected.front();
        m_headerLabel->setText(QStringLiteral("Type: %1").arg(CadEntity::entityTypeToString(m_currentEntity->type()).toUpper()));
        populateEntityProperties(m_currentEntity);
    } else {
        m_currentEntity.reset();
        m_headerLabel->setText(QStringLiteral("Multiple Selection (%1 entities)").arg(selected.size()));
        populateMultiSelectionProperties(selected);
    }

    m_isUpdating = false;
}

void PropertiesDock::populateDocumentSummary() {
    const auto& page = m_document->pageSettings();
    const auto& units = m_document->unitSettings();

    addPropertyRow(tr("Page"), tr("Format"), page.pageSizeName(), false);
    addPropertyRow(tr("Page"), tr("Orientation"), page.orientation() == PageOrientation::Portrait ? tr("Portrait") : tr("Landscape"), false);
    addPropertyRow(tr("Page"), tr("Dimensions"), page.dimensionsText(), false);
    addPropertyRow(tr("Units"), tr("Base Unit"), units.unitName(), false);
    addPropertyRow(tr("Units"), tr("Precision"), QString::number(units.precision()), false);
    addPropertyRow(tr("Stats"), tr("Total Entities"), QString::number(m_document->entityCount()), false);
    if (m_document->layerManager()) {
        addPropertyRow(tr("Stats"), tr("Total Layers"), QString::number(m_document->layerManager()->layers().size()), false);
    }
}

void PropertiesDock::populateEntityProperties(const std::shared_ptr<CadEntity>& entity) {
    if (!entity) return;

    const auto& units = m_document->unitSettings();

    // 1. General Section with interactive controls
    if (m_document->layerManager()) {
        auto* layerCombo = new QComboBox(m_table);
        const auto& layers = m_document->layerManager()->layers();
        int curIdx = 0;
        for (size_t i = 0; i < layers.size(); ++i) {
            const auto& lay = layers[i];
            QPixmap pix(12, 12);
            pix.fill(Qt::transparent);
            QPainter p(&pix);
            p.setRenderHint(QPainter::Antialiasing);
            p.setBrush(lay->color());
            p.setPen(QPen(Qt::white, 1));
            p.drawEllipse(1, 1, 10, 10);
            p.end();

            layerCombo->addItem(QIcon(pix), lay->name());
            if (lay->name() == entity->layer()) {
                curIdx = static_cast<int>(i);
            }
        }
        layerCombo->setCurrentIndex(curIdx);
        connect(layerCombo, &QComboBox::currentTextChanged, this, &PropertiesDock::onLayerComboChanged);
        addWidgetRow(tr("General"), tr("Layer"), layerCombo);
    } else {
        addPropertyRow(tr("General"), tr("Layer"), entity->layer(), false);
    }

    // Color Swatch Button
    auto* colorBtn = new QPushButton(m_table);
    QPixmap pix(14, 14);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(entity->color());
    p.setPen(QPen(Qt::white, 1));
    p.drawRoundedRect(1, 1, 12, 12, 2, 2);
    p.end();
    colorBtn->setIcon(QIcon(pix));
    colorBtn->setText(entity->color().name().toUpper());
    connect(colorBtn, &QPushButton::clicked, this, &PropertiesDock::onColorButtonClicked);
    addWidgetRow(tr("General"), tr("Color"), colorBtn);

    // Stroke width
    auto* widthSpin = new QDoubleSpinBox(m_table);
    widthSpin->setRange(0.1, 20.0);
    widthSpin->setSingleStep(0.25);
    widthSpin->setDecimals(2);
    widthSpin->setValue(entity->strokeWidth());
    widthSpin->setSuffix(QStringLiteral(" mm"));
    connect(widthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PropertiesDock::onStrokeWidthSpinChanged);
    addWidgetRow(tr("General"), tr("Stroke Width"), widthSpin);

    // 2. Geometry & Measurements Section
    switch (entity->type()) {
        case EntityType::Line: {
            auto line = std::dynamic_pointer_cast<LineEntity>(entity);
            if (line) {
                addPropertyRow(tr("Geometry"), tr("Start X"), units.formatLinear(line->p1().x()), true);
                addPropertyRow(tr("Geometry"), tr("Start Y"), units.formatLinear(line->p1().y()), true);
                addPropertyRow(tr("Geometry"), tr("End X"), units.formatLinear(line->p2().x()), true);
                addPropertyRow(tr("Geometry"), tr("End Y"), units.formatLinear(line->p2().y()), true);
                addPropertyRow(tr("Geometry"), tr("Length"), units.formatLinear(line->length()), true);
                addPropertyRow(tr("Geometry"), tr("Angle"), units.formatAngle(line->angleDeg()), false);
                addPropertyRow(tr("Geometry"), tr("ΔX"), units.formatLinear(std::abs(line->p2().x() - line->p1().x())), false);
                addPropertyRow(tr("Geometry"), tr("ΔY"), units.formatLinear(std::abs(line->p2().y() - line->p1().y())), false);
            }
            break;
        }
        case EntityType::Rectangle: {
            auto rect = std::dynamic_pointer_cast<RectangleEntity>(entity);
            if (rect) {
                addPropertyRow(tr("Geometry"), tr("Top-Left X"), units.formatLinear(rect->topLeft().x()), false);
                addPropertyRow(tr("Geometry"), tr("Top-Left Y"), units.formatLinear(rect->topLeft().y()), false);
                addPropertyRow(tr("Geometry"), tr("Length"), units.formatLinear(rect->width()), true);
                addPropertyRow(tr("Geometry"), tr("Width"), units.formatLinear(rect->height()), true);
                addPropertyRow(tr("Measurement"), tr("Area"), units.formatArea(rect->area()), false);
                addPropertyRow(tr("Measurement"), tr("Perimeter"), units.formatLinear(rect->perimeter()), false);
            }
            break;
        }
        case EntityType::Circle: {
            auto circle = std::dynamic_pointer_cast<CircleEntity>(entity);
            if (circle) {
                addPropertyRow(tr("Geometry"), tr("Center X"), units.formatLinear(circle->center().x()), true);
                addPropertyRow(tr("Geometry"), tr("Center Y"), units.formatLinear(circle->center().y()), true);
                addPropertyRow(tr("Geometry"), tr("Radius"), units.formatLinear(circle->radius()), true);
                addPropertyRow(tr("Geometry"), tr("Diameter"), units.formatLinear(circle->diameter()), true);
                addPropertyRow(tr("Measurement"), tr("Circumference"), units.formatLinear(circle->circumference()), false);
                addPropertyRow(tr("Measurement"), tr("Area"), units.formatArea(circle->area()), false);
            }
            break;
        }
        case EntityType::Arc: {
            auto arc = std::dynamic_pointer_cast<ArcEntity>(entity);
            if (arc) {
                addPropertyRow(tr("Geometry"), tr("Center X"), units.formatLinear(arc->center().x()), true);
                addPropertyRow(tr("Geometry"), tr("Center Y"), units.formatLinear(arc->center().y()), true);
                addPropertyRow(tr("Geometry"), tr("Radius"), units.formatLinear(arc->radius()), true);
                addPropertyRow(tr("Geometry"), tr("Start Angle"), units.formatAngle(arc->startAngle()), true);
                addPropertyRow(tr("Geometry"), tr("Sweep Angle"), units.formatAngle(arc->sweepAngle()), true);
                addPropertyRow(tr("Measurement"), tr("Arc Length"), units.formatLinear(arc->arcLength()), false);
                addPropertyRow(tr("Measurement"), tr("Sector Area"), units.formatArea(arc->area()), false);
            }
            break;
        }
        case EntityType::Polyline: {
            auto poly = std::dynamic_pointer_cast<PolylineEntity>(entity);
            if (poly) {
                addPropertyRow(tr("Geometry"), tr("Vertex Count"), QString::number(poly->points().size()), false);
                addPropertyRow(tr("Geometry"), tr("Closed"), poly->isClosed() ? tr("Yes") : tr("No"), false);
                addPropertyRow(tr("Measurement"), tr("Length / Perimeter"), units.formatLinear(poly->perimeter()), false);
                if (poly->isClosed()) {
                    addPropertyRow(tr("Measurement"), tr("Area"), units.formatArea(poly->area()), false);
                }
            }
            break;
        }
        case EntityType::Polygon: {
            auto polygon = std::dynamic_pointer_cast<PolygonEntity>(entity);
            if (polygon) {
                addPropertyRow(tr("Geometry"), tr("Sides"), QString::number(polygon->sides()), true);
                addPropertyRow(tr("Geometry"), tr("Center X"), units.formatLinear(polygon->center().x()), false);
                addPropertyRow(tr("Geometry"), tr("Center Y"), units.formatLinear(polygon->center().y()), false);
                addPropertyRow(tr("Geometry"), tr("Radius"), units.formatLinear(polygon->radius()), true);
                addPropertyRow(tr("Geometry"), tr("Rotation"), units.formatAngle(polygon->rotationDeg()), false);
                addPropertyRow(tr("Measurement"), tr("Perimeter"), units.formatLinear(polygon->perimeter()), false);
                addPropertyRow(tr("Measurement"), tr("Area"), units.formatArea(polygon->area()), false);
            }
            break;
        }
        case EntityType::Ellipse: {
            auto ellipse = std::dynamic_pointer_cast<EllipseEntity>(entity);
            if (ellipse) {
                addPropertyRow(tr("Geometry"), tr("Center X"), units.formatLinear(ellipse->center().x()), false);
                addPropertyRow(tr("Geometry"), tr("Center Y"), units.formatLinear(ellipse->center().y()), false);
                addPropertyRow(tr("Geometry"), tr("Radius X"), units.formatLinear(ellipse->radiusX()), false);
                addPropertyRow(tr("Geometry"), tr("Radius Y"), units.formatLinear(ellipse->radiusY()), false);
                addPropertyRow(tr("Geometry"), tr("Rotation"), units.formatAngle(ellipse->rotationDeg()), false);
                addPropertyRow(tr("Measurement"), tr("Perimeter"), units.formatLinear(ellipse->perimeter()), false);
                addPropertyRow(tr("Measurement"), tr("Area"), units.formatArea(ellipse->area()), false);
            }
            break;
        }
        case EntityType::Text: {
            auto text = std::dynamic_pointer_cast<TextEntity>(entity);
            if (text) {
                addPropertyRow(tr("Text"), tr("Content"), text->text(), true);
                addPropertyRow(tr("Text"), tr("Height"), units.formatLinear(text->height()), true);
                addPropertyRow(tr("Text"), tr("Rotation"), units.formatAngle(text->rotationDeg()), true);
            }
            break;
        }
        case EntityType::Dimension: {
            auto dim = std::dynamic_pointer_cast<DimensionEntity>(entity);
            if (dim) {
                addPropertyRow(tr("Dimension"), tr("Type"), dim->dimensionType() == DimensionType::Aligned ? tr("Aligned") :
                                                            dim->dimensionType() == DimensionType::Horizontal ? tr("Horizontal") :
                                                            dim->dimensionType() == DimensionType::Vertical ? tr("Vertical") :
                                                            dim->dimensionType() == DimensionType::Radius ? tr("Radius") : tr("Diameter"), false);
                addPropertyRow(tr("Dimension"), tr("Measured Value"), units.formatLinear(dim->measuredValue()), true);
                addPropertyRow(tr("Dimension"), tr("Text"), dim->dimensionText(), false);
                addPropertyRow(tr("Dimension"), tr("Associative"), dim->isAssociative() ? tr("Yes") : tr("No"), false);
            }
            break;
        }
        default:
            break;
    }
}

void PropertiesDock::populateMultiSelectionProperties(const std::vector<std::shared_ptr<CadEntity>>& selected) {
    addPropertyRow(tr("Selection"), tr("Total Count"), QString::number(selected.size()), false);

    // Multi-selection layer assignment
    if (m_document && m_document->layerManager()) {
        auto* layerCombo = new QComboBox(m_table);
        const auto& layers = m_document->layerManager()->layers();
        layerCombo->addItem(tr("(Multiple Layers)"));
        for (const auto& lay : layers) {
            QPixmap pix(12, 12);
            pix.fill(Qt::transparent);
            QPainter p(&pix);
            p.setRenderHint(QPainter::Antialiasing);
            p.setBrush(lay->color());
            p.setPen(QPen(Qt::white, 1));
            p.drawEllipse(1, 1, 10, 10);
            p.end();

            layerCombo->addItem(QIcon(pix), lay->name());
        }
        connect(layerCombo, &QComboBox::currentTextChanged, this, [this, selected](const QString& layerName) {
            if (layerName == tr("(Multiple Layers)") || !m_document) return;
            for (const auto& ent : selected) {
                if (ent) {
                    auto oldState = ent->clone();
                    auto newState = ent->clone();
                    newState->setLayer(layerName);
                    m_document->undoStack()->push(new ModifyEntityPropertiesCommand(
                        m_document, ent, oldState, newState, tr("Batch Layer Change")
                    ));
                }
            }
        });
        addWidgetRow(tr("Batch General"), tr("Layer"), layerCombo);
    }

    double totalLength = 0.0;
    double totalArea = 0.0;
    for (const auto& ent : selected) {
        if (ent) {
            totalLength += ent->perimeter();
            totalArea += ent->area();
        }
    }

    addPropertyRow(tr("Batch Totals"), tr("Total Length/Perimeter"), m_document->unitSettings().formatLinear(totalLength), false);
    if (totalArea > 0.0) {
        addPropertyRow(tr("Batch Totals"), tr("Total Area"), m_document->unitSettings().formatArea(totalArea), false);
    }
}

void PropertiesDock::onLayerComboChanged(const QString& layerName) {
    if (m_isUpdating || !m_currentEntity || !m_document || layerName.isEmpty()) return;
    if (m_currentEntity->layer() == layerName) return;

    auto oldState = m_currentEntity->clone();
    auto newState = m_currentEntity->clone();
    newState->setLayer(layerName);

    m_document->undoStack()->push(new ModifyEntityPropertiesCommand(
        m_document, m_currentEntity, oldState, newState, tr("Change Layer to %1").arg(layerName)
    ));
}

void PropertiesDock::onColorButtonClicked() {
    if (!m_currentEntity || !m_document) return;

    QColor chosen = QColorDialog::getColor(m_currentEntity->color(), this, tr("Select Object Color"));
    if (chosen.isValid() && chosen != m_currentEntity->color()) {
        auto oldState = m_currentEntity->clone();
        auto newState = m_currentEntity->clone();
        newState->setColor(chosen);

        m_document->undoStack()->push(new ModifyEntityPropertiesCommand(
            m_document, m_currentEntity, oldState, newState, tr("Change Object Color")
        ));
        updateProperties();
    }
}

void PropertiesDock::onStrokeWidthSpinChanged(double val) {
    if (m_isUpdating || !m_currentEntity || !m_document) return;
    if (std::abs(m_currentEntity->strokeWidth() - val) < 1e-4) return;

    auto oldState = m_currentEntity->clone();
    auto newState = m_currentEntity->clone();
    newState->setStrokeWidth(val);

    m_document->undoStack()->push(new ModifyEntityPropertiesCommand(
        m_document, m_currentEntity, oldState, newState, tr("Change Stroke Width")
    ));
}

void PropertiesDock::onPropertyItemChanged(int row, int column) {
    if (m_isUpdating || column != 1 || !m_currentEntity || !m_document) return;

    QString propName = m_table->item(row, 0)->text();
    QString valStr = m_table->item(row, 1)->text();
    const auto& units = m_document->unitSettings();

    auto oldState = m_currentEntity->clone();
    auto newState = m_currentEntity->clone();

    bool modified = false;

    if (m_currentEntity->type() == EntityType::Line) {
        auto line = std::dynamic_pointer_cast<LineEntity>(newState);
        if (line) {
            if (propName.contains(QStringLiteral("Start X"))) {
                bool ok = false;
                double val = units.parseLinear(valStr, &ok);
                if (ok) { line->setP1(QPointF(val, line->p1().y())); modified = true; }
            } else if (propName.contains(QStringLiteral("Start Y"))) {
                bool ok = false;
                double val = units.parseLinear(valStr, &ok);
                if (ok) { line->setP1(QPointF(line->p1().x(), val)); modified = true; }
            } else if (propName.contains(QStringLiteral("End X"))) {
                bool ok = false;
                double val = units.parseLinear(valStr, &ok);
                if (ok) { line->setP2(QPointF(val, line->p2().y())); modified = true; }
            } else if (propName.contains(QStringLiteral("End Y"))) {
                bool ok = false;
                double val = units.parseLinear(valStr, &ok);
                if (ok) { line->setP2(QPointF(line->p2().x(), val)); modified = true; }
            } else if (propName.contains(QStringLiteral("Length"))) {
                bool ok = false;
                double val = units.parseLinear(valStr, &ok);
                if (ok && val > 1e-4) {
                    double currentAngle = line->angleDeg() * M_PI / 180.0;
                    line->setP2(QPointF(line->p1().x() + val * std::cos(currentAngle),
                                        line->p1().y() + val * std::sin(currentAngle)));
                    modified = true;
                }
            }
        }
    } else if (m_currentEntity->type() == EntityType::Circle) {
        auto circle = std::dynamic_pointer_cast<CircleEntity>(newState);
        if (circle) {
            if (propName.contains(QStringLiteral("Center X"))) {
                bool ok = false;
                double val = units.parseLinear(valStr, &ok);
                if (ok) { circle->setCenter(QPointF(val, circle->center().y())); modified = true; }
            } else if (propName.contains(QStringLiteral("Center Y"))) {
                bool ok = false;
                double val = units.parseLinear(valStr, &ok);
                if (ok) { circle->setCenter(QPointF(circle->center().x(), val)); modified = true; }
            } else if (propName.contains(QStringLiteral("Radius"))) {
                bool ok = false;
                double val = units.parseLinear(valStr, &ok);
                if (ok && val > 1e-4) { circle->setRadius(val); modified = true; }
            } else if (propName.contains(QStringLiteral("Diameter"))) {
                bool ok = false;
                double val = units.parseLinear(valStr, &ok);
                if (ok && val > 1e-4) { circle->setRadius(val * 0.5); modified = true; }
            }
        }
    } else if (m_currentEntity->type() == EntityType::Rectangle) {
        auto rect = std::dynamic_pointer_cast<RectangleEntity>(newState);
        if (rect) {
            if (propName.contains(QStringLiteral("Length"))) {
                bool ok = false;
                double val = units.parseLinear(valStr, &ok);
                if (ok && val > 1e-4) { rect->setWidth(val); modified = true; }
            } else if (propName.contains(QStringLiteral("Width")) || propName.contains(QStringLiteral("Height"))) {
                bool ok = false;
                double val = units.parseLinear(valStr, &ok);
                if (ok && val > 1e-4) { rect->setHeight(val); modified = true; }
            }
        }
    } else if (m_currentEntity->type() == EntityType::Dimension) {
        auto dim = std::dynamic_pointer_cast<DimensionEntity>(m_currentEntity);
        if (dim && propName.contains(QStringLiteral("Measured Value"))) {
            bool ok = false;
            double val = units.parseLinear(valStr, &ok);
            if (ok && val > 1e-4 && dim->isAssociative()) {
                auto target = m_document->findEntity(dim->targetEntityId());
                if (target) {
                    auto oldTargetState = target->clone();
                    auto newTargetState = target->clone();
                    bool targetModified = false;

                    if (target->type() == EntityType::Rectangle) {
                        auto r = std::dynamic_pointer_cast<RectangleEntity>(newTargetState);
                        if (r) {
                            if (dim->targetFeatureIndex() == 0 || dim->dimensionType() == DimensionType::Horizontal) {
                                r->setWidth(val);
                            } else {
                                r->setHeight(val);
                            }
                            targetModified = true;
                        }
                    } else if (target->type() == EntityType::Line) {
                        auto l = std::dynamic_pointer_cast<LineEntity>(newTargetState);
                        if (l) {
                            double curLen = l->length();
                            if (curLen > 1e-4) {
                                double scale = val / curLen;
                                l->setP2(l->p1() + (l->p2() - l->p1()) * scale);
                                targetModified = true;
                            }
                        }
                    } else if (target->type() == EntityType::Circle) {
                        auto c = std::dynamic_pointer_cast<CircleEntity>(newTargetState);
                        if (c) {
                            if (dim->dimensionType() == DimensionType::Diameter) {
                                c->setRadius(val * 0.5);
                            } else {
                                c->setRadius(val);
                            }
                            targetModified = true;
                        }
                    }

                    if (targetModified) {
                        m_document->undoStack()->push(new ModifyEntityPropertiesCommand(
                            m_document, target, oldTargetState, newTargetState,
                            QStringLiteral("Parametric Dimension Edit")
                        ));
                        return;
                    }
                }
            }
        }
    } else if (m_currentEntity->type() == EntityType::Polygon) {
        auto polygon = std::dynamic_pointer_cast<PolygonEntity>(newState);
        if (polygon) {
            if (propName.contains(QStringLiteral("Radius"))) {
                bool ok = false;
                double val = units.parseLinear(valStr, &ok);
                if (ok && val > 1e-4) { polygon->setRadius(val); modified = true; }
            } else if (propName.contains(QStringLiteral("Sides"))) {
                int sides = valStr.toInt();
                if (sides >= 3 && sides <= 64) { polygon->setSides(sides); modified = true; }
            }
        }
    } else if (m_currentEntity->type() == EntityType::Text) {
        auto text = std::dynamic_pointer_cast<TextEntity>(newState);
        if (text) {
            if (propName.contains(QStringLiteral("Content"))) {
                text->setText(valStr);
                modified = true;
            } else if (propName.contains(QStringLiteral("Height"))) {
                bool ok = false;
                double val = units.parseLinear(valStr, &ok);
                if (ok && val > 0.5) { text->setHeight(val); modified = true; }
            } else if (propName.contains(QStringLiteral("Rotation"))) {
                bool ok = false;
                double val = valStr.toDouble(&ok);
                if (ok) { text->setRotationDeg(val); modified = true; }
            }
        }
    }

    if (modified) {
        m_document->undoStack()->push(new ModifyEntityPropertiesCommand(
            m_document, m_currentEntity, oldState, newState,
            QStringLiteral("Change %1").arg(propName)
        ));
    }
}
