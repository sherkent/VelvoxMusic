#include "models/LyricsModel.h"
#include "core/MetadataReader.h"

#include <QJsonArray>
#include <QJsonObject>

namespace Velvox {

LyricsModel::LyricsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int LyricsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_lines.size();
}

QVariant LyricsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_lines.size())
        return QVariant();

    const LyricLine &line = m_lines.at(index.row());
    switch (role) {
    case TimeRole:
        return line.timeMs;
    case TextRole:
        return line.text;
    case ActiveRole:
        return index.row() == m_currentIndex;
    case WordsRole: {
        QJsonArray array;
        for (const LyricWord &word : std::as_const(line.words)) {
            QJsonObject obj;
            obj.insert(QStringLiteral("timeMs"), word.timeMs);
            obj.insert(QStringLiteral("text"), word.text);
            array.append(obj);
        }
        return array.toVariantList();
    }
    case WordIndexRole:
        return (index.row() == m_currentIndex) ? m_currentWordIndex : -1;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> LyricsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TimeRole] = "timeMs";
    roles[TextRole] = "text";
    roles[ActiveRole] = "active";
    roles[WordsRole] = "words";
    roles[WordIndexRole] = "wordIndex";
    return roles;
}

QString LyricsModel::sourceFile() const
{
    return m_sourceFile;
}

int LyricsModel::currentIndex() const
{
    return m_currentIndex;
}

int LyricsModel::currentWordIndex() const
{
    return m_currentWordIndex;
}

bool LyricsModel::hasLyrics() const
{
    return !m_lines.isEmpty();
}

void LyricsModel::setSourceFile(const QString &filePath)
{
    if (m_sourceFile == filePath)
        return;

    m_sourceFile = filePath;
    loadLyrics();
    emit sourceFileChanged();
}

void LyricsModel::setEmbeddedLyrics(const QByteArray &data)
{
    if (m_embeddedLyrics == data)
        return;

    m_embeddedLyrics = data;
    loadLyrics();
}

void LyricsModel::setPosition(qint64 positionMs)
{
    if (m_lines.isEmpty())
        return;

    int newIndex = -1;
    for (int i = 0; i < m_lines.size(); ++i) {
        if (m_lines.at(i).timeMs <= positionMs)
            newIndex = i;
        else
            break;
    }

    int newWordIndex = -1;
    if (newIndex >= 0 && newIndex < m_lines.size()) {
        const QList<LyricWord> &words = m_lines.at(newIndex).words;
        for (int i = 0; i < words.size(); ++i) {
            if (words.at(i).timeMs <= positionMs)
                newWordIndex = i;
            else
                break;
        }
    }

    bool indexChanged = newIndex != m_currentIndex;
    bool wordIndexChanged = newWordIndex != m_currentWordIndex;

    if (indexChanged || wordIndexChanged) {
        const int oldIndex = m_currentIndex;
        m_currentIndex = newIndex;
        m_currentWordIndex = newWordIndex;

        if (oldIndex >= 0 && oldIndex != m_currentIndex)
            emit dataChanged(index(oldIndex), index(oldIndex), {ActiveRole, WordIndexRole});
        if (m_currentIndex >= 0)
            emit dataChanged(index(m_currentIndex), index(m_currentIndex), {ActiveRole, WordIndexRole});

        if (indexChanged)
            emit currentIndexChanged();
        if (wordIndexChanged)
            emit currentWordIndexChanged();
    }
}

void LyricsModel::loadLyrics()
{
    beginResetModel();
    m_lines = LyricsParser::loadForTrack(m_sourceFile, m_embeddedLyrics);
    m_currentIndex = -1;
    m_currentWordIndex = -1;
    endResetModel();
    emit hasLyricsChanged();
    emit currentIndexChanged();
    emit currentWordIndexChanged();
}

void LyricsModel::loadForTrack(const QString &filePath)
{
    if (m_sourceFile == filePath && !m_embeddedLyrics.isEmpty())
        return;

    m_sourceFile = filePath;
    m_embeddedLyrics = MetadataReader::extractLyrics(filePath);
    loadLyrics();
    emit sourceFileChanged();
}

} // namespace Velvox
