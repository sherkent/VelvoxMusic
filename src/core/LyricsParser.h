#pragma once

#include <QString>
#include <QList>
#include <QRegularExpression>

namespace Velvox {

/**
 * @brief Single word inside a lyric line, with its own timestamp.
 */
struct LyricWord
{
    qint64 timeMs = -1;
    QString text;

    bool isValid() const { return timeMs >= 0; }
};

/**
 * @brief Single lyric line with a timestamp in milliseconds.
 *
 * For enhanced LRC, the line is split into words so the UI can highlight
 * individual words as playback progresses.
 */
struct LyricLine
{
    qint64 timeMs = -1;
    QString text;
    QList<LyricWord> words;

    bool isValid() const { return timeMs >= 0; }
};

/**
 * @brief Parses LRC files and embedded lyrics.
 *
 * Supported formats:
 * - [mm:ss.xx] lyrics
 * - [mm:ss.xxx] lyrics
 * - [mm:ss] lyrics
 * - TagLib USLT (unsynchronized lyrics) is treated as a single block at time 0.
 */
class LyricsParser
{
public:
    static QList<LyricLine> parseLrc(const QString &content);
    static QList<LyricLine> parseLrcFile(const QString &filePath);
    static QList<LyricLine> parseEmbeddedLyrics(const QByteArray &rawData);

    /**
     * @brief Try to load lyrics for an audio file.
     *
     * Looks for a same-name .lrc file next to the audio file first, then falls
     * back to embedded lyrics if available.
     */
    static QList<LyricLine> loadForTrack(const QString &audioFilePath,
                                         const QByteArray &embeddedLyrics = QByteArray());

    /**
     * @brief Parse a LRC time tag such as [mm:ss.xx] or <mm:ss.xx> into ms.
     */
    static qint64 parseTimeStamp(const QString &timeString);

    /**
     * @brief Regex used to detect word-level <mm:ss.xx> tags.
     */
    static const QRegularExpression &wordTagRegex();

private:
    static const QRegularExpression s_timeTagRegex;
    static const QRegularExpression s_wordTagRegex;
};

} // namespace Velvox
