#pragma once

#include <QDockWidget>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <memory>
#include "geometry/CadEntity.h"

class CadDocument;

class PropertiesDock : public QDockWidget {
    Q_OBJECT

public:
    explicit PropertiesDock(CadDocument* document, QWidget* parent = nullptr);
    ~PropertiesDock() override = default;

public slots:
    void updateProperties();

private slots:
    void onPropertyItemChanged(int row, int column);
    void onLayerComboChanged(const QString& layerName);
    void onColorButtonClicked();
    void onStrokeWidthSpinChanged(double val);

private:
    void setupUi();
    void addPropertyRow(const QString& category, const QString& name, const QString& value, bool editable = false);
    void addWidgetRow(const QString& category, const QString& name, QWidget* widget);
    void populateDocumentSummary();
    void populateEntityProperties(const std::shared_ptr<CadEntity>& entity);
    void populateMultiSelectionProperties(const std::vector<std::shared_ptr<CadEntity>>& selected);

    CadDocument* m_document{nullptr};
    QTableWidget* m_table{nullptr};
    QLabel* m_headerLabel{nullptr};
    bool m_isUpdating{false};
    std::shared_ptr<CadEntity> m_currentEntity;
};
