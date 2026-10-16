#pragma once

#include <QDockWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QTextBrowser>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QComboBox>
#include <memory>
#include "ai/AiDsl.h"
#include "ai/AiClient.h"
#include "ai/AiGeometryAnalyzer.h"
#include "ai/AiFloorPlanProvider.h"

class CadDocument;
class MainWindow;

class AiAssistantDock : public QDockWidget {
    Q_OBJECT

public:
    explicit AiAssistantDock(CadDocument* document, MainWindow* mainWindow, QWidget* parent = nullptr);
    ~AiAssistantDock() override = default;

    AiClient* client() { return m_aiClient.get(); }

public slots:
    void promptForGeneration(const QString& samplePrompt = QString());
    void runAnalysis();
    void runAutoDimension();
    void runAutoClean();

private slots:
    void onGenerateClicked();
    void onPreviewPlanClicked();
    void onApplyProposalClicked();
    void onDiscardProposalClicked();
    void onAnalyzeClicked();
    void onCleanClicked();
    void onDimensionAllClicked();
    void onDimensionSelectedClicked();
    void onRagSearchClicked();
    void onQuickPromptClicked();
    void onQuickRagTopicClicked();
    void onProviderChanged(int index);
    void onClarificationOptionClicked();

private:
    void setupUi();
    QWidget* createGenerateTab();
    QWidget* createAnalyzeTab();
    QWidget* createDimensionTab();
    QWidget* createRagTab();

    void displayProposal(const AiProposal& proposal);
    void clearProposal();
    void displayAnalysisReport(const AiAnalysisReport& report);

    CadDocument* m_document{nullptr};
    MainWindow* m_mainWindow{nullptr};
    std::unique_ptr<AiClient> m_aiClient;

    QTabWidget* m_tabWidget{nullptr};

    // Tab 1: Generate & Approval Workflow
    QLineEdit* m_promptInput{nullptr};
    QPushButton* m_btnGenerate{nullptr};
    QPushButton* m_btnPreviewPlan{nullptr};

    // Architectural floor plan options
    QComboBox* m_wallStyleCombo{nullptr};
    QDoubleSpinBox* m_extWallThickSpin{nullptr};
    QDoubleSpinBox* m_intWallThickSpin{nullptr};
    QComboBox* m_aiProviderCombo{nullptr};
    QLabel* m_providerStatusLabel{nullptr};

    // Clarification & Ambiguity UI
    QGroupBox* m_clarificationBox{nullptr};
    QLabel* m_clarificationLabel{nullptr};
    QWidget* m_clarificationButtonsWidget{nullptr};
    QLabel* m_errorFeedbackLabel{nullptr};

    QGroupBox* m_proposalGroupBox{nullptr};
    QLabel* m_proposalSummaryLabel{nullptr};
    QLabel* m_proposalDescLabel{nullptr};
    QListWidget* m_proposalOpsList{nullptr};
    QPushButton* m_btnApplyProposal{nullptr};
    QPushButton* m_btnDiscardProposal{nullptr};
    QLabel* m_statusLabel{nullptr};

    AiProposal m_stagedProposal;
    FloorPlanLayout m_stagedFloorPlan;
    std::vector<std::shared_ptr<CadEntity>> m_stagedPreviewEntities;

    // Tab 2: Analyze & Clean
    QPushButton* m_btnAnalyze{nullptr};
    QPushButton* m_btnAutoClean{nullptr};
    QLabel* m_statsLabel{nullptr};
    QListWidget* m_issuesList{nullptr};

    // Tab 3: Dimension
    QDoubleSpinBox* m_dimOffsetSpin{nullptr};
    QPushButton* m_btnDimensionAll{nullptr};
    QPushButton* m_btnDimensionSelected{nullptr};

    // Tab 4: RAG
    QLineEdit* m_ragQueryInput{nullptr};
    QPushButton* m_btnRagSearch{nullptr};
    QTextBrowser* m_ragBrowser{nullptr};
};
