#include "ui/CloudWorkspaceDialog.h"
#include "cloud/CloudClient.h"
#include "document/CadDocument.h"
#include "io/GencadFile.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFile>

// Version History sub-dialog
class CloudVersionDialog : public QDialog {
public:
    CloudVersionDialog(CloudClient* client, int drawingId, const QString& drawingName, CadDocument* doc, QWidget* parent = nullptr)
        : QDialog(parent), m_client(client), m_drawingId(drawingId), m_document(doc)
    {
        setWindowTitle(tr("Version History - %1").arg(drawingName));
        setMinimumSize(600, 400);

        auto* layout = new QVBoxLayout(this);

        m_table = new QTableWidget(this);
        m_table->setColumnCount(4);
        m_table->setHorizontalHeaderLabels({tr("Version"), tr("Commit Message"), tr("Created At"), tr("ID")});
        m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        layout->addWidget(m_table);

        auto* btnLayout = new QHBoxLayout();
        m_revertBtn = new QPushButton(tr("Revert to Selected Version"), this);
        m_revertBtn->setStyleSheet("background-color: #2e7d32; color: white; font-weight: bold; padding: 6px;");
        auto* closeBtn = new QPushButton(tr("Close"), this);
        btnLayout->addWidget(m_revertBtn);
        btnLayout->addStretch();
        btnLayout->addWidget(closeBtn);
        layout->addLayout(btnLayout);

        connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
        connect(m_revertBtn, &QPushButton::clicked, this, &CloudVersionDialog::onRevert);

        loadVersions();
    }

    bool wasReverted() const { return m_reverted; }

private:
    void loadVersions() {
        m_table->setRowCount(0);
        QString error;
        QList<CloudDrawingVersion> versions = m_client->listDrawingVersions(m_drawingId, &error);
        if (!error.isEmpty() && versions.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Failed to fetch versions: %1").arg(error));
            return;
        }

        m_table->setRowCount(versions.size());
        for (int i = 0; i < versions.size(); ++i) {
            const auto& v = versions[i];
            m_table->setItem(i, 0, new QTableWidgetItem(QString("v%1").arg(v.versionNumber)));
            m_table->setItem(i, 1, new QTableWidgetItem(v.commitMessage));
            m_table->setItem(i, 2, new QTableWidgetItem(v.createdAt));
            m_table->setItem(i, 3, new QTableWidgetItem(QString::number(v.id)));
            m_table->item(i, 0)->setData(Qt::UserRole, v.versionNumber);
        }
    }

    void onRevert() {
        int row = m_table->currentRow();
        if (row < 0) {
            QMessageBox::warning(this, tr("Select Version"), tr("Please select a version to revert to."));
            return;
        }

        int versionNum = m_table->item(row, 0)->data(Qt::UserRole).toInt();
        auto reply = QMessageBox::question(
            this,
            tr("Confirm Revert"),
            tr("Are you sure you want to revert the drawing to version %1?\nThis will create a new revision on the server and update your active canvas.").arg(versionNum),
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply != QMessageBox::Yes) return;

        QString error;
        bool ok = m_client->revertDrawingVersion(m_drawingId, versionNum, m_document, &error);
        if (ok) {
            m_reverted = true;
            QMessageBox::information(this, tr("Success"), tr("Successfully reverted to version %1 and loaded into canvas!").arg(versionNum));
            loadVersions();
        } else {
            QMessageBox::critical(this, tr("Revert Failed"), tr("Error: %1").arg(error));
        }
    }

    CloudClient* m_client;
    int m_drawingId;
    CadDocument* m_document;
    QTableWidget* m_table{nullptr};
    QPushButton* m_revertBtn{nullptr};
    bool m_reverted{false};
};

CloudWorkspaceDialog::CloudWorkspaceDialog(CloudClient* client, CadDocument* document, QWidget* parent)
    : QDialog(parent), m_client(client), m_document(document)
{
    setWindowTitle(tr("GenCAD Cloud Workspace"));
    setMinimumSize(850, 560);
    resize(920, 600);

    setupUi();

    if (m_client) {
        connect(m_client, &CloudClient::authStateChanged, this, &CloudWorkspaceDialog::updateAuthUi);
    }

    updateAuthUi();
}

void CloudWorkspaceDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    m_mainTabs = new QTabWidget(this);
    mainLayout->addWidget(m_mainTabs);

    setupAuthPage();
    setupWorkspacePage();

    m_mainTabs->addTab(m_authTab, tr("Authentication"));
    m_mainTabs->addTab(m_workspaceTab, tr("Cloud Workspace"));
}

