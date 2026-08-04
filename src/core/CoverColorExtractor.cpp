#include "core/CoverColorExtractor.h"
#include "core/DatabaseManager.h"

#include <QDebug>
#include <QFileInfo>
#include <QMetaObject>
#include <QRunnable>
#include <QThreadPool>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

namespace Velvox {

namespace {

struct Bin {
    int h = 0;
    int s = 0;
    int v = 0;
    int count = 0;
    qint64 r = 0;
    qint64 g = 0;
    qint64 b = 0;

    QColor averageColor() const
    {
        if (count <= 0)
            return QColor();
        return QColor(static_cast<int>(r / count),
                      static_cast<int>(g / count),
                      static_cast<int>(b / count));
    }
};

inline void rgbToHsv(int r, int g, int b, qreal &h, qreal &s, qreal &v)
{
    const qreal rd = r / 255.0;
    const qreal gd = g / 255.0;
    const qreal bd = b / 255.0;
    const qreal max = std::max({rd, gd, bd});
    const qreal min = std::min({rd, gd, bd});
    const qreal delta = max - min;

    v = max;
    s = (max > 0.0) ? (delta / max) : 0.0;

    if (delta < 1e-6) {
        h = 0.0;
    } else if (max == rd) {
        h = 60.0 * std::fmod((gd - bd) / delta + 6.0, 6.0);
    } else if (max == gd) {
        h = 60.0 * ((bd - rd) / delta + 2.0);
    } else {
        h = 60.0 * ((rd - gd) / delta + 4.0);
    }
}

inline int colorDistanceSquared(const QColor &a, const QColor &b)
{
    const int dr = a.red() - b.red();
    const int dg = a.green() - b.green();
    const int db = a.blue() - b.blue();
    return dr * dr + dg * dg + db * db;
}

QColor imageAverageColor(const QImage &image)
{
    qint64 r = 0, g = 0, b = 0;
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QRgb px = image.pixel(x, y);
            r += qRed(px);
            g += qGreen(px);
            b += qBlue(px);
            ++count;
        }
    }
    if (count == 0)
        return QColor("#2a2a40");
    return QColor(static_cast<int>(r / count),
                  static_cast<int>(g / count),
                  static_cast<int>(b / count));
}

} // anonymous namespace

CoverColorExtractor::CoverColorExtractor(DatabaseManager *db, QObject *parent)
    : QObject(parent)
    , m_db(db)
{
}

void CoverColorExtractor::extract(int trackId, const QString &coverPath)
{
    if (trackId < 0 || coverPath.isEmpty()) {
        emit extractionFinished(trackId, QVariantList());
        return;
    }

    // Return cached palette immediately if available.
    if (m_db) {
        const QList<QColor> cached = m_db->getCoverColors(trackId);
        if (!cached.isEmpty()) {
            QVariantList list;
            list.reserve(cached.size());
            for (const QColor &c : cached)
                list.append(c);
            emit extractionFinished(trackId, list);
            return;
        }
    }

    QThreadPool::globalInstance()->start(QRunnable::create([this, trackId, coverPath]() {
        const QVariantList colors = extractColors(coverPath);
        QMetaObject::invokeMethod(this, [this, trackId, colors]() {
            onFinished(trackId, colors);
        }, Qt::QueuedConnection);
    }));
}

void CoverColorExtractor::onFinished(int trackId, const QVariantList &colors)
{
    if (m_db) {
        QList<QColor> saveColors;
        saveColors.reserve(colors.size());
        for (const QVariant &v : colors)
            saveColors.append(v.value<QColor>());
        m_db->setCoverColors(trackId, saveColors);
    }
    emit extractionFinished(trackId, colors);
}

QVariantList CoverColorExtractor::extractColors(const QString &coverPath)
{
    QVariantList result;
    if (!QFileInfo::exists(coverPath))
        return result;

    QImage image(coverPath);
    if (image.isNull()) {
        qWarning() << "CoverColorExtractor: failed to load image" << coverPath;
        return result;
    }

    // Downsample before analysis to keep the work cheap.
    if (image.width() > 40 || image.height() > 40)
        image = image.scaled(40, 40, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    // Build a coarse HSV histogram while skipping blown-out or crushed pixels.
    constexpr int H_BINS = 12;
    constexpr int S_BINS = 4;
    constexpr int V_BINS = 4;

    std::vector<Bin> bins;
    bins.reserve(H_BINS * S_BINS * V_BINS);
    for (int h = 0; h < H_BINS; ++h) {
        for (int s = 0; s < S_BINS; ++s) {
            for (int v = 0; v < V_BINS; ++v) {
                bins.push_back({h, s, v, 0, 0, 0, 0});
            }
        }
    }

    auto binIndex = [](int h, int s, int v) {
        return h * S_BINS * V_BINS + s * V_BINS + v;
    };

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QRgb px = image.pixel(x, y);
            const int r = qRed(px);
            const int g = qGreen(px);
            const int b = qBlue(px);

            qreal hue, sat, val;
            rgbToHsv(r, g, b, hue, sat, val);

            // Skip near-white, near-black and near-gray pixels.
            if (val < 0.15 || val > 0.90 || sat < 0.12)
                continue;

            const int h = static_cast<int>(hue / 360.0 * H_BINS) % H_BINS;
            const int s = static_cast<int>(sat * S_BINS);
            const int v = static_cast<int>(val * V_BINS);
            const int idx = binIndex(h, qBound(0, s, S_BINS - 1), qBound(0, v, V_BINS - 1));

            Bin &bin = bins[idx];
            bin.count += 1;
            bin.r += r;
            bin.g += g;
            bin.b += b;
        }
    }

    std::sort(bins.begin(), bins.end(), [](const Bin &a, const Bin &b) {
        return a.count > b.count;
    });

    // Pick the top bins, requiring reasonable color separation.
    constexpr int MIN_DISTANCE_SQUARED = 60 * 60;
    std::vector<QColor> palette;
    palette.reserve(5);

    for (const Bin &bin : bins) {
        if (bin.count <= 0)
            break;
        const QColor candidate = bin.averageColor();
        bool tooClose = false;
        for (const QColor &existing : palette) {
            if (colorDistanceSquared(candidate, existing) < MIN_DISTANCE_SQUARED) {
                tooClose = true;
                break;
            }
        }
        if (!tooClose) {
            palette.push_back(candidate);
            if (palette.size() >= 4)
                break;
        }
    }

    // If the cover is mostly washed out or grayscale, fall back to the image average.
    const QColor average = imageAverageColor(image);
    while (palette.size() < 4)
        palette.push_back(average);

    // Build a dark, moody base from the most dominant color so blobs stand out.
    const QColor dominant = palette.empty() ? average : palette.front();
    QColor base = dominant;
    int baseH, baseS, baseV, baseA;
    base.getHsv(&baseH, &baseS, &baseV, &baseA);
    baseV = qMax(8, baseV / 2);
    base.setHsv(baseH, baseS, baseV, baseA);

    result.reserve(5);
    result.append(base);
    for (const QColor &c : palette)
        result.append(c);
    return result;
}

} // namespace Velvox
