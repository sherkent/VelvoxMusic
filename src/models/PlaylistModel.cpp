#include "models/PlaylistModel.h"

namespace Velvox {

PlaylistModel::PlaylistModel(DatabaseManager *database, QObject *parent)
    : QAbstractListModel(parent)
    , m_db(database)
{
    loadPlaylists();
}

int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_currentPlaylistId < 0 ? m_playlists.size() : m_tracks.size();
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (m_currentPlaylistId < 0) {
        if (index.row() < 0 || index.row() >= m_playlists.size())
            return QVariant();
        const Playlist &playlist = m_playlists.at(index.row());
        switch (role) {
        case IdRole:
            return playlist.id;
        case NameRole:
            return playlist.name;
        case TrackCountRole:
            return m_trackCounts.value(playlist.id, 0);
        default:
            return QVariant();
        }
    } else {
        if (index.row() < 0 || index.row() >= m_tracks.size())
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
        case DurationMsRole:
            return track.durationMs;
        case DurationTextRole:
            return formatDuration(track.durationMs);
        case CoverPathRole:
            return track.coverPath;
        default:
            return QVariant();
        }
    }
}

QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "playlistId";
    roles[NameRole] = "name";
    roles[TrackCountRole] = "trackCount";
    roles[FilePathRole] = "filePath";
    roles[TitleRole] = "title";
    roles[ArtistRole] = "artist";
    roles[AlbumRole] = "album";
    roles[DurationMsRole] = "durationMs";
    roles[DurationTextRole] = "durationText";
    roles[CoverPathRole] = "coverPath";
    return roles;
}

int PlaylistModel::currentPlaylistId() const
{
    return m_currentPlaylistId;
}

bool PlaylistModel::showingTracks() const
{
    return m_currentPlaylistId >= 0;
}

int PlaylistModel::count() const
{
    return m_currentPlaylistId < 0 ? m_playlists.size() : m_tracks.size();
}

void PlaylistModel::reload()
{
    const int oldCount = count();
    beginResetModel();
    if (m_currentPlaylistId < 0)
        loadPlaylists();
    else
        loadTracks(m_currentPlaylistId);
    endResetModel();
    if (oldCount != count())
        emit countChanged();
}

void PlaylistModel::setCurrentPlaylistId(int playlistId)
{
    if (m_currentPlaylistId == playlistId)
        return;

    const int oldCount = count();
    beginResetModel();
    m_currentPlaylistId = playlistId;
    if (m_currentPlaylistId >= 0)
        loadTracks(m_currentPlaylistId);
    else
        loadPlaylists();
    endResetModel();

    emit currentPlaylistIdChanged();
    emit showingTracksChanged();
    if (oldCount != count())
        emit countChanged();
}

int PlaylistModel::createPlaylist(const QString &name)
{
    if (!m_db || name.trimmed().isEmpty())
        return -1;

    int id = m_db->createPlaylist(name.trimmed());
    if (id >= 0) {
        reload();
        emit countChanged();
    }
    return id;
}

bool PlaylistModel::deletePlaylist(int playlistId)
{
    if (!m_db)
        return false;

    bool ok = m_db->deletePlaylist(playlistId);
    if (ok) {
        reload();
        emit countChanged();
    }
    return ok;
}

bool PlaylistModel::renamePlaylist(int playlistId, const QString &name)
{
    if (!m_db || name.trimmed().isEmpty())
        return false;

    bool ok = m_db->renamePlaylist(playlistId, name.trimmed());
    if (ok)
        reload();
    return ok;
}

bool PlaylistModel::addTrack(int playlistId, int trackId)
{
    if (!m_db || playlistId < 0 || trackId < 0)
        return false;

    QList<Track> tracks = m_db->getPlaylistTracks(playlistId);
    int position = tracks.size();
    const int oldCount = count();
    bool ok = m_db->addTrackToPlaylist(playlistId, trackId, position);
    if (ok && m_currentPlaylistId == playlistId)
        reload();
    if (m_currentPlaylistId == playlistId && oldCount != count())
        emit countChanged();
    return ok;
}

bool PlaylistModel::removeTrack(int playlistId, int trackId)
{
    if (!m_db || playlistId < 0 || trackId < 0)
        return false;

    const int oldCount = count();
    bool ok = m_db->removeTrackFromPlaylist(playlistId, trackId);
    if (ok && m_currentPlaylistId == playlistId)
        reload();
    if (m_currentPlaylistId == playlistId && oldCount != count())
        emit countChanged();
    return ok;
}

bool PlaylistModel::moveTrack(int fromRow, int toRow)
{
    if (!m_db || m_currentPlaylistId < 0)
        return false;
    if (fromRow == toRow || fromRow < 0 || toRow < 0 || fromRow >= m_tracks.size() || toRow >= m_tracks.size())
        return false;

    // Update local order first.
    Track track = m_tracks.takeAt(fromRow);
    m_tracks.insert(toRow, track);

    // Rewrite positions in the database.
    m_db->transaction();
    bool ok = true;
    for (int i = 0; i < m_tracks.size(); ++i) {
        if (!m_db->moveTrackInPlaylist(m_currentPlaylistId, m_tracks.at(i).id, i)) {
            ok = false;
            break;
        }
    }
    if (ok)
        m_db->commit();
    else
        m_db->rollback();

    emit dataChanged(index(0), index(m_tracks.size() - 1));
    return ok;
}

void PlaylistModel::backToPlaylists()
{
    setCurrentPlaylistId(-1);
}

void PlaylistModel::loadPlaylists()
{
    m_playlists = m_db ? m_db->getPlaylists() : QList<Playlist>{};
    m_tracks.clear();

    m_trackCounts.clear();
    if (m_db) {
        for (const Playlist &playlist : std::as_const(m_playlists)) {
            const int cnt = m_db->getPlaylistTracks(playlist.id).size();
            m_trackCounts.insert(playlist.id, cnt);
        }
    }
}

void PlaylistModel::loadTracks(int playlistId)
{
    m_tracks = m_db ? m_db->getPlaylistTracks(playlistId) : QList<Track>{};
}

QString PlaylistModel::formatDuration(qint64 ms)
{
    if (ms <= 0)
        return QStringLiteral("0:00");

    qint64 totalSeconds = ms / 1000;
    qint64 minutes = totalSeconds / 60;
    qint64 seconds = totalSeconds % 60;
    return QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
}

} // namespace Velvox
