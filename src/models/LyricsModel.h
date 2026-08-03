#pragma once

#include "core/LyricsParser.h"

#include <QAbstractListModel>
#include <QString>

namespace Velvox {

/**
 * @brief Exposes parsed lyric lines to QML and tracks the current active line.
 */
class LyricsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString sourceFile READ sourceFile WRITE setSourceFile NOTIFY sourceFileChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int currentWordIndex READ currentWordIndex NOTIFY currentWordIndexChanged)
    Q_PROPERTY(bool hasLyrics READ hasLyrics NOTIFY hasLyricsChanged)

public:
    enum Roles {
        TimeRole = Qt::UserRole + 1,
        TextRole,
        ActiveRole,
        WordsRole,
        WordIndexRole
    };
    Q_ENUM(Roles)

    explicit LyricsModel(QObject *parent = nullptr);

    Q_INVOKABLE void loadForTrack(const QString &filePath);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString sourceFile() const;
    int currentIndex() const;
    int currentWordIndex() const;
    bool hasLyrics() const;

public slots:
    void setSourceFile(const QString &filePath);
    void setEmbeddedLyrics(const QByteArray &data);
    void setPosition(qint64 positionMs);

signals:
    void sourceFileChanged();
    void currentIndexChanged();
    void currentWordIndexChanged();
    void hasLyricsChanged();

private:
    void loadLyrics();

    QList<LyricLine> m_lines;
    QString m_sourceFile;
    QByteArray m_embeddedLyrics;
    int m_currentIndex = -1;
    int m_currentWordIndex = -1;
};

} // namespace Velvox
