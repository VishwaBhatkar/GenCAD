#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QPoint>

class CadDocument;

class LayerWidget : public QWidget {
    Q_OBJECT

public:
    explicit LayerWidget(CadDocument* document, QWidget* parent = nullptr);
    ~LayerWidget() override = default;

    void setDocument(CadDocument* document);

public slots:
    void refresh();
    void showNewLayerDialog();

private slots:
    void onAddLayerClicked();
    void onDeleteLayerClicked();
    void onSetActiveClicked();
    void onTurnAllOnClicked();
    void onTurnAllOffClicked();
    void onFilterTextChanged(const QString& text);
    void onCellClicked(int row, int column);
    void onCellDoubleClicked(int row, int column);
    void onContextMenuRequested(const QPoint& pos);

private:
    void setupUi();
    int countEntitiesOnLayer(const QString& layerName) const;

    CadDocument* m_document{nullptr};
    QLineEdit* m_filterEdit{nullptr};
    QTableWidget* m_table{nullptr};
    QPushButton* m_btnAdd{nullptr};
    QPushButton* m_btnDelete{nullptr};
    QPushButton* m_btnSetActive{nullptr};
    QPushButton* m_btnTurnAllOn{nullptr};
    QPushButton* m_btnTurnAllOff{nullptr};
    bool m_updatingTable{false};
};
