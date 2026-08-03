#include "models/ArtistListModel.h"

namespace Velvox {

ArtistListModel::ArtistListModel(DatabaseManager *database, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(database)
{
    reload();
}

int ArtistListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_artists.size();
}

QVariant ArtistListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_artists.size())
        return QVariant();

    const Artist &artist = m_artists.at(index.row());
    switch (role) {
    case NameRole:
        return artist.name;
    case CoverPathRole:
        return artist.coverPath;
    case TrackCountRole:
        return artist.trackCount;
    case AlbumCountRole:
        return artist.albumCount;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ArtistListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[CoverPathRole] = "coverPath";
    roles[TrackCountRole] = "trackCount";
    roles[AlbumCountRole] = "albumCount";
    return roles;
}

void ArtistListModel::reload()
{
    beginResetModel();
    m_artists = m_db->getAllArtists();
    endResetModel();
    emit countChanged();
}

QVariantList ArtistListModel::tracksAt(int row) const
{
    QVariantList result;
    if (row < 0 || row >= m_artists.size())
        return result;

    const Artist &artist = m_artists.at(row);
    const QList<Track> tracks = m_db->getTracksByArtist(artist.name);
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

int ArtistListModel::count() const
{
    return m_artists.size();
}

QString ArtistListModel::formatDuration(qint64 ms)
{
    if (ms <= 0)
        return QStringLiteral("0:00");

    const qint64 totalSeconds = ms / 1000;
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;
    return QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
}

} // namespace Velvox
