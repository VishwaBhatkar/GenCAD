#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>

class CloudClient;
class CadDocument;

class CloudWorkspaceDialog : public QDialog {
    Q_OBJECT

public:
    explicit CloudWorkspaceDialog(CloudClient* client, CadDocument* document, QWidget* parent = nullptr);
    ~CloudWorkspaceDialog() override = default;

signals:
    void documentLoaded(const QString& drawingName);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onLogoutClicked();
    void onRefreshProjects();
    void onProjectSelectionChanged();
    void onCreateProject();
    void onDeleteProject();
    void onRefreshDrawings();
    void onOpenDrawing();
    void onSaveCurrentToCloud();
    void onUpdateSelectedDrawing();
    void onDeleteDrawing();
    void onVersionHistory();
    void onDownloadDrawing();
    void updateAuthUi();

private:
    void setupUi();
    void setupAuthPage();
    void setupWorkspacePage();
    void loadProjects();
    void loadDrawingsForProject(int projectId);

    CloudClient* m_client{nullptr};
    CadDocument* m_document{nullptr};

    // Main layout widgets
    QTabWidget* m_mainTabs{nullptr};
    QWidget* m_authTab{nullptr};
    QWidget* m_workspaceTab{nullptr};

    // Auth widgets
    QLineEdit* m_serverUrlEdit{nullptr};
    QLineEdit* m_usernameEdit{nullptr};
    QLineEdit* m_passwordEdit{nullptr};
    QLineEdit* m_regUsernameEdit{nullptr};
    QLineEdit* m_regEmailEdit{nullptr};
    QLineEdit* m_regPasswordEdit{nullptr};
    QLabel* m_authStatusLabel{nullptr};

    // Workspace widgets
    QLabel* m_userInfoLabel{nullptr};
    QPushButton* m_logoutBtn{nullptr};
    QListWidget* m_projectsList{nullptr};
    QPushButton* m_newProjectBtn{nullptr};
    QPushButton* m_deleteProjectBtn{nullptr};
    QPushButton* m_refreshProjectsBtn{nullptr};

    QTableWidget* m_drawingsTable{nullptr};
    QPushButton* m_openDrawingBtn{nullptr};
    QPushButton* m_saveCurrentBtn{nullptr};
    QPushButton* m_updateDrawingBtn{nullptr};
    QPushButton* m_historyBtn{nullptr};
    QPushButton* m_deleteDrawingBtn{nullptr};
    QPushButton* m_downloadBtn{nullptr};
    QPushButton* m_refreshDrawingsBtn{nullptr};
    QLabel* m_workspaceStatusLabel{nullptr};
};