void CloudWorkspaceDialog::setupAuthPage() {
    m_authTab = new QWidget(this);
    auto* layout = new QVBoxLayout(m_authTab);

    // Server configuration group
    auto* serverGroup = new QGroupBox(tr("Server Connection"), m_authTab);
    auto* serverLayout = new QHBoxLayout(serverGroup);
    serverLayout->addWidget(new QLabel(tr("Server URL:"), serverGroup));
    m_serverUrlEdit = new QLineEdit(m_client ? m_client->serverUrl() : QStringLiteral("http://127.0.0.1:8000"), serverGroup);
    serverLayout->addWidget(m_serverUrlEdit);
    layout->addWidget(serverGroup);

    // Auth forms container
    auto* formsLayout = new QHBoxLayout();

    // Login form
    auto* loginGroup = new QGroupBox(tr("Sign In"), m_authTab);
    auto* loginForm = new QFormLayout(loginGroup);
    m_usernameEdit = new QLineEdit(loginGroup);
    m_passwordEdit = new QLineEdit(loginGroup);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    auto* loginBtn = new QPushButton(tr("Log In"), loginGroup);
    loginBtn->setStyleSheet("background-color: #1976d2; color: white; font-weight: bold; padding: 6px;");

    loginForm->addRow(tr("Username:"), m_usernameEdit);
    loginForm->addRow(tr("Password:"), m_passwordEdit);
    loginForm->addRow(loginBtn);
    formsLayout->addWidget(loginGroup);

    // Register form
    auto* regGroup = new QGroupBox(tr("Create Account"), m_authTab);
    auto* regForm = new QFormLayout(regGroup);
    m_regUsernameEdit = new QLineEdit(regGroup);
    m_regEmailEdit = new QLineEdit(regGroup);
    m_regPasswordEdit = new QLineEdit(regGroup);
    m_regPasswordEdit->setEchoMode(QLineEdit::Password);
    auto* regBtn = new QPushButton(tr("Register"), regGroup);
    regBtn->setStyleSheet("background-color: #388e3c; color: white; font-weight: bold; padding: 6px;");

    regForm->addRow(tr("Username:"), m_regUsernameEdit);
    regForm->addRow(tr("Email:"), m_regEmailEdit);
    regForm->addRow(tr("Password:"), m_regPasswordEdit);
    regForm->addRow(regBtn);
    formsLayout->addWidget(regGroup);

    layout->addLayout(formsLayout);

    // Status label
    m_authStatusLabel = new QLabel(m_authTab);
    m_authStatusLabel->setStyleSheet("color: #ff9800; font-weight: bold; padding: 4px;");
    layout->addWidget(m_authStatusLabel);
    layout->addStretch();

    connect(loginBtn, &QPushButton::clicked, this, &CloudWorkspaceDialog::onLoginClicked);
    connect(regBtn, &QPushButton::clicked, this, &CloudWorkspaceDialog::onRegisterClicked);
}

