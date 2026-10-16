#include "cloud/CloudClient.h"
#include "document/CadDocument.h"
#include "io/GencadFile.h"

#include <QEventLoop>
#include <QTimer>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

CloudClient::CloudClient(QObject* parent)
    : QObject(parent) {
}

void CloudClient::setServerUrl(const QString& url) {
    m_serverUrl = url.trimmed();
    while (m_serverUrl.endsWith('/')) {
        m_serverUrl.chop(1);
    }
}

void CloudClient::logout() {
    m_token.clear();
    m_username.clear();
    m_userId = 0;
    emit authStateChanged(false, QString());
}

QByteArray CloudClient::executeSyncRequest(const QString& endpoint,
                                           const QString& method,
                                           const QByteArray& data,
                                           int* statusCode,
                                           QString* errorMessage) {
    QString fullUrl = m_serverUrl + endpoint;
    QNetworkRequest request{QUrl(fullUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toUtf8());
    }

    QNetworkReply* reply = nullptr;
    if (method == QStringLiteral("GET")) {
        reply = m_netManager.get(request);
    } else if (method == QStringLiteral("POST")) {
        reply = m_netManager.post(request, data);
    } else if (method == QStringLiteral("PUT")) {
        reply = m_netManager.put(request, data);
    } else if (method == QStringLiteral("DELETE")) {
        reply = m_netManager.deleteResource(request);
    } else {
        if (errorMessage) *errorMessage = QString("Unsupported HTTP method: %1").arg(method);
        return QByteArray();
    }

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        if (reply && reply->isRunning()) {
            reply->abort();
        }
        loop.quit();
    });
    timer.start(10000); // 10 second timeout

    loop.exec();

    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode) {
        *statusCode = code;
    }

    QByteArray responseData = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        if (errorMessage) {
            QJsonParseError parseErr;
            QJsonDocument errDoc = QJsonDocument::fromJson(responseData, &parseErr);
            if (parseErr.error == QJsonParseError::NoError && errDoc.isObject() && errDoc.object().contains("detail")) {
                *errorMessage = errDoc.object()["detail"].toString();
            } else {
                *errorMessage = reply->errorString();
            }
        }
    }

    reply->deleteLater();
    return responseData;
}

bool CloudClient::registerUser(const QString& username, const QString& email, const QString& password, QString* errorMessage) {
    QJsonObject reqObj;
    reqObj["username"] = username;
    reqObj["email"] = email;
    reqObj["password"] = password;

    int statusCode = 0;
    QByteArray resData = executeSyncRequest(
        QStringLiteral("/api/v1/auth/register"),
        QStringLiteral("POST"),
        QJsonDocument(reqObj).toJson(QJsonDocument::Compact),
        &statusCode,
        errorMessage
    );

    return (statusCode == 201 || statusCode == 200);
}

bool CloudClient::login(const QString& username, const QString& password, QString* errorMessage) {
    QJsonObject reqObj;
    reqObj["username"] = username;
    reqObj["password"] = password;

    int statusCode = 0;
    QByteArray resData = executeSyncRequest(
        QStringLiteral("/api/v1/auth/login"),
        QStringLiteral("POST"),
        QJsonDocument(reqObj).toJson(QJsonDocument::Compact),
        &statusCode,
        errorMessage
    );

    if (statusCode != 200) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(resData);
    if (!doc.isObject()) {
        if (errorMessage) *errorMessage = QStringLiteral("Invalid server response");
        return false;
    }

    QJsonObject obj = doc.object();
    m_token = obj["access_token"].toString();
    if (obj.contains("user") && obj["user"].isObject()) {
        QJsonObject userObj = obj["user"].toObject();
        m_username = userObj["username"].toString();
        m_userId = userObj["id"].toInt();
    } else {
        m_username = username;
    }

    emit authStateChanged(true, m_username);
    return true;
}

QList<CloudProject> CloudClient::listProjects(QString* errorMessage) {
    QList<CloudProject> result;
    int statusCode = 0;
    QByteArray resData = executeSyncRequest(
        QStringLiteral("/api/v1/projects/"),
        QStringLiteral("GET"),
        QByteArray(),
        &statusCode,
        errorMessage
    );

    if (statusCode != 200) {
        return result;
    }

    QJsonDocument doc = QJsonDocument::fromJson(resData);
    if (!doc.isArray()) return result;

    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        if (!item.isObject()) continue;
        QJsonObject obj = item.toObject();
        CloudProject proj;
        proj.id = obj["id"].toInt();
        proj.name = obj["name"].toString();
        proj.description = obj["description"].toString();
        proj.createdAt = obj["created_at"].toString();
        result.append(proj);
    }

    return result;
}

