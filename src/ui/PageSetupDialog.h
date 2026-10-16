#pragma once

#include <QDialog>
#include <QStackedWidget>
#include <QComboBox>
#include <QRadioButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include "document/PageSettings.h"
#include "document/UnitSystem.h"

class PagePreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PagePreviewWidget(QWidget* parent = nullptr);
    void setPageSettings(const PageSettings& settings);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    PageSettings m_settings;
};

class CadDocument;

class PageSetupDialog : public QDialog {
    Q_OBJECT

public:
    explicit PageSetupDialog(const PageSettings& initialPage, const UnitSettings& initialUnits, QWidget* parent = nullptr);
    explicit PageSetupDialog(CadDocument* doc, QWidget* parent = nullptr);
    ~PageSetupDialog() override = default;

    PageSettings pageSettings() const { return m_pageSettings; }
    UnitSettings unitSettings() const { return m_unitSettings; }
    void applySettings(CadDocument* doc) const;

private slots:
    void onPageSizeChanged(int index);
    void onOrientationChanged();
    void onCustomDimensionsChanged();
    void onLimitsChanged();
    void onUnitChanged(int index);
    void onPrecisionChanged(int index);
    void updatePagePreview();
    void updateUnitPreview();
    void goToStep2();
    void goToStep1();

private:
    void setupUi();
    QWidget* createPageLayoutStep();
    QWidget* createUnitsStep();

    PageSettings m_pageSettings;
    UnitSettings m_unitSettings;

    QStackedWidget* m_stack{nullptr};

    // Step 1: Layout controls
    QComboBox* m_comboPageSize{nullptr};
    QRadioButton* m_radioPortrait{nullptr};
    QRadioButton* m_radioLandscape{nullptr};
    QDoubleSpinBox* m_spinCustomW{nullptr};
    QDoubleSpinBox* m_spinCustomH{nullptr};
    QDoubleSpinBox* m_spinMinX{nullptr};
    QDoubleSpinBox* m_spinMinY{nullptr};
    QDoubleSpinBox* m_spinMaxX{nullptr};
    QDoubleSpinBox* m_spinMaxY{nullptr};
    QLabel* m_labelPageDimensions{nullptr};
    PagePreviewWidget* m_pagePreview{nullptr};

    // Step 2: Units controls
    QComboBox* m_comboUnits{nullptr};
    QSpinBox* m_spinPrecision{nullptr};
    QDoubleSpinBox* m_spinCustomScale{nullptr};
    QLineEdit* m_editCustomUnitName{nullptr};
    QLabel* m_labelUnitPreview{nullptr};
};