void CloudWorkspaceDialog::setupWorkspacePage() {
    m_workspaceTab = new QWidget(this);
    auto* layout = new QVBoxLayout(m_workspaceTab);

    // Top status and logout bar
    auto* topBar = new QHBoxLayout();
    m_userInfoLabel = new QLabel(m_workspaceTab);
    m_userInfoLabel->setStyleSheet("font-size: 13px; font-weight: bold;");
    m_logoutBtn = new QPushButton(tr("Log Out"), m_workspaceTab);
    m_logoutBtn->setStyleSheet("background-color: #d32f2f; color: white; font-weight: bold;");

    topBar->addWidget(m_userInfoLabel);
    topBar->addStretch();
    topBar->addWidget(m_logoutBtn);
    layout->addLayout(topBar);

    // Splitter for Projects (left) and Drawings (right)
    auto* splitter = new QSplitter(Qt::Horizontal, m_workspaceTab);

    // Left pane: Projects
    auto* projectsWidget = new QWidget(splitter);
    auto* projectsLayout = new QVBoxLayout(projectsWidget);
    projectsLayout->setContentsMargins(0, 0, 0, 0);

    auto* projHeaderLayout = new QHBoxLayout();
    projHeaderLayout->addWidget(new QLabel(tr("<b>Projects</b>"), projectsWidget));
    projHeaderLayout->addStretch();
    m_refreshProjectsBtn = new QPushButton(tr("Refresh"), projectsWidget);
    projHeaderLayout->addWidget(m_refreshProjectsBtn);
    projectsLayout->addLayout(projHeaderLayout);

    m_projectsList = new QListWidget(projectsWidget);
    projectsLayout->addWidget(m_projectsList);

    auto* projBtnLayout = new QHBoxLayout();
    m_newProjectBtn = new QPushButton(tr("+ New Project"), projectsWidget);
    m_deleteProjectBtn = new QPushButton(tr("- Delete"), projectsWidget);
    projBtnLayout->addWidget(m_newProjectBtn);
    projBtnLayout->addWidget(m_deleteProjectBtn);
    projectsLayout->addLayout(projBtnLayout);

    splitter->addWidget(projectsWidget);

    // Right pane: Drawings
    auto* drawingsWidget = new QWidget(splitter);
    auto* drawingsLayout = new QVBoxLayout(drawingsWidget);
    drawingsLayout->setContentsMargins(0, 0, 0, 0);

    auto* drawHeaderLayout = new QHBoxLayout();
    drawHeaderLayout->addWidget(new QLabel(tr("<b>Drawings in Project</b>"), drawingsWidget));
    drawHeaderLayout->addStretch();
    m_refreshDrawingsBtn = new QPushButton(tr("Refresh"), drawingsWidget);
    drawHeaderLayout->addWidget(m_refreshDrawingsBtn);
    drawingsLayout->addLayout(drawHeaderLayout);

    m_drawingsTable = new QTableWidget(drawingsWidget);
    m_drawingsTable->setColumnCount(6);
    m_drawingsTable->setHorizontalHeaderLabels({tr("ID"), tr("Drawing Name"), tr("Version"), tr("Format"), tr("Last Updated"), tr("Description")});
    m_drawingsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_drawingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_drawingsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_drawingsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    drawingsLayout->addWidget(m_drawingsTable);

    // Action buttons toolbar
    auto* actionsLayout = new QHBoxLayout();
    m_openDrawingBtn = new QPushButton(tr("Open in Canvas"), drawingsWidget);
    m_openDrawingBtn->setStyleSheet("background-color: #1976d2; color: white; font-weight: bold;");

    m_saveCurrentBtn = new QPushButton(tr("Upload Canvas as New"), drawingsWidget);
    m_saveCurrentBtn->setStyleSheet("background-color: #388e3c; color: white; font-weight: bold;");

    m_updateDrawingBtn = new QPushButton(tr("Update Cloud with Canvas"), drawingsWidget);
    m_updateDrawingBtn->setStyleSheet("background-color: #0288d1; color: white; font-weight: bold;");

    m_historyBtn = new QPushButton(tr("Version History"), drawingsWidget);
    m_downloadBtn = new QPushButton(tr("Download File"), drawingsWidget);
    m_deleteDrawingBtn = new QPushButton(tr("Delete Drawing"), drawingsWidget);

    actionsLayout->addWidget(m_openDrawingBtn);
    actionsLayout->addWidget(m_saveCurrentBtn);
    actionsLayout->addWidget(m_updateDrawingBtn);
    actionsLayout->addWidget(m_historyBtn);
    actionsLayout->addWidget(m_downloadBtn);
    actionsLayout->addWidget(m_deleteDrawingBtn);
    drawingsLayout->addLayout(actionsLayout);

    splitter->addWidget(drawingsWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);

    layout->addWidget(splitter);

    // Status label at bottom
    m_workspaceStatusLabel = new QLabel(m_workspaceTab);
    m_workspaceStatusLabel->setStyleSheet("color: #90caf9; padding: 2px;");
    layout->addWidget(m_workspaceStatusLabel);

    // Connect signals
    connect(m_logoutBtn, &QPushButton::clicked, this, &CloudWorkspaceDialog::onLogoutClicked);
    connect(m_refreshProjectsBtn, &QPushButton::clicked, this, &CloudWorkspaceDialog::onRefreshProjects);
    connect(m_projectsList, &QListWidget::currentRowChanged, this, &CloudWorkspaceDialog::onProjectSelectionChanged);
    connect(m_newProjectBtn, &QPushButton::clicked, this, &CloudWorkspaceDialog::onCreateProject);
    connect(m_deleteProjectBtn, &QPushButton::clicked, this, &CloudWorkspaceDialog::onDeleteProject);

    connect(m_refreshDrawingsBtn, &QPushButton::clicked, this, &CloudWorkspaceDialog::onRefreshDrawings);
    connect(m_openDrawingBtn, &QPushButton::clicked, this, &CloudWorkspaceDialog::onOpenDrawing);
    connect(m_saveCurrentBtn, &QPushButton::clicked, this, &CloudWorkspaceDialog::onSaveCurrentToCloud);
    connect(m_updateDrawingBtn, &QPushButton::clicked, this, &CloudWorkspaceDialog::onUpdateSelectedDrawing);
    connect(m_historyBtn, &QPushButton::clicked, this, &CloudWorkspaceDialog::onVersionHistory);
    connect(m_downloadBtn, &QPushButton::clicked, this, &CloudWorkspaceDialog::onDownloadDrawing);
    connect(m_deleteDrawingBtn, &QPushButton::clicked, this, &CloudWorkspaceDialog::onDeleteDrawing);
}

