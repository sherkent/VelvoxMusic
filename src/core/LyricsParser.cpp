#include "core/LyricsParser.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QTextCodec>
#include <QDebug>
#include <algorithm>

namespace Velvox {

const QRegularExpression LyricsParser::s_timeTagRegex(
    QStringLiteral(R"(\[(\d{1,2}):(\d{2})(?:\.(\d{2,3}))?\])"));

// Word-level timestamps use angle brackets: <mm:ss.xx>
const QRegularExpression LyricsParser::s_wordTagRegex(
    QStringLiteral(R"(<(\d{1,2}):(\d{2})(?:\.(\d{2,3}))?>)"));

const QRegularExpression &LyricsParser::wordTagRegex()
{
    return s_wordTagRegex;
}

static QList<LyricWord> parseWords(const QString &lineText, qint64 lineTimeMs)
{
    QList<LyricWord> words;
    QRegularExpressionMatchIterator it = LyricsParser::wordTagRegex().globalMatch(lineText);

    // If no word tags, return the whole line as a single word.
    if (!it.hasNext()) {
        words.append({lineTimeMs, lineText.trimmed()});
        return words;
    }

    // If there is leading text before the first word tag, treat it as the
    // first word starting at the line time.
    const QRegularExpressionMatch firstMatch = it.peekNext();
    if (firstMatch.capturedStart() > 0) {
        QString leading = lineText.left(firstMatch.capturedStart()).trimmed();
        if (!leading.isEmpty())
            words.append({lineTimeMs, leading});
    }

    qint64 lastTimeMs = lineTimeMs;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        qint64 wordTimeMs = LyricsParser::parseTimeStamp(match.captured(0));
        if (wordTimeMs < 0)
            wordTimeMs = lastTimeMs;

        // Text between this tag and the next tag belongs to this word.
        int tagEnd = match.capturedEnd();
        int nextTagStart = lineText.length();
        QRegularExpressionMatchIterator nextIt = LyricsParser::wordTagRegex().globalMatch(lineText);
        while (nextIt.hasNext()) {
            QRegularExpressionMatch nextMatch = nextIt.next();
            if (nextMatch.capturedStart() > tagEnd) {
                nextTagStart = nextMatch.capturedStart();
                break;
            }
        }

        QString wordText = lineText.mid(tagEnd, nextTagStart - tagEnd).trimmed();
        if (!wordText.isEmpty())
            words.append({wordTimeMs, wordText});
        lastTimeMs = wordTimeMs;
    }

    return words;
}

QList<LyricLine> LyricsParser::parseLrc(const QString &content)
{
    QList<LyricLine> lines;
    const QStringList rawLines = content.split(QLatin1Char('\n'));

    for (const QString &line : rawLines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;

        QRegularExpressionMatchIterator it = s_timeTagRegex.globalMatch(trimmed);
        QString text = trimmed;

        // Remove line-level time tags from the text portion.
        text.remove(s_timeTagRegex);
        text = text.trimmed();

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            qint64 ms = parseTimeStamp(match.captured(0));
            if (ms >= 0) {
                LyricLine lyric;
                lyric.timeMs = ms;
                lyric.text = text;
                lyric.words = parseWords(text, ms);
                lines.append(std::move(lyric));
            }
        }
    }

    std::sort(lines.begin(), lines.end(), [](const LyricLine &a, const LyricLine &b) {
        return a.timeMs < b.timeMs;
    });

    return lines;
}

QList<LyricLine> LyricsParser::parseLrcFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QByteArray data = file.readAll();
    file.close();

    // Try UTF-8 first, fall back to system locale, then UTF-16 LE/BE.
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::ConverterState state;
    QString content = codec->toUnicode(data.constData(), data.size(), &state);
    if (state.invalidChars > 0) {
        codec = QTextCodec::codecForLocale();
        content = codec->toUnicode(data);
    }

    return parseLrc(content);
}

QList<LyricLine> LyricsParser::parseEmbeddedLyrics(const QByteArray &rawData)
{
    if (rawData.isEmpty())
        return {};

    // Prefer parsing as LRC if it contains time tags.
    QString text = QString::fromUtf8(rawData);
    if (text.contains(s_timeTagRegex))
        return parseLrc(text);

    // Otherwise treat the whole text as one unsynchronized block at time 0.
    return {{0, text}};
}

QList<LyricLine> LyricsParser::loadForTrack(const QString &audioFilePath,
                                            const QByteArray &embeddedLyrics)
{
    if (audioFilePath.isEmpty())
        return {};

    QFileInfo info(audioFilePath);
    QString lrcPath = info.path() + QLatin1Char('/') + info.completeBaseName() + QLatin1String(".lrc");

    if (QFile::exists(lrcPath)) {
        auto lines = parseLrcFile(lrcPath);
        if (!lines.isEmpty())
            return lines;
    }

    if (!embeddedLyrics.isEmpty())
        return parseEmbeddedLyrics(embeddedLyrics);

    return {};
}

qint64 LyricsParser::parseTimeStamp(const QString &timeString)
{
    // Accept both line tags [mm:ss.xx] and word tags <mm:ss.xx>.
    static const QRegularExpression timeRegex(
        QStringLiteral(R"([\[<](\d{1,2}):(\d{2})(?:\.(\d{2,3}))?[\]>])"));

    QRegularExpressionMatch match = timeRegex.match(timeString);
    if (!match.hasMatch())
        return -1;

    int minutes = match.captured(1).toInt();
    int seconds = match.captured(2).toInt();
    QString frac = match.captured(3);

    int millis = 0;
    if (!frac.isEmpty()) {
        if (frac.length() == 2)
            millis = frac.toInt() * 10;
        else if (frac.length() == 3)
            millis = frac.toInt();
    }

    return (minutes * 60 + seconds) * 1000LL + millis;
}

} // namespace Velvox
