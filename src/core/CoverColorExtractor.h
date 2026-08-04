#pragma once

#include <QObject>
#include <QVariantList>
#include <QColor>
#include <QImage>

namespace Velvox {

class DatabaseManager;

/**
 * @brief Extracts a small palette of representative colors from a cover image.
 *
 * The extraction runs on the global thread pool so the UI stays responsive.
 * Results are cached in the local database by track id, so the same cover is
 * only analyzed once.
 */
class CoverColorExtractor : public QObject
{
    Q_OBJECT

public:
    explicit CoverColorExtractor(DatabaseManager *db, QObject *parent = nullptr);

    /**
     * @brief Starts extraction for the given track.
     *
     * If cached colors exist they are emitted immediately on the calling thread.
     * Otherwise the image is loaded and analyzed in the background and
     * @p extractionFinished is emitted from this object's thread.
     */
    Q_INVOKABLE void extract(int trackId, const QString &coverPath);

signals:
    void extractionFinished(int trackId, QVariantList colors);

private:
    void onFinished(int trackId, const QVariantList &colors);
    static QVariantList extractColors(const QString &coverPath);

    DatabaseManager *m_db = nullptr;
};

} // namespace Velvox