void CloudWorkspaceDialog::updateAuthUi() {
    bool loggedIn = m_client && m_client->isLoggedIn();

    if (loggedIn) {
        m_userInfoLabel->setText(tr("Connected to <b>%1</b> as user <b>%2</b>").arg(m_client->serverUrl(), m_client->username()));
        m_mainTabs->setCurrentWidget(m_workspaceTab);
        loadProjects();
    } else {
        m_userInfoLabel->setText(tr("Not Connected"));
        m_mainTabs->setCurrentWidget(m_authTab);
        m_projectsList->clear();
        m_drawingsTable->setRowCount(0);
    }
}

void CloudWorkspaceDialog::onLoginClicked() {
    if (!m_client) return;

    m_client->setServerUrl(m_serverUrlEdit->text());
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        m_authStatusLabel->setText(tr("Please enter both username and password"));
        return;
    }

    m_authStatusLabel->setText(tr("Authenticating..."));
    QString error;
    bool ok = m_client->login(username, password, &error);
    if (ok) {
        m_authStatusLabel->setText(QStringLiteral("Logged in successfully!"));
        m_passwordEdit->clear();
    } else {
        m_authStatusLabel->setText(tr("Login error: %1").arg(error.isEmpty() ? tr("Invalid credentials") : error));
        QMessageBox::warning(this, tr("Login Failed"), m_authStatusLabel->text());
    }
}

void CloudWorkspaceDialog::onRegisterClicked() {
    if (!m_client) return;

    m_client->setServerUrl(m_serverUrlEdit->text());
    QString username = m_regUsernameEdit->text().trimmed();
    QString email = m_regEmailEdit->text().trimmed();
    QString password = m_regPasswordEdit->text();

    if (username.isEmpty() || email.isEmpty() || password.isEmpty()) {
        m_authStatusLabel->setText(tr("Please fill in username, email, and password"));
        return;
    }

    m_authStatusLabel->setText(tr("Registering account..."));
    QString error;
    bool ok = m_client->registerUser(username, email, password, &error);
    if (ok) {
        m_authStatusLabel->setText(QStringLiteral("Registration successful! Logging in..."));
        m_client->login(username, password, &error);
    } else {
        m_authStatusLabel->setText(tr("Registration error: %1").arg(error));
        QMessageBox::warning(this, tr("Registration Failed"), m_authStatusLabel->text());
    }
}

void CloudWorkspaceDialog::onLogoutClicked() {
    if (m_client) {
        m_client->logout();
    }
}

void CloudWorkspaceDialog::onRefreshProjects() {
    loadProjects();
}

void CloudWorkspaceDialog::loadProjects() {
    if (!m_client || !m_client->isLoggedIn()) return;

    m_projectsList->clear();
    QString error;
    QList<CloudProject> projects = m_client->listProjects(&error);
    if (!error.isEmpty() && projects.isEmpty()) {
        m_workspaceStatusLabel->setText(tr("Failed to load projects: %1").arg(error));
        return;
    }

    for (const auto& proj : projects) {
        auto* item = new QListWidgetItem(QStringLiteral("%1 (#%2)").arg(proj.name).arg(proj.id), m_projectsList);
        item->setData(Qt::UserRole, proj.id);
        item->setToolTip(proj.description);
    }

    if (m_projectsList->count() > 0) {
        m_projectsList->setCurrentRow(0);
    } else {
        m_drawingsTable->setRowCount(0);
        m_workspaceStatusLabel->setText(tr("No projects found. Click '+ New Project' to create one."));
    }
}

void CloudWorkspaceDialog::onProjectSelectionChanged() {
    auto* item = m_projectsList->currentItem();
    if (!item) {
        m_drawingsTable->setRowCount(0);
        return;
    }

    int projectId = item->data(Qt::UserRole).toInt();
    loadDrawingsForProject(projectId);
}