CloudProject CloudClient::createProject(const QString& name, const QString& description, QString* errorMessage) {
    CloudProject result;
    QJsonObject reqObj;
    reqObj["name"] = name;
    reqObj["description"] = description;

    int statusCode = 0;
    QByteArray resData = executeSyncRequest(
        QStringLiteral("/api/v1/projects/"),
        QStringLiteral("POST"),
        QJsonDocument(reqObj).toJson(QJsonDocument::Compact),
        &statusCode,
        errorMessage
    );

    if (statusCode != 201 && statusCode != 200) {
        return result;
    }

    QJsonDocument doc = QJsonDocument::fromJson(resData);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        result.id = obj["id"].toInt();
        result.name = obj["name"].toString();
        result.description = obj["description"].toString();
        result.createdAt = obj["created_at"].toString();
    }
    return result;
}

bool CloudClient::deleteProject(int projectId, QString* errorMessage) {
    int statusCode = 0;
    executeSyncRequest(
        QString("/api/v1/projects/%1").arg(projectId),
        QStringLiteral("DELETE"),
        QByteArray(),
        &statusCode,
        errorMessage
    );
    return (statusCode == 204 || statusCode == 200);
}

QList<CloudDrawing> CloudClient::listDrawings(int projectId, QString* errorMessage) {
    QList<CloudDrawing> result;
    QString endpoint = QStringLiteral("/api/v1/drawings/");
    if (projectId > 0) {
        endpoint += QString("?project_id=%1").arg(projectId);
    }

    int statusCode = 0;
    QByteArray resData = executeSyncRequest(
        endpoint,
        QStringLiteral("GET"),
        QByteArray(),
        &statusCode,
        errorMessage
    );

    if (statusCode != 200) return result;

    QJsonDocument doc = QJsonDocument::fromJson(resData);
    if (!doc.isArray()) return result;

    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        if (!item.isObject()) continue;
        QJsonObject obj = item.toObject();
        CloudDrawing d;
        d.id = obj["id"].toInt();
        d.projectId = obj["project_id"].toInt();
        d.name = obj["name"].toString();
        d.format = obj["format"].toString();
        d.version = obj["version"].toInt();
        d.description = obj["description"].toString();
        d.updatedAt = obj["updated_at"].toString();
        result.append(d);
    }

    return result;
}

CloudDrawing CloudClient::getDrawing(int drawingId, QString* content, QString* errorMessage) {
    CloudDrawing result;
    int statusCode = 0;
    QByteArray resData = executeSyncRequest(
        QString("/api/v1/drawings/%1").arg(drawingId),
        QStringLiteral("GET"),
        QByteArray(),
        &statusCode,
        errorMessage
    );

    if (statusCode != 200) return result;

    QJsonDocument doc = QJsonDocument::fromJson(resData);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        result.id = obj["id"].toInt();
        result.projectId = obj["project_id"].toInt();
        result.name = obj["name"].toString();
        result.format = obj["format"].toString();
        result.version = obj["version"].toInt();
        result.description = obj["description"].toString();
        result.updatedAt = obj["updated_at"].toString();
        if (content) {
            *content = obj["content"].toString();
        }
    }
    return result;
}

CloudDrawing CloudClient::uploadDrawing(int projectId, const QString& name, const QString& content, const QString& format, const QString& description, QString* errorMessage) {
    CloudDrawing result;
    QJsonObject reqObj;
    reqObj["project_id"] = projectId;
    reqObj["name"] = name;
    reqObj["format"] = format;
    reqObj["content"] = content;
    reqObj["description"] = description;

    int statusCode = 0;
    QByteArray resData = executeSyncRequest(
        QStringLiteral("/api/v1/drawings/"),
        QStringLiteral("POST"),
        QJsonDocument(reqObj).toJson(QJsonDocument::Compact),
        &statusCode,
        errorMessage
    );

    if (statusCode != 201 && statusCode != 200) return result;

    QJsonDocument doc = QJsonDocument::fromJson(resData);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        result.id = obj["id"].toInt();
        result.projectId = obj["project_id"].toInt();
        result.name = obj["name"].toString();
        result.format = obj["format"].toString();
        result.version = obj["version"].toInt();
        result.description = obj["description"].toString();
        result.updatedAt = obj["updated_at"].toString();
    }
    return result;
}

