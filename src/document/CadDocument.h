#pragma once

#include <QObject>
#include <QUndoStack>
#include <QUuid>
#include <QRectF>
#include <vector>
#include <memory>
#include <QHash>
#include "geometry/CadEntity.h"
#include "document/PageSettings.h"
#include "document/UnitSystem.h"
#include "document/GridSettings.h"

class ConstraintEngine;
class DimensionEntity;

class CadDocument : public QObject {
    Q_OBJECT

public:
    explicit CadDocument(QObject* parent = nullptr);
    ~CadDocument() override;

    // Entity access
    size_t entityCount() const { return m_entities.size(); }
    const std::vector<std::shared_ptr<CadEntity>>& entities() const { return m_entities; }
    std::shared_ptr<CadEntity> findEntity(const QUuid& id) const;

    // Direct entity manipulation (called by Commands)
    bool addEntity(const std::shared_ptr<CadEntity>& entity);
    bool removeEntity(const QUuid& id);
    void clear();

    // Selection management
    void select(const QUuid& id, bool addToSelection = false);
    void deselect(const QUuid& id);
    void selectAll();
    void clearSelection();
    bool isSelected(const QUuid& id) const;
    std::vector<std::shared_ptr<CadEntity>> selectedEntities() const;
    std::vector<QUuid> selectedIds() const;
    size_t selectionCount() const;

    // Geometry
    QRectF boundingBox() const;

    // Document state & file
    bool isModified() const { return m_isModified; }
    void setModified(bool modified);

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString& path);
    QString fileName() const;

    // Undo stack
    QUndoStack* undoStack() const { return m_undoStack; }

    // Layers
    class LayerManager* layerManager() const { return m_layerManager; }

    // Page Settings
    const PageSettings& pageSettings() const { return m_pageSettings; }
    PageSettings& pageSettings() { return m_pageSettings; }
    void setPageSettings(const PageSettings& settings);

    // Unit Settings
    const UnitSettings& unitSettings() const { return m_unitSettings; }
    UnitSettings& unitSettings() { return m_unitSettings; }
    void setUnitSettings(const UnitSettings& settings);

    // Grid Settings
    const GridSettings& gridSettings() const { return m_gridSettings; }
    GridSettings& gridSettings() { return m_gridSettings; }
    void setGridSettings(const GridSettings& settings);

    // Constraint Engine
    ConstraintEngine* constraintEngine() const { return m_constraintEngine; }

    // Associative dimensions helper
    std::vector<std::shared_ptr<DimensionEntity>> findDimensionsForEntity(const QUuid& entityId) const;

signals:
    void entityAdded(std::shared_ptr<CadEntity> entity);
    void entityRemoved(const QUuid& id);
    void entityModified(std::shared_ptr<CadEntity> entity);
    void documentCleared();
    void selectionChanged();
    void documentChanged();
    void modifiedChanged(bool modified);
    void filePathChanged(const QString& path);
    void pageSettingsChanged(const PageSettings& settings);
    void unitSettingsChanged(const UnitSettings& settings);
    void gridSettingsChanged(const GridSettings& settings);

private:
    std::vector<std::shared_ptr<CadEntity>> m_entities;
    QHash<QUuid, std::shared_ptr<CadEntity>> m_entityMap;
    QUndoStack* m_undoStack{nullptr};
    class LayerManager* m_layerManager{nullptr};
    PageSettings m_pageSettings;
    UnitSettings m_unitSettings;
    GridSettings m_gridSettings;
    ConstraintEngine* m_constraintEngine{nullptr};
    QString m_filePath;
    bool m_isModified{false};
};