void CloudWorkspaceDialog::loadDrawingsForProject(int projectId) {
    if (!m_client) return;

    m_drawingsTable->setRowCount(0);
    QString error;
    QList<CloudDrawing> drawings = m_client->listDrawings(projectId, &error);
    if (!error.isEmpty() && drawings.isEmpty()) {
        m_workspaceStatusLabel->setText(tr("Failed to load drawings: %1").arg(error));
        return;
    }

    m_drawingsTable->setRowCount(drawings.size());
    for (int i = 0; i < drawings.size(); ++i) {
        const auto& d = drawings[i];
        m_drawingsTable->setItem(i, 0, new QTableWidgetItem(QString::number(d.id)));
        m_drawingsTable->setItem(i, 1, new QTableWidgetItem(d.name));
        m_drawingsTable->setItem(i, 2, new QTableWidgetItem(QString("v%1").arg(d.version)));
        m_drawingsTable->setItem(i, 3, new QTableWidgetItem(d.format.toUpper()));
        m_drawingsTable->setItem(i, 4, new QTableWidgetItem(d.updatedAt));
        m_drawingsTable->setItem(i, 5, new QTableWidgetItem(d.description));
        m_drawingsTable->item(i, 0)->setData(Qt::UserRole, d.id);
    }

    m_workspaceStatusLabel->setText(tr("Loaded %1 drawings for project.").arg(drawings.size()));
}

void CloudWorkspaceDialog::onCreateProject() {
    if (!m_client) return;

    bool ok = false;
    QString name = QInputDialog::getText(this, tr("New Project"), tr("Project Name:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QString desc = QInputDialog::getText(this, tr("New Project"), tr("Description (optional):"), QLineEdit::Normal, QString(), &ok);

    QString error;
    CloudProject p = m_client->createProject(name.trimmed(), desc.trimmed(), &error);
    if (p.id > 0) {
        loadProjects();
        // Select newly created project
        for (int i = 0; i < m_projectsList->count(); ++i) {
            if (m_projectsList->item(i)->data(Qt::UserRole).toInt() == p.id) {
                m_projectsList->setCurrentRow(i);
                break;
            }
        }
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to create project: %1").arg(error));
    }
}

void CloudWorkspaceDialog::onDeleteProject() {
    auto* item = m_projectsList->currentItem();
    if (!item) return;

    int projectId = item->data(Qt::UserRole).toInt();
    auto reply = QMessageBox::question(
        this,
        tr("Delete Project"),
        tr("Are you sure you want to delete this project and all its drawings?"),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) return;

    QString error;
    if (m_client->deleteProject(projectId, &error)) {
        loadProjects();
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to delete project: %1").arg(error));
    }
}

void CloudWorkspaceDialog::onRefreshDrawings() {
    onProjectSelectionChanged();
}

void CloudWorkspaceDialog::onOpenDrawing() {
    int row = m_drawingsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, tr("Select Drawing"), tr("Please select a drawing to open."));
        return;
    }

    int drawingId = m_drawingsTable->item(row, 0)->data(Qt::UserRole).toInt();
    QString drawingName = m_drawingsTable->item(row, 1)->text();

    QString error;
    if (m_client->loadDocumentFromCloud(drawingId, m_document, &error)) {
        m_document->setFilePath(QStringLiteral("[Cloud: %1]").arg(drawingName));
        emit documentLoaded(drawingName);
        QMessageBox::information(this, tr("Drawing Opened"), tr("Drawing '%1' loaded into canvas successfully!").arg(drawingName));
    } else {
        QMessageBox::critical(this, tr("Open Failed"), tr("Failed to load drawing from cloud: %1").arg(error));
    }
}

