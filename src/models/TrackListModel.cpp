#include "models/TrackListModel.h"

namespace Velvox {

TrackListModel::TrackListModel(DatabaseManager *database, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(database)
{
    reload();
}

int TrackListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_tracks.size();
}

QVariant TrackListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tracks.size())
        return QVariant();

    const Track &track = m_tracks.at(index.row());

    switch (role) {
    case IdRole:
        return track.id;
    case FilePathRole:
        return track.filePath;
    case TitleRole:
        return track.title;
    case ArtistRole:
        return track.artist;
    case AlbumRole:
        return track.album;
    case GenreRole:
        return track.genre;
    case YearRole:
        return track.year;
    case TrackNumberRole:
        return track.trackNumber;
    case DurationMsRole:
        return track.durationMs;
    case DurationTextRole:
        return formatDuration(track.durationMs);
    case CoverPathRole:
        return track.coverPath;
    case DateAddedRole:
        return track.dateAdded;
    case PlayCountRole:
        return track.playCount;
    case IsFavoriteRole:
        return m_db->isFavorite(track.id);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> TrackListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "trackId";
    roles[FilePathRole] = "filePath";
    roles[TitleRole] = "title";
    roles[ArtistRole] = "artist";
    roles[AlbumRole] = "album";
    roles[GenreRole] = "genre";
    roles[YearRole] = "year";
    roles[TrackNumberRole] = "trackNumber";
    roles[DurationMsRole] = "durationMs";
    roles[DurationTextRole] = "durationText";
    roles[CoverPathRole] = "coverPath";
    roles[DateAddedRole] = "dateAdded";
    roles[PlayCountRole] = "playCount";
    roles[IsFavoriteRole] = "isFavorite";
    return roles;
}

void TrackListModel::reload()
{
    beginResetModel();
    m_tracks = m_db->getAllTracks();
    endResetModel();
    emit countChanged();
}

void TrackListModel::toggleFavorite(int row)
{
    if (row < 0 || row >= m_tracks.size())
        return;

    const int id = m_tracks.at(row).id;
    const bool favorite = m_db->isFavorite(id);
    m_db->setFavorite(id, !favorite);

    emit dataChanged(index(row), index(row), {IsFavoriteRole});
}

void TrackListModel::removeTrack(int row)
{
    if (row < 0 || row >= m_tracks.size())
        return;

    const int id = m_tracks.at(row).id;
    beginRemoveRows(QModelIndex(), row, row);
    m_db->removeTrack(id);
    m_tracks.removeAt(row);
    endRemoveRows();
    emit countChanged();
}

int TrackListModel::trackIdAt(int row) const
{
    if (row < 0 || row >= m_tracks.size())
        return -1;
    return m_tracks.at(row).id;
}

int TrackListModel::count() const
{
    return m_tracks.size();
}

QString TrackListModel::formatDuration(qint64 ms)
{
    if (ms <= 0)
        return QStringLiteral("0:00");

    const qint64 totalSeconds = ms / 1000;
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;
    return QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
}

} // namespace Velvox
