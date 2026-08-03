#pragma once

#include "core/Track.h"

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QString>

namespace Velvox {

/**
 * @brief Reads audio metadata using TagLib.
 *
 * IMPORTANT: TagLib is licensed under the GNU Lesser General Public License
 * (LGPL). Velvox Music links to TagLib dynamically. When distributing this
 * application you must ship the TagLib shared library (e.g. taglib.dll on
 * Windows) together with the application and provide the TagLib license text.
 */
class MetadataReader : public QObject
{
    Q_OBJECT

public:
    explicit MetadataReader(const QString &coverCacheDir, QObject *parent = nullptr);

    Track readTrack(const QString &filePath) const;

    QString coverCacheDir() const;
    void setCoverCacheDir(const QString &dir);

    /**
     * @brief Extract the front cover picture from the file.
     * @return Raw cover bytes, or empty if none is found.
     */
    static QByteArray extractCoverData(const QString &filePath);

    /**
     * @brief Extract embedded lyrics (USLT/SYLTCOMM/USF lyrics) from the file.
     * @return Raw lyrics bytes, or empty if none is found.
     */
    static QByteArray extractLyrics(const QString &filePath);

    /**
     * @brief Scale and save a cover to the cache directory.
     * @return Path to the cached cover image, or empty on failure.
     */
    QString saveCoverToCache(const QByteArray &coverData, const QString &filePath) const;

private:
    QString m_coverCacheDir;
};

} // namespace Velvox
