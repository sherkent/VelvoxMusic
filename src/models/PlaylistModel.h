#pragma once

#include "core/DatabaseManager.h"
#include "core/Playlist.h"
#include "core/Track.h"

#include <QAbstractListModel>
#include <QString>

namespace Velvox {

/**
 * @brief Model for playlists and the tracks inside the selected playlist.
 *
 * The model has two modes:
 * - Playlist list mode (default): rows are playlists.
 * - Track mode: rows are tracks inside `m_currentPlaylistId`.
 *
 * This keeps the QML side simple while avoiding two separate models at the
 * cost of a reset when switching between modes.
 */
class PlaylistModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int currentPlaylistId READ currentPlaylistId WRITE setCurrentPlaylistId NOTIFY currentPlaylistIdChanged)
    Q_PROPERTY(bool showingTracks READ showingTracks NOTIFY showingTracksChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TrackCountRole,
        FilePathRole,
        TitleRole,
        ArtistRole,
        AlbumRole,
        DurationMsRole,
        DurationTextRole,
        CoverPathRole
    };
    Q_ENUM(Roles)

    explicit PlaylistModel(DatabaseManager *database, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int currentPlaylistId() const;
    bool showingTracks() const;
    int count() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE void setCurrentPlaylistId(int playlistId);
    Q_INVOKABLE int createPlaylist(const QString &name);
    Q_INVOKABLE bool deletePlaylist(int playlistId);
    Q_INVOKABLE bool renamePlaylist(int playlistId, const QString &name);
    Q_INVOKABLE bool addTrack(int playlistId, int trackId);
    Q_INVOKABLE bool removeTrack(int playlistId, int trackId);
    Q_INVOKABLE bool moveTrack(int fromRow, int toRow);
    Q_INVOKABLE void backToPlaylists();

signals:
    void currentPlaylistIdChanged();
    void showingTracksChanged();
    void countChanged();

private:
    void loadPlaylists();
    void loadTracks(int playlistId);
    static QString formatDuration(qint64 ms);

    DatabaseManager *m_db = nullptr;
    QList<Playlist> m_playlists;
    QList<Track> m_tracks;
    QHash<int, int> m_trackCounts;
    int m_currentPlaylistId = -1;
};

} // namespace Velvox