void CloudWorkspaceDialog::onSaveCurrentToCloud() {
    auto* item = m_projectsList->currentItem();
    if (!item) {
        QMessageBox::warning(this, tr("Select Project"), tr("Please select a project before uploading."));
        return;
    }

    int projectId = item->data(Qt::UserRole).toInt();

    bool ok = false;
    QString defaultName = m_document ? m_document->fileName() : QStringLiteral("Drawing_1");
    if (defaultName.startsWith(QStringLiteral("[Cloud: ")) && defaultName.endsWith(QStringLiteral("]"))) {
        defaultName = defaultName.mid(8, defaultName.length() - 9);
    }
    QString name = QInputDialog::getText(this, tr("Upload Canvas to Cloud"), tr("Drawing Name:"), QLineEdit::Normal, defaultName, &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QString error;
    if (m_client->saveDocumentToCloud(projectId, name.trimmed(), m_document, &error)) {
        m_workspaceStatusLabel->setText(tr("Drawing '%1' uploaded successfully!").arg(name));
        loadDrawingsForProject(projectId);
    } else {
        QMessageBox::critical(this, tr("Upload Failed"), tr("Error uploading drawing: %1").arg(error));
    }
}

void CloudWorkspaceDialog::onUpdateSelectedDrawing() {
    int row = m_drawingsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, tr("Select Drawing"), tr("Please select a drawing from the table to update."));
        return;
    }

    int drawingId = m_drawingsTable->item(row, 0)->data(Qt::UserRole).toInt();
    QString drawingName = m_drawingsTable->item(row, 1)->text();

    bool ok = false;
    QString commitMsg = QInputDialog::getText(
        this,
        tr("Update Drawing Revision"),
        tr("Commit Message for '%1':").arg(drawingName),
        QLineEdit::Normal,
        tr("Desktop canvas update"),
        &ok
    );
    if (!ok) return;

    QString error;
    if (m_client->updateDocumentToCloud(drawingId, m_document, commitMsg.trimmed(), &error)) {
        m_workspaceStatusLabel->setText(tr("Drawing '%1' updated with new revision!").arg(drawingName));
        onProjectSelectionChanged();
        QMessageBox::information(this, tr("Revision Created"), tr("New version for '%1' saved to cloud.").arg(drawingName));
    } else {
        QMessageBox::critical(this, tr("Update Failed"), tr("Failed to update drawing: %1").arg(error));
    }
}

void CloudWorkspaceDialog::onDeleteDrawing() {
    int row = m_drawingsTable->currentRow();
    if (row < 0) return;

    int drawingId = m_drawingsTable->item(row, 0)->data(Qt::UserRole).toInt();
    QString drawingName = m_drawingsTable->item(row, 1)->text();

    auto reply = QMessageBox::question(
        this,
        tr("Delete Drawing"),
        tr("Are you sure you want to permanently delete drawing '%1'?").arg(drawingName),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) return;

    QString error;
    if (m_client->deleteDrawing(drawingId, &error)) {
        onProjectSelectionChanged();
    } else {
        QMessageBox::critical(this, tr("Delete Failed"), tr("Error: %1").arg(error));
    }
}

void CloudWorkspaceDialog::onVersionHistory() {
    int row = m_drawingsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, tr("Select Drawing"), tr("Please select a drawing to view version history."));
        return;
    }

    int drawingId = m_drawingsTable->item(row, 0)->data(Qt::UserRole).toInt();
    QString drawingName = m_drawingsTable->item(row, 1)->text();

    CloudVersionDialog dlg(m_client, drawingId, drawingName, m_document, this);
    dlg.exec();

    if (dlg.wasReverted()) {
        m_document->setFilePath(QStringLiteral("[Cloud: %1]").arg(drawingName));
        emit documentLoaded(drawingName);
        onProjectSelectionChanged();
    }
}

void CloudWorkspaceDialog::onDownloadDrawing() {
    int row = m_drawingsTable->currentRow();
    if (row < 0) return;

    int drawingId = m_drawingsTable->item(row, 0)->data(Qt::UserRole).toInt();
    QString drawingName = m_drawingsTable->item(row, 1)->text();
    QString format = m_drawingsTable->item(row, 3)->text().toLower();

    QString ext = (format == QStringLiteral("dxf")) ? QStringLiteral(".dxf") : QStringLiteral(".gencad");
    QString filter = (format == QStringLiteral("dxf")) ? QStringLiteral("DXF Drawing (*.dxf)") : GencadFile::fileFilter();

    QString localPath = QFileDialog::getSaveFileName(this, tr("Save Drawing Locally"), drawingName + ext, filter);
    if (localPath.isEmpty()) return;

    QString content;
    QString error;
    CloudDrawing d = m_client->getDrawing(drawingId, &content, &error);
    if (d.id == 0) {
        QMessageBox::critical(this, tr("Download Failed"), tr("Failed to fetch drawing content: %1").arg(error));
        return;
    }

    QFile file(localPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to open file for writing:\n%1").arg(file.errorString()));
        return;
    }

    file.write(content.toUtf8());
    file.close();
    QMessageBox::information(this, tr("Saved"), tr("Drawing saved to:\n%1").arg(localPath));
}
