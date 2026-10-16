#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class CadDocument;

struct CloudProject {
    int id{0};
    QString name;
    QString description;
    QString createdAt;
};

struct CloudDrawing {
    int id{0};
    int projectId{0};
    QString name;
    QString format; // "gencad" or "dxf"
    int version{1};
    QString description;
    QString updatedAt;
};

struct CloudDrawingVersion {
    int id{0};
    int versionNumber{1};
    QString commitMessage;
    QString createdAt;
};

class CloudClient : public QObject {
    Q_OBJECT

public:
    explicit CloudClient(QObject* parent = nullptr);
    ~CloudClient() override = default;

    QString serverUrl() const { return m_serverUrl; }
    void setServerUrl(const QString& url);

    bool isLoggedIn() const { return !m_token.isEmpty(); }
    QString token() const { return m_token; }
    QString username() const { return m_username; }
    int userId() const { return m_userId; }

    void logout();

    // Authentication
    bool registerUser(const QString& username, const QString& email, const QString& password, QString* errorMessage = nullptr);
    bool login(const QString& username, const QString& password, QString* errorMessage = nullptr);

    // Projects
    QList<CloudProject> listProjects(QString* errorMessage = nullptr);
    CloudProject createProject(const QString& name, const QString& description, QString* errorMessage = nullptr);
    bool deleteProject(int projectId, QString* errorMessage = nullptr);

    // Drawings
    QList<CloudDrawing> listDrawings(int projectId = 0, QString* errorMessage = nullptr);
    CloudDrawing getDrawing(int drawingId, QString* content = nullptr, QString* errorMessage = nullptr);
    CloudDrawing uploadDrawing(int projectId, const QString& name, const QString& content, const QString& format = "gencad", const QString& description = "", QString* errorMessage = nullptr);
    CloudDrawing updateDrawing(int drawingId, const QString& content, const QString& commitMessage = "Update drawing", QString* errorMessage = nullptr);
    bool deleteDrawing(int drawingId, QString* errorMessage = nullptr);

    // Document direct save / load
    bool saveDocumentToCloud(int projectId, const QString& name, const CadDocument* doc, QString* errorMessage = nullptr);
    bool updateDocumentToCloud(int drawingId, const CadDocument* doc, const QString& commitMessage, QString* errorMessage = nullptr);
    bool loadDocumentFromCloud(int drawingId, CadDocument* doc, QString* errorMessage = nullptr);

    // Versions & Revert
    QList<CloudDrawingVersion> listDrawingVersions(int drawingId, QString* errorMessage = nullptr);
    bool revertDrawingVersion(int drawingId, int versionNumber, CadDocument* doc, QString* errorMessage = nullptr);

signals:
    void authStateChanged(bool loggedIn, const QString& username);

private:
    QByteArray executeSyncRequest(const QString& endpoint,
                                 const QString& method,
                                 const QByteArray& data,
                                 int* statusCode,
                                 QString* errorMessage);

    QString m_serverUrl{QStringLiteral("http://127.0.0.1:8000")};
    QString m_token;
    QString m_username;
    int m_userId{0};
    QNetworkAccessManager m_netManager;
};