CloudDrawing CloudClient::updateDrawing(int drawingId, const QString& content, const QString& commitMessage, QString* errorMessage) {
    CloudDrawing result;
    QJsonObject reqObj;
    reqObj["content"] = content;
    reqObj["commit_message"] = commitMessage;

    int statusCode = 0;
    QByteArray resData = executeSyncRequest(
        QString("/api/v1/drawings/%1").arg(drawingId),
        QStringLiteral("PUT"),
        QJsonDocument(reqObj).toJson(QJsonDocument::Compact),
        &statusCode,
        errorMessage
    );

    if (statusCode != 200) return result;

    QJsonDocument doc = QJsonDocument::fromJson(resData);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        result.id = obj["id"].toInt();
        result.projectId = obj["project_id"].toInt();
        result.name = obj["name"].toString();
        result.format = obj["format"].toString();
        result.version = obj["version"].toInt();
        result.description = obj["description"].toString();
        result.updatedAt = obj["updated_at"].toString();
    }
    return result;
}

bool CloudClient::deleteDrawing(int drawingId, QString* errorMessage) {
    int statusCode = 0;
    executeSyncRequest(
        QString("/api/v1/drawings/%1").arg(drawingId),
        QStringLiteral("DELETE"),
        QByteArray(),
        &statusCode,
        errorMessage
    );
    return (statusCode == 204 || statusCode == 200);
}

bool CloudClient::saveDocumentToCloud(int projectId, const QString& name, const CadDocument* doc, QString* errorMessage) {
    if (!doc) {
        if (errorMessage) *errorMessage = QStringLiteral("Invalid document pointer");
        return false;
    }
    QString jsonStr = GencadFile::toJson(doc);
    CloudDrawing drawing = uploadDrawing(projectId, name, jsonStr, QStringLiteral("gencad"), QStringLiteral("Uploaded from GenCAD Desktop"), errorMessage);
    return (drawing.id > 0);
}

bool CloudClient::updateDocumentToCloud(int drawingId, const CadDocument* doc, const QString& commitMessage, QString* errorMessage) {
    if (!doc) {
        if (errorMessage) *errorMessage = QStringLiteral("Invalid document pointer");
        return false;
    }
    QString jsonStr = GencadFile::toJson(doc);
    CloudDrawing drawing = updateDrawing(drawingId, jsonStr, commitMessage, errorMessage);
    return (drawing.id > 0);
}

bool CloudClient::loadDocumentFromCloud(int drawingId, CadDocument* doc, QString* errorMessage) {
    if (!doc) {
        if (errorMessage) *errorMessage = QStringLiteral("Invalid document pointer");
        return false;
    }
    QString content;
    CloudDrawing drawing = getDrawing(drawingId, &content, errorMessage);
    if (drawing.id == 0) return false;
    return GencadFile::fromJson(content, doc, errorMessage);
}

QList<CloudDrawingVersion> CloudClient::listDrawingVersions(int drawingId, QString* errorMessage) {
    QList<CloudDrawingVersion> result;
    int statusCode = 0;
    QByteArray resData = executeSyncRequest(
        QString("/api/v1/drawings/%1/versions").arg(drawingId),
        QStringLiteral("GET"),
        QByteArray(),
        &statusCode,
        errorMessage
    );

    if (statusCode != 200) return result;

    QJsonDocument doc = QJsonDocument::fromJson(resData);
    if (!doc.isArray()) return result;

    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        if (!item.isObject()) continue;
        QJsonObject obj = item.toObject();
        CloudDrawingVersion v;
        v.id = obj["id"].toInt();
        v.versionNumber = obj["version_number"].toInt();
        v.commitMessage = obj["commit_message"].toString();
        v.createdAt = obj["created_at"].toString();
        result.append(v);
    }
    return result;
}

bool CloudClient::revertDrawingVersion(int drawingId, int versionNumber, CadDocument* doc, QString* errorMessage) {
    int statusCode = 0;
    QByteArray resData = executeSyncRequest(
        QString("/api/v1/drawings/%1/revert/%2").arg(drawingId).arg(versionNumber),
        QStringLiteral("POST"),
        QByteArray(),
        &statusCode,
        errorMessage
    );

    if (statusCode != 200) return false;

    if (doc) {
        QJsonDocument resDoc = QJsonDocument::fromJson(resData);
        if (resDoc.isObject()) {
            QJsonObject obj = resDoc.object();
            QString content = obj["content"].toString();
            return GencadFile::fromJson(content, doc, errorMessage);
        }
    }
    return true;
}
