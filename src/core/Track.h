#pragma once

#include <QString>
#include <QDateTime>

namespace Velvox {

/**
 * @brief Plain data object representing a single music track.
 *
 * This struct is intentionally simple (no QObject) so it can be safely copied
 * between the database layer, the scanner thread and the UI models.
 */
struct Track
{
    int id = -1;
    QString filePath;
    QString title;
    QString artist;
    QString album;
    QString albumArtist;
    QString genre;
    int year = 0;
    int trackNumber = 0;
    qint64 durationMs = 0;
    QString coverPath;
    QDateTime dateAdded;
    int playCount = 0;
    qint64 fileMtime = 0;

    bool isValid() const { return id >= 0; }

    bool operator==(const Track &other) const
    {
        return id == other.id && filePath == other.filePath;
    }

    bool operator!=(const Track &other) const
    {
        return !(*this == other);
    }
};

/**
 * @brief Aggregated album information derived from tracks.
 */
struct Album
{
    QString name;
    QString albumArtist;
    QString coverPath;
    int year = 0;
    int trackCount = 0;
    qint64 totalDurationMs = 0;

    bool isValid() const { return !name.isEmpty(); }

    bool operator==(const Album &other) const
    {
        return name == other.name && albumArtist == other.albumArtist;
    }

    bool operator!=(const Album &other) const
    {
        return !(*this == other);
    }
};

/**
 * @brief Aggregated artist information derived from tracks.
 */
struct Artist
{
    QString name;
    QString coverPath;
    int trackCount = 0;
    int albumCount = 0;

    bool isValid() const { return !name.isEmpty(); }

    bool operator==(const Artist &other) const
    {
        return name == other.name;
    }

    bool operator!=(const Artist &other) const
    {
        return !(*this == other);
    }
};

} // namespace Velvox
