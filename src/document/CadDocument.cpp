#include "document/CadDocument.h"
#include "layers/LayerManager.h"
#include "document/ConstraintEngine.h"
#include "geometry/DimensionEntity.h"
#include <QFileInfo>
#include <algorithm>

CadDocument::CadDocument(QObject* parent)
    : QObject(parent)
    , m_undoStack(new QUndoStack(this))
    , m_layerManager(new LayerManager(this))
    , m_constraintEngine(new ConstraintEngine(this))
{
    connect(m_undoStack, &QUndoStack::cleanChanged, this, [this](bool clean) {
        setModified(!clean);
    });

    connect(m_layerManager, &LayerManager::layerUpdated, this, [this](const QString&) {
        emit documentChanged();
    });
    connect(m_layerManager, &LayerManager::layersReset, this, [this]() {
        emit documentChanged();
    });

    // Associative dimensions: automatically update dimensions when target geometry changes
    connect(this, &CadDocument::entityModified, this, [this](std::shared_ptr<CadEntity> entity) {
        if (!entity || entity->type() == EntityType::Dimension) return;
        for (const auto& ent : m_entities) {
            if (ent && ent->type() == EntityType::Dimension) {
                auto dim = std::dynamic_pointer_cast<DimensionEntity>(ent);
                if (dim && dim->targetEntityId() == entity->id()) {
                    if (dim->updateFromTarget(entity.get())) {
                        emit entityModified(dim);
                    }
                }
            }
        }
    });

    // Detach dimension if target is removed
    connect(this, &CadDocument::entityRemoved, this, [this](const QUuid& id) {
        for (const auto& ent : m_entities) {
            if (ent && ent->type() == EntityType::Dimension) {
                auto dim = std::dynamic_pointer_cast<DimensionEntity>(ent);
                if (dim && dim->targetEntityId() == id) {
                    dim->setTargetEntityId(QUuid());
                }
            }
        }
    });
}

CadDocument::~CadDocument() = default;

std::shared_ptr<CadEntity> CadDocument::findEntity(const QUuid& id) const {
    auto it = m_entityMap.find(id);
    if (it != m_entityMap.end()) {
        return it.value();
    }
    return nullptr;
}

bool CadDocument::addEntity(const std::shared_ptr<CadEntity>& entity) {
    if (!entity) return false;
    const QUuid id = entity->id();
    if (m_entityMap.find(id) != m_entityMap.end()) {
        return false;
    }

    if (entity->layer().isEmpty()) {
        entity->setLayer(m_layerManager->activeLayerName());
    }

    // If entity is dimension, apply current document unit suffix and precision
    if (entity->type() == EntityType::Dimension) {
        auto dim = std::dynamic_pointer_cast<DimensionEntity>(entity);
        if (dim) {
            dim->setUnitSuffix(m_unitSettings.unitSuffix());
            dim->setPrecision(m_unitSettings.precision());
        }
    }

    m_entities.push_back(entity);
    m_entityMap[id] = entity;

    emit entityAdded(entity);
    emit documentChanged();
    return true;
}

bool CadDocument::removeEntity(const QUuid& id) {
    auto it = m_entityMap.find(id);
    if (it == m_entityMap.end()) {
        return false;
    }

    auto entity = it.value();
    m_entityMap.erase(it);

    auto vecIt = std::find_if(m_entities.begin(), m_entities.end(),
                             [&id](const std::shared_ptr<CadEntity>& e) {
                                 return e->id() == id;
                             });
    if (vecIt != m_entities.end()) {
        m_entities.erase(vecIt);
    }

    if (entity->isSelected()) {
        entity->setSelected(false);
        emit selectionChanged();
    }

    emit entityRemoved(id);
    emit documentChanged();
    return true;
}

void CadDocument::clear() {
    m_entities.clear();
    m_entityMap.clear();
    m_undoStack->clear();
    m_layerManager->clear();
    if (m_constraintEngine) {
        m_constraintEngine->clear();
    }
    m_filePath.clear();
    setModified(false);

    emit documentCleared();
    emit selectionChanged();
    emit documentChanged();
}

