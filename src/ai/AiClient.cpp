#include "ai/AiClient.h"
#include "io/GencadFile.h"
#include <QEventLoop>
#include <QTimer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

AiClient::AiClient(QObject* parent)
    : QObject(parent)
{
}

void AiClient::setServerUrl(const QString& url) {
    m_serverUrl = url.trimmed();
    while (m_serverUrl.endsWith(QLatin1Char('/'))) {
        m_serverUrl.chop(1);
    }
}

QByteArray AiClient::executeSyncPost(const QString& endpoint, const QByteArray& data, int* statusCode, QString* errorMessage) {
    QString fullUrl = m_serverUrl + endpoint;
    QNetworkRequest request{QUrl(fullUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_netManager.post(request, data);

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
    timer.start(3000); // 3 second timeout for AI gateway ping before local fallback

    loop.exec();

    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode) {
        *statusCode = code;
    }

    QByteArray responseData = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Network Error (%1): %2").arg(code).arg(reply->errorString());
        }
        reply->deleteLater();
        return QByteArray();
    }

    reply->deleteLater();
    return responseData;
}

AiProposal AiClient::generate(const QString& prompt, const CadDocument* doc, QString* errorMessage) {
    if (!m_offlineMode) {
        QJsonObject reqObj;
        reqObj["prompt"] = prompt;
        if (doc) {
            reqObj["document"] = GencadFile::toJson(doc);
        }

        int code = 0;
        QString netErr;
        QByteArray resp = executeSyncPost(QStringLiteral("/api/v1/ai/generate"),
                                          QJsonDocument(reqObj).toJson(QJsonDocument::Compact),
                                          &code,
                                          &netErr);
        if (code == 200 && !resp.isEmpty()) {
            AiProposal proposal;
            QString parseErr;
            if (AiProposal::fromJsonString(QString::fromUtf8(resp), &proposal, &parseErr)) {
                emit generationFinished(proposal);
                return proposal;
            }
        }
    }

    // Fallback to local intelligent parser
    AiProposal localProposal = m_localEngine.generateFromPrompt(prompt, doc);
    emit generationFinished(localProposal);
    return localProposal;
}

AiAnalysisReport AiClient::analyze(const CadDocument* doc, QString* errorMessage) {
    if (!m_offlineMode && doc) {
        QJsonObject reqObj;
        reqObj["document"] = GencadFile::toJson(doc);

        int code = 0;
        QString netErr;
        QByteArray resp = executeSyncPost(QStringLiteral("/api/v1/ai/analyze"),
                                          QJsonDocument(reqObj).toJson(QJsonDocument::Compact),
                                          &code,
                                          &netErr);
        if (code == 200 && !resp.isEmpty()) {
            QJsonDocument resDoc = QJsonDocument::fromJson(resp);
            if (resDoc.isObject()) {
                QJsonObject root = resDoc.object();
                AiAnalysisReport report;
                report.totalEntities = root["total_entities"].toInt();
                report.totalWireLength = root["total_wire_length"].toDouble();

                // Duplicates
                QJsonArray dups = root["duplicates"].toArray();
                for (const auto& val : dups) {
                    QJsonObject d = val.toObject();
                    report.duplicates.push_back({
                        QUuid::fromString(d["primary_id"].toString()),
                        QUuid::fromString(d["duplicate_id"].toString()),
                        d["entity_type"].toString(),
                        d["description"].toString()
                    });
                }

                // Overlaps
                QJsonArray ovs = root["overlaps"].toArray();
                for (const auto& val : ovs) {
                    QJsonObject o = val.toObject();
                    report.overlaps.push_back({
                        QUuid::fromString(o["id1"].toString()),
                        QUuid::fromString(o["id2"].toString()),
                        o["description"].toString()
                    });
                }

                // Layer issues
                QJsonArray liss = root["layer_issues"].toArray();
                for (const auto& val : liss) {
                    QJsonObject l = val.toObject();
                    report.layerIssues.push_back({
                        QUuid::fromString(l["id"].toString()),
                        l["layer"].toString(),
                        l["description"].toString()
                    });
                }

                // Suggestions
                QJsonArray suggs = root["suggestions"].toArray();
                for (const auto& val : suggs) {
                    report.suggestions.append(val.toString());
                }

                emit analysisFinished(report);
                return report;
            }
        }
    }

    AiAnalysisReport report = m_localEngine.analyzeDocument(doc);
    emit analysisFinished(report);
    return report;
}

