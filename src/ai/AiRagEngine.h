#pragma once

#include <QString>
#include <QStringList>
#include <vector>
#include <QJsonObject>
#include <QJsonArray>

struct RagArticle {
    QString title;
    QString category; // e.g., "Tools", "Standards", "Shortcuts", "Best Practices"
    QString content;
    QStringList keywords;
};

struct RagQueryResult {
    QString title;
    QString category;
    QString content;
    double score{0.0};

    QJsonObject toJson() const;
};

class AiRagEngine {
public:
    AiRagEngine();

    std::vector<RagQueryResult> query(const QString& question, int maxResults = 3) const;
    QString answerQuestion(const QString& question) const;

    const std::vector<RagArticle>& articles() const { return m_knowledgeBase; }

private:
    void populateKnowledgeBase();
    double calculateScore(const QStringList& queryTokens, const RagArticle& article) const;

    std::vector<RagArticle> m_knowledgeBase;
};
