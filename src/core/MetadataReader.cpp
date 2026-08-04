#include "core/MetadataReader.h"

// TagLib is used under the LGPL. See MetadataReader.h and README.md for the
// dynamic-linking / redistribution notice.
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>

#include <taglib/mpegfile.h>
#include <taglib/flacfile.h>
#include <taglib/mp4file.h>
#include <taglib/vorbisfile.h>
#include <taglib/xiphcomment.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/id3v2tag.h>
#include <taglib/flacpicture.h>
#include <taglib/mp4coverart.h>
#include <taglib/mp4item.h>
#include <taglib/unsynchronizedlyricsframe.h>
#include <taglib/synchronizedlyricsframe.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImage>

namespace Velvox {

namespace {

QString toQString(const TagLib::String &str)
{
    if (str.isEmpty())
        return QString();
    // to8Bit(true) preserves UTF-8 encoding instead of falling back to Latin-1.
    return QString::fromUtf8(str.to8Bit(true).c_str());
}

// Convert an ID3v2 SYLT frame to plain LRC text. TagLib 2.x exposes the
// entries as a SynchedTextList instead of a single text() string.
QString syncedLyricsToLrc(const TagLib::ID3v2::SynchronizedLyricsFrame *frame)
{
    if (!frame)
        return QString();

    QString lrc;
    const auto entries = frame->synchedText();
    for (const auto &entry : entries) {
        const qint64 ms = static_cast<qint64>(entry.time);
        const int minutes = static_cast<int>(ms / 60000);
        const int seconds = static_cast<int>((ms % 60000) / 1000);
        const int hundredths = static_cast<int>((ms % 1000) / 10);
        const QString timeStamp =
            QString("%1:%2.%3")
                .arg(minutes, 2, 10, QLatin1Char('0'))
                .arg(seconds, 2, 10, QLatin1Char('0'))
                .arg(hundredths, 2, 10, QLatin1Char('0'));
        lrc += QString("[%1]%2\n").arg(timeStamp, toQString(entry.text));
    }
    return lrc;
}

} // anonymous namespace

MetadataReader::MetadataReader(const QString &coverCacheDir, QObject *parent)
    : QObject(parent)
    , m_coverCacheDir(coverCacheDir)
{
}

QString MetadataReader::coverCacheDir() const
{
    return m_coverCacheDir;
}

void MetadataReader::setCoverCacheDir(const QString &dir)
{
    m_coverCacheDir = dir;
}

Track MetadataReader::readTrack(const QString &filePath) const
{
    Track track;
    track.filePath = filePath;

    QFileInfo fi(filePath);
    track.fileMtime = fi.lastModified().toMSecsSinceEpoch();

    // Fallback title from filename if tags are empty.
    track.title = fi.completeBaseName();

    TagLib::FileRef ref(filePath.toUtf8().constData());
    if (ref.isNull())
        return track;

    if (ref.tag()) {
        TagLib::Tag *tag = ref.tag();
        QString t = toQString(tag->title());
        if (!t.isEmpty())
            track.title = t;
        track.artist = toQString(tag->artist());
        track.album = toQString(tag->album());
        track.genre = toQString(tag->genre());
        track.year = static_cast<int>(tag->year());
        track.trackNumber = static_cast<int>(tag->track());
    }

    if (ref.audioProperties()) {
        TagLib::AudioProperties *props = ref.audioProperties();
        track.durationMs = static_cast<qint64>(props->lengthInMilliseconds());
    }

    // Extract, scale and cache the embedded cover.
    QByteArray coverData = extractCoverData(filePath);
    if (!coverData.isEmpty()) {
        track.coverPath = saveCoverToCache(coverData, filePath);
    }

    return track;
}

QByteArray MetadataReader::extractCoverData(const QString &filePath)
{
    QByteArray data;
    const std::string path = filePath.toUtf8().constData();

    // 1. MP3 / ID3v2 attached picture
    {
        TagLib::MPEG::File file(path.c_str());
        if (file.isValid() && file.ID3v2Tag()) {
            const auto &map = file.ID3v2Tag()->frameListMap();
            auto it = map.find("APIC");
            if (it != map.end()) {
                for (auto *frame : it->second) {
                    auto *pic = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(frame);
                    if (!pic)
                        continue;
                    if (pic->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
                        data = QByteArray(pic->picture().data(), static_cast<int>(pic->picture().size()));
                        break;
                    }
                    if (data.isEmpty()) {
                        data = QByteArray(pic->picture().data(), static_cast<int>(pic->picture().size()));
                    }
                }
            }
        }
        if (!data.isEmpty())
            return data;
    }

    // 2. FLAC picture blocks
    {
        TagLib::FLAC::File file(path.c_str());
        if (file.isValid()) {
            const auto &pics = file.pictureList();
            if (!pics.isEmpty()) {
                for (auto *pic : pics) {
                    if (pic->type() == TagLib::FLAC::Picture::FrontCover) {
                        data = QByteArray(pic->data().data(), static_cast<int>(pic->data().size()));
                        break;
                    }
                    if (data.isEmpty()) {
                        data = QByteArray(pic->data().data(), static_cast<int>(pic->data().size()));
                    }
                }
            }
        }
        if (!data.isEmpty())
            return data;
    }

    // 3. MP4 / M4A cover art
    {
        TagLib::MP4::File file(path.c_str());
        if (file.isValid() && file.tag()) {
            auto items = file.tag()->itemMap();
            auto it = items.find("covr");
            if (it != items.end()) {
                TagLib::MP4::CoverArtList covers = it->second.toCoverArtList();
                if (!covers.isEmpty()) {
                    data = QByteArray(covers.front().data().data(), static_cast<int>(covers.front().data().size()));
                }
            }
        }
        if (!data.isEmpty())
            return data;
    }

    // 4. Ogg Vorbis / XiphComment METADATA_BLOCK_PICTURE
    {
        TagLib::Ogg::Vorbis::File file(path.c_str());
        if (file.isValid() && file.tag()) {
            auto pics = file.tag()->pictureList();
            if (!pics.isEmpty()) {
                for (auto *pic : pics) {
                    if (pic->type() == TagLib::FLAC::Picture::FrontCover) {
                        data = QByteArray(pic->data().data(), static_cast<int>(pic->data().size()));
                        break;
                    }
                    if (data.isEmpty()) {
                        data = QByteArray(pic->data().data(), static_cast<int>(pic->data().size()));
                    }
                }
            }
        }
        if (!data.isEmpty())
            return data;
    }

    return data;
}

QByteArray MetadataReader::extractLyrics(const QString &filePath)
{
    QByteArray data;
    const std::string path = filePath.toUtf8().constData();

    // 1. MP3 / ID3v2 unsynchronized lyrics (USLT) or synchronized lyrics (SYLT).
    {
        TagLib::MPEG::File file(path.c_str());
        if (file.isValid() && file.ID3v2Tag()) {
            const auto &map = file.ID3v2Tag()->frameListMap();
            auto it = map.find("USLT");
            if (it != map.end() && !it->second.isEmpty()) {
                auto *frame = dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame *>(it->second.front());
                if (frame)
                    data = QByteArray(frame->text().to8Bit(true).data(), static_cast<int>(frame->text().to8Bit(true).size()));
            }
            if (data.isEmpty()) {
                auto sylt = map.find("SYLT");
                if (sylt != map.end() && !sylt->second.isEmpty()) {
                    auto *frame = dynamic_cast<TagLib::ID3v2::SynchronizedLyricsFrame *>(sylt->second.front());
                    if (frame) {
                        const QString lrc = syncedLyricsToLrc(frame);
                        data = lrc.toUtf8();
                    }
                }
            }
        }
        if (!data.isEmpty())
            return data;
    }

    // 2. FLAC / VorbisComment LYRICS field.
    {
        TagLib::FLAC::File file(path.c_str());
        if (file.isValid() && file.xiphComment()) {
            auto lyrics = file.xiphComment()->fieldListMap()["LYRICS"];
            if (!lyrics.isEmpty()) {
                auto s = lyrics.front();
                data = QByteArray(s.to8Bit(true).data(), static_cast<int>(s.to8Bit(true).size()));
            }
        }
        if (!data.isEmpty())
            return data;
    }

    // 3. MP4 / M4A ©lyr box.
    {
        TagLib::MP4::File file(path.c_str());
        if (file.isValid() && file.tag()) {
            auto items = file.tag()->itemMap();
            auto it = items.find("\xC2\xA9lyr"); // "©lyr"
            if (it != items.end()) {
                auto s = it->second.toStringList().toString();
                data = QByteArray(s.to8Bit(true).data(), static_cast<int>(s.to8Bit(true).size()));
            }
        }
        if (!data.isEmpty())
            return data;
    }

    // 4. Ogg Vorbis / XiphComment LYRICS field.
    {
        TagLib::Ogg::Vorbis::File file(path.c_str());
        if (file.isValid() && file.tag()) {
            auto lyrics = file.tag()->fieldListMap()["LYRICS"];
            if (!lyrics.isEmpty()) {
                auto s = lyrics.front();
                data = QByteArray(s.to8Bit(true).data(), static_cast<int>(s.to8Bit(true).size()));
            }
        }
    }

    return data;
}

QString MetadataReader::saveCoverToCache(const QByteArray &coverData, const QString &filePath) const
{
    if (coverData.isEmpty() || m_coverCacheDir.isEmpty())
        return QString();

    QDir dir(m_coverCacheDir);
    if (!dir.exists() && !dir.mkpath("."))
        return QString();

    // Use MD5 of the file path as the cache filename so each track has a
    // deterministic cache entry. We overwrite it on rescan so updated covers
    // are refreshed without leaking old files.
    const QString hash = QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex();
    const QString coverPath = dir.filePath(hash + ".jpg");

    QImage image;
    if (!image.loadFromData(coverData))
        return QString();

    // Limit to 400x400 to keep the cache small and QML image provider fast.
    const QImage scaled = image.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (!scaled.save(coverPath, "JPEG", 90))
        return QString();

    return coverPath;
}

} // namespace Velvox
