#include "models/AlbumListModel.h"

namespace Velvox {

AlbumListModel::AlbumListModel(DatabaseManager *database, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(database)
{
    reload();
}

int AlbumListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_albums.size();
}

QVariant AlbumListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_albums.size())
        return QVariant();

    const Album &album = m_albums.at(index.row());
    switch (role) {
    case NameRole:
        return album.name;
    case AlbumArtistRole:
        return album.albumArtist;
    case CoverPathRole:
        return album.coverPath;
    case YearRole:
        return album.year;
    case TrackCountRole:
        return album.trackCount;
    case TotalDurationMsRole:
        return album.totalDurationMs;
    case TotalDurationTextRole:
        return formatDuration(album.totalDurationMs);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> AlbumListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[AlbumArtistRole] = "albumArtist";
    roles[CoverPathRole] = "coverPath";
    roles[YearRole] = "year";
    roles[TrackCountRole] = "trackCount";
    roles[TotalDurationMsRole] = "totalDurationMs";
    roles[TotalDurationTextRole] = "totalDurationText";
    return roles;
}

void AlbumListModel::reload()
{
    beginResetModel();
    m_albums = m_db->getAllAlbums();
    endResetModel();
    emit countChanged();
}

QVariantList AlbumListModel::tracksAt(int row) const
{
    QVariantList result;
    if (row < 0 || row >= m_albums.size())
        return result;

    const Album &album = m_albums.at(row);
    const QList<Track> tracks = m_db->getTracksByAlbum(album.name, album.albumArtist);
    for (const Track &track : tracks) {
        QVariantMap map;
        map[QStringLiteral("trackId")] = track.id;
        map[QStringLiteral("filePath")] = track.filePath;
        map[QStringLiteral("title")] = track.title;
        map[QStringLiteral("artist")] = track.artist;
        map[QStringLiteral("album")] = track.album;
        map[QStringLiteral("durationMs")] = track.durationMs;
        map[QStringLiteral("durationText")] = formatDuration(track.durationMs);
        map[QStringLiteral("coverPath")] = track.coverPath;
        result.append(map);
    }
    return result;
}

int AlbumListModel::count() const
{
    return m_albums.size();
}

QString AlbumListModel::formatDuration(qint64 ms)
{
    if (ms <= 0)
        return QStringLiteral("0:00");

    const qint64 totalSeconds = ms / 1000;
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;
    return QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
}

} // namespace Velvox