AiProposal AiClient::cleanup(const CadDocument* doc, QString* errorMessage) {
    if (!m_offlineMode && doc) {
        QJsonObject reqObj;
        reqObj["document"] = GencadFile::toJson(doc);

        int code = 0;
        QString netErr;
        QByteArray resp = executeSyncPost(QStringLiteral("/api/v1/ai/cleanup"),
                                          QJsonDocument(reqObj).toJson(QJsonDocument::Compact),
                                          &code,
                                          &netErr);
        if (code == 200 && !resp.isEmpty()) {
            AiProposal proposal;
            QString parseErr;
            if (AiProposal::fromJsonString(QString::fromUtf8(resp), &proposal, &parseErr)) {
                return proposal;
            }
        }
    }

    return m_localEngine.cleanupDocument(doc);
}

AiProposal AiClient::dimension(const CadDocument* doc, const std::vector<QUuid>& targetIds, QString* errorMessage) {
    if (!m_offlineMode && doc) {
        QJsonObject reqObj;
        reqObj["document"] = GencadFile::toJson(doc);
        QJsonArray idsArr;
        for (const auto& id : targetIds) {
            idsArr.append(id.toString(QUuid::WithoutBraces));
        }
        reqObj["target_ids"] = idsArr;

        int code = 0;
        QString netErr;
        QByteArray resp = executeSyncPost(QStringLiteral("/api/v1/ai/dimension"),
                                          QJsonDocument(reqObj).toJson(QJsonDocument::Compact),
                                          &code,
                                          &netErr);
        if (code == 200 && !resp.isEmpty()) {
            AiProposal proposal;
            QString parseErr;
            if (AiProposal::fromJsonString(QString::fromUtf8(resp), &proposal, &parseErr)) {
                return proposal;
            }
        }
    }

    return m_localEngine.dimensionDocument(doc, targetIds);
}

std::vector<RagQueryResult> AiClient::queryRag(const QString& question, QString* errorMessage) {
    if (!m_offlineMode) {
        QJsonObject reqObj;
        reqObj["query"] = question;

        int code = 0;
        QString netErr;
        QByteArray resp = executeSyncPost(QStringLiteral("/api/v1/ai/rag/query"),
                                          QJsonDocument(reqObj).toJson(QJsonDocument::Compact),
                                          &code,
                                          &netErr);
        if (code == 200 && !resp.isEmpty()) {
            QJsonDocument resDoc = QJsonDocument::fromJson(resp);
            if (resDoc.isObject() && resDoc.object().contains("results")) {
                std::vector<RagQueryResult> results;
                QJsonArray arr = resDoc.object()["results"].toArray();
                for (const auto& item : arr) {
                    QJsonObject o = item.toObject();
                    RagQueryResult r;
                    r.title = o["title"].toString();
                    r.category = o["category"].toString();
                    r.content = o["content"].toString();
                    r.score = o["score"].toDouble();
                    results.push_back(r);
                }
                return results;
            }
        }
    }

    return m_localEngine.ragEngine()->query(question);
}

QString AiClient::askRag(const QString& question) {
    auto results = queryRag(question);
    if (results.empty()) {
        return m_localEngine.ragEngine()->answerQuestion(question);
    }
    QString ans = QStringLiteral("### %1\n\n%2\n").arg(results[0].title, results[0].content);
    if (results.size() > 1) {
        ans += QStringLiteral("\n\n---\n**Related: %1**\n%2").arg(results[1].title, results[1].content);
    }
    return ans;
}
