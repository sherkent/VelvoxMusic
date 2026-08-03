#pragma once

#include "core/Playlist.h"
#include "core/Track.h"

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

namespace Velvox {

/**
 * @brief Local SQLite database access layer.
 *
 * Manages the connection, schema migration, and CRUD operations for tracks,
 * playlists, favorites and recent play history.
 */
class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(const QString &dbPath, QObject *parent = nullptr);
    explicit DatabaseManager(const QString &dbPath, const QString &connectionName, QObject *parent = nullptr);
    ~DatabaseManager() override;

    bool initialize();
    bool isOpen() const;
    QString dbPath() const;

    // Tracks
    bool addOrUpdateTrack(const Track &track);
    bool removeTrack(int id);
    bool removeTracksNotIn(const QStringList &filePaths);
    Track getTrackById(int id);
    Track getTrackByPath(const QString &filePath);
    QList<Track> getAllTracks(const QString &orderBy = "date_added DESC");
    QList<Track> searchTracks(const QString &keyword, const QString &orderBy = "date_added DESC");
    int trackCount() const;

    // Playlists
    int createPlaylist(const QString &name);
    bool deletePlaylist(int id);
    bool renamePlaylist(int id, const QString &name);
    QList<Playlist> getPlaylists();
    QList<Track> getPlaylistTracks(int playlistId);
    bool addTrackToPlaylist(int playlistId, int trackId, int position);
    bool removeTrackFromPlaylist(int playlistId, int trackId);
    bool moveTrackInPlaylist(int playlistId, int trackId, int newPosition);

    // Favorites
    bool setFavorite(int trackId, bool favorite);
    bool isFavorite(int trackId) const;
    QList<Track> getFavorites();

    // Recent plays
    bool addRecentPlay(int trackId);
    QList<Track> getRecentPlays(int limit = 50);

    // Albums / Artists
    QList<Album> getAllAlbums();
    QList<Artist> getAllArtists();
    QList<Track> getTracksByAlbum(const QString &album, const QString &albumArtist);
    QList<Track> getTracksByArtist(const QString &artist);

    // Transaction helpers
    bool transaction();
    bool commit();
    bool rollback();

private:
    bool createSchema();
    Track trackFromQuery(const QSqlQuery &query) const;

    QSqlDatabase m_db;
    QString m_dbPath;
    QString m_connectionName;
    bool m_initialized = false;
};

} // namespace Velvox