void CadDocument::select(const QUuid& id, bool addToSelection) {
    if (!addToSelection) {
        for (const auto& entity : m_entities) {
            if (entity->id() != id && entity->isSelected()) {
                entity->setSelected(false);
            }
        }
    }

    auto entity = findEntity(id);
    if (entity && !entity->isSelected()) {
        entity->setSelected(true);
    }

    emit selectionChanged();
}

void CadDocument::deselect(const QUuid& id) {
    auto entity = findEntity(id);
    if (entity && entity->isSelected()) {
        entity->setSelected(false);
        emit selectionChanged();
    }
}

void CadDocument::selectAll() {
    bool changed = false;
    for (const auto& entity : m_entities) {
        if (!entity->isSelected()) {
            entity->setSelected(true);
            changed = true;
        }
    }
    if (changed) {
        emit selectionChanged();
    }
}

void CadDocument::clearSelection() {
    bool changed = false;
    for (const auto& entity : m_entities) {
        if (entity->isSelected()) {
            entity->setSelected(false);
            changed = true;
        }
    }
    if (changed) {
        emit selectionChanged();
    }
}

bool CadDocument::isSelected(const QUuid& id) const {
    auto entity = findEntity(id);
    return entity ? entity->isSelected() : false;
}

std::vector<std::shared_ptr<CadEntity>> CadDocument::selectedEntities() const {
    std::vector<std::shared_ptr<CadEntity>> selected;
    for (const auto& entity : m_entities) {
        if (entity->isSelected()) {
            selected.push_back(entity);
        }
    }
    return selected;
}

std::vector<QUuid> CadDocument::selectedIds() const {
    std::vector<QUuid> ids;
    for (const auto& entity : m_entities) {
        if (entity->isSelected()) {
            ids.push_back(entity->id());
        }
    }
    return ids;
}

size_t CadDocument::selectionCount() const {
    size_t count = 0;
    for (const auto& entity : m_entities) {
        if (entity->isSelected()) {
            ++count;
        }
    }
    return count;
}

QRectF CadDocument::boundingBox() const {
    if (m_entities.empty()) {
        return m_pageSettings.pageRect();
    }

    QRectF box = m_entities.front()->boundingRect();
    for (size_t i = 1; i < m_entities.size(); ++i) {
        box = box.united(m_entities[i]->boundingRect());
    }
    return box;
}

void CadDocument::setModified(bool modified) {
    if (m_isModified != modified) {
        m_isModified = modified;
        emit modifiedChanged(m_isModified);
    }
}

void CadDocument::setFilePath(const QString& path) {
    if (m_filePath != path) {
        m_filePath = path;
        emit filePathChanged(m_filePath);
    }
}

QString CadDocument::fileName() const {
    if (m_filePath.isEmpty()) {
        return QStringLiteral("Untitled.gencad");
    }
    return QFileInfo(m_filePath).fileName();
}

void CadDocument::setPageSettings(const PageSettings& settings) {
    m_pageSettings = settings;
    setModified(true);
    emit pageSettingsChanged(m_pageSettings);
    emit documentChanged();
}

void CadDocument::setUnitSettings(const UnitSettings& settings) {
    m_unitSettings = settings;
    for (const auto& ent : m_entities) {
        if (ent && ent->type() == EntityType::Dimension) {
            auto dim = std::dynamic_pointer_cast<DimensionEntity>(ent);
            if (dim) {
                dim->setUnitSuffix(m_unitSettings.unitSuffix());
                dim->setPrecision(m_unitSettings.precision());
            }
        }
    }
    setModified(true);
    emit unitSettingsChanged(m_unitSettings);
    emit documentChanged();
}

void CadDocument::setGridSettings(const GridSettings& settings) {
    m_gridSettings = settings;
    emit gridSettingsChanged(m_gridSettings);
}

std::vector<std::shared_ptr<DimensionEntity>> CadDocument::findDimensionsForEntity(const QUuid& entityId) const {
    std::vector<std::shared_ptr<DimensionEntity>> dims;
    if (entityId.isNull()) return dims;
    for (const auto& ent : m_entities) {
        if (ent && ent->type() == EntityType::Dimension) {
            auto dim = std::dynamic_pointer_cast<DimensionEntity>(ent);
            if (dim && dim->targetEntityId() == entityId) {
                dims.push_back(dim);
            }
        }
    }
    return dims;
}
