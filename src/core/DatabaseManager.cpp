#include "core/DatabaseManager.h"

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace Velvox {

DatabaseManager::DatabaseManager(const QString &dbPath, QObject *parent)
    : QObject(parent)
    , m_dbPath(dbPath)
    , m_connectionName(QSqlDatabase::defaultConnection)
{
}

DatabaseManager::DatabaseManager(const QString &dbPath, const QString &connectionName, QObject *parent)
    : QObject(parent)
    , m_dbPath(dbPath)
    , m_connectionName(connectionName)
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    if (!m_connectionName.isEmpty() && QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool DatabaseManager::initialize()
{
    if (m_initialized)
        return true;

    QDir dir = QFileInfo(m_dbPath).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        qWarning() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    // Enable foreign keys and WAL mode for better concurrency.
    QSqlQuery pragma(m_db);
    pragma.exec("PRAGMA foreign_keys = ON");
    pragma.exec("PRAGMA journal_mode = WAL");

    if (!createSchema()) {
        qWarning() << "Failed to create database schema";
        return false;
    }

    m_initialized = true;
    return true;
}

bool DatabaseManager::isOpen() const
{
    return m_db.isOpen();
}

QString DatabaseManager::dbPath() const
{
    return m_dbPath;
}

bool DatabaseManager::createSchema()
{
    QSqlQuery query(m_db);

    const QStringList statements = {
        R"(
            CREATE TABLE IF NOT EXISTS tracks (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                file_path TEXT NOT NULL UNIQUE,
                title TEXT,
                artist TEXT,
                album TEXT,
                album_artist TEXT,
                genre TEXT,
                year INTEGER,
                track_number INTEGER,
                duration_ms INTEGER,
                cover_path TEXT,
                date_added INTEGER,
                play_count INTEGER DEFAULT 0,
                file_mtime INTEGER
            )
        )",
        R"(
            CREATE INDEX IF NOT EXISTS idx_tracks_artist ON tracks(artist)
        )",
        R"(
            CREATE INDEX IF NOT EXISTS idx_tracks_album ON tracks(album)
        )",
        R"(
            CREATE INDEX IF NOT EXISTS idx_tracks_title ON tracks(title)
        )",

        R"(
            CREATE TABLE IF NOT EXISTS playlists (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                date_created INTEGER,
                date_modified INTEGER
            )
        )",

        R"(
            CREATE TABLE IF NOT EXISTS playlist_tracks (
                playlist_id INTEGER NOT NULL,
                track_id INTEGER NOT NULL,
                position INTEGER NOT NULL,
                PRIMARY KEY (playlist_id, track_id),
                FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE,
                FOREIGN KEY (track_id) REFERENCES tracks(id) ON DELETE CASCADE
            )
        )",
        R"(
            CREATE INDEX IF NOT EXISTS idx_playlist_tracks ON playlist_tracks(playlist_id, position)
        )",

        R"(
            CREATE TABLE IF NOT EXISTS favorites (
                track_id INTEGER PRIMARY KEY,
                date_added INTEGER,
                FOREIGN KEY (track_id) REFERENCES tracks(id) ON DELETE CASCADE
            )
        )",

        R"(
            CREATE TABLE IF NOT EXISTS recent_plays (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                track_id INTEGER NOT NULL,
                played_at INTEGER,
                FOREIGN KEY (track_id) REFERENCES tracks(id) ON DELETE CASCADE
            )
        )",
        R"(
            CREATE INDEX IF NOT EXISTS idx_recent_plays ON recent_plays(played_at DESC)
        )",
        R"(
            CREATE TABLE IF NOT EXISTS track_cover_colors (
                track_id INTEGER PRIMARY KEY,
                color0 TEXT,
                color1 TEXT,
                color2 TEXT,
                color3 TEXT,
                color4 TEXT,
                updated_at INTEGER,
                FOREIGN KEY (track_id) REFERENCES tracks(id) ON DELETE CASCADE
            )
        )"
    };

    for (const QString &stmt : statements) {
        if (!query.exec(stmt)) {
            qWarning() << "Schema error:" << query.lastError().text() << stmt;
            return false;
        }
    }

    return true;
}

Track DatabaseManager::trackFromQuery(const QSqlQuery &query) const
{
    Track track;
    track.id = query.value("id").toInt();
    track.filePath = query.value("file_path").toString();
    track.title = query.value("title").toString();
    track.artist = query.value("artist").toString();
    track.album = query.value("album").toString();
    track.albumArtist = query.value("album_artist").toString();
    track.genre = query.value("genre").toString();
    track.year = query.value("year").toInt();
    track.trackNumber = query.value("track_number").toInt();
    track.durationMs = query.value("duration_ms").toLongLong();
    track.coverPath = query.value("cover_path").toString();
    track.dateAdded = QDateTime::fromMSecsSinceEpoch(query.value("date_added").toLongLong());
    track.playCount = query.value("play_count").toInt();
    track.fileMtime = query.value("file_mtime").toLongLong();
    return track;
}

bool DatabaseManager::addOrUpdateTrack(const Track &track)
{
    QSqlQuery query(m_db);

    // Check existing record by file path.
    query.prepare("SELECT id, file_mtime FROM tracks WHERE file_path = ?");
    query.addBindValue(track.filePath);
    if (!query.exec()) {
        qWarning() << "addOrUpdateTrack select error:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        int existingId = query.value(0).toInt();
        qint64 existingMtime = query.value(1).toLongLong();

        if (existingMtime == track.fileMtime) {
            // File has not changed; nothing to do.
            return true;
        }

        QSqlQuery update(m_db);
        update.prepare(R"(
            UPDATE tracks SET
                title = ?, artist = ?, album = ?, album_artist = ?, genre = ?,
                year = ?, track_number = ?, duration_ms = ?, cover_path = ?,
                file_mtime = ?
            WHERE id = ?
        )");
        update.addBindValue(track.title);
        update.addBindValue(track.artist);
        update.addBindValue(track.album);
        update.addBindValue(track.albumArtist);
        update.addBindValue(track.genre);
        update.addBindValue(track.year);
        update.addBindValue(track.trackNumber);
        update.addBindValue(track.durationMs);
        update.addBindValue(track.coverPath);
        update.addBindValue(track.fileMtime);
        update.addBindValue(existingId);

        if (!update.exec()) {
            qWarning() << "addOrUpdateTrack update error:" << update.lastError().text();
            return false;
        }
        return true;
    }

    // Insert new record.
    QSqlQuery insert(m_db);
    insert.prepare(R"(
        INSERT INTO tracks
            (file_path, title, artist, album, album_artist, genre, year,
             track_number, duration_ms, cover_path, date_added, play_count, file_mtime)
        VALUES
            (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    insert.addBindValue(track.filePath);
    insert.addBindValue(track.title);
    insert.addBindValue(track.artist);
    insert.addBindValue(track.album);
    insert.addBindValue(track.albumArtist);
    insert.addBindValue(track.genre);
    insert.addBindValue(track.year);
    insert.addBindValue(track.trackNumber);
    insert.addBindValue(track.durationMs);
    insert.addBindValue(track.coverPath);
    insert.addBindValue(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    insert.addBindValue(track.playCount);
    insert.addBindValue(track.fileMtime);

    if (!insert.exec()) {
        qWarning() << "addOrUpdateTrack insert error:" << insert.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::removeTrack(int id)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM tracks WHERE id = ?");
    query.addBindValue(id);
    return query.exec();
}

bool DatabaseManager::removeTracksNotIn(const QStringList &filePaths)
{
    if (filePaths.isEmpty()) {
        QSqlQuery query(m_db);
        return query.exec("DELETE FROM tracks");
    }

    // Build a temporary table of valid paths for an efficient DELETE ... NOT IN.
    QSqlQuery query(m_db);
    if (!query.exec("CREATE TEMP TABLE IF NOT EXISTS temp_valid_paths (path TEXT PRIMARY KEY)")) {
        qWarning() << "temp table create error:" << query.lastError().text();
        return false;
    }
    if (!query.exec("DELETE FROM temp_valid_paths")) {
        qWarning() << "temp table clear error:" << query.lastError().text();
        return false;
    }

    m_db.transaction();

    QSqlQuery insert(m_db);
    insert.prepare("INSERT OR IGNORE INTO temp_valid_paths (path) VALUES (?)");
    for (const QString &path : filePaths) {
        insert.addBindValue(path);
        if (!insert.exec()) {
            qWarning() << "temp table insert error:" << insert.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    QSqlQuery del(m_db);
    if (!del.exec("DELETE FROM tracks WHERE file_path NOT IN (SELECT path FROM temp_valid_paths)")) {
        qWarning() << "removeTracksNotIn error:" << del.lastError().text();
        m_db.rollback();
        return false;
    }

    return m_db.commit();
}

Track DatabaseManager::getTrackById(int id)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM tracks WHERE id = ?");
    query.addBindValue(id);
    if (query.exec() && query.next()) {
        return trackFromQuery(query);
    }
    return Track();
}

Track DatabaseManager::getTrackByPath(const QString &filePath)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM tracks WHERE file_path = ?");
    query.addBindValue(filePath);
    if (query.exec() && query.next()) {
        return trackFromQuery(query);
    }
    return Track();
}

QList<Track> DatabaseManager::getAllTracks(const QString &orderBy)
{
    QList<Track> tracks;
    QSqlQuery query(m_db);
    QString sql = QStringLiteral("SELECT * FROM tracks ORDER BY %1").arg(orderBy);
    if (query.exec(sql)) {
        while (query.next()) {
            tracks.append(trackFromQuery(query));
        }
    } else {
        qWarning() << "getAllTracks error:" << query.lastError().text();
    }
    return tracks;
}

QList<Track> DatabaseManager::searchTracks(const QString &keyword, const QString &orderBy)
{
    QList<Track> tracks;
    if (keyword.isEmpty())
        return getAllTracks(orderBy);

    QSqlQuery query(m_db);
    QString pattern = QStringLiteral("%%%1%%").arg(keyword);
    QString sql = R"(
        SELECT * FROM tracks
        WHERE title LIKE ? OR artist LIKE ? OR album LIKE ?
        ORDER BY )" + orderBy;

    query.prepare(sql);
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    query.addBindValue(pattern);

    if (query.exec()) {
        while (query.next()) {
            tracks.append(trackFromQuery(query));
        }
    } else {
        qWarning() << "searchTracks error:" << query.lastError().text();
    }
    return tracks;
}

int DatabaseManager::trackCount() const
{
    QSqlQuery query(m_db);
    if (query.exec("SELECT COUNT(*) FROM tracks") && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int DatabaseManager::createPlaylist(const QString &name)
{
    QSqlQuery query(m_db);
    qint64 now = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    query.prepare("INSERT INTO playlists (name, date_created, date_modified) VALUES (?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(now);
    query.addBindValue(now);
    if (!query.exec()) {
        qWarning() << "createPlaylist error:" << query.lastError().text();
        return -1;
    }
    return query.lastInsertId().toInt();
}

bool DatabaseManager::deletePlaylist(int id)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM playlists WHERE id = ?");
    query.addBindValue(id);
    return query.exec();
}

bool DatabaseManager::renamePlaylist(int id, const QString &name)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE playlists SET name = ?, date_modified = ? WHERE id = ?");
    query.addBindValue(name);
    query.addBindValue(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    query.addBindValue(id);
    return query.exec();
}

QList<Playlist> DatabaseManager::getPlaylists()
{
    QList<Playlist> playlists;
    QSqlQuery query(m_db);
    if (query.exec("SELECT * FROM playlists ORDER BY date_created DESC")) {
        while (query.next()) {
            Playlist p;
            p.id = query.value("id").toInt();
            p.name = query.value("name").toString();
            p.dateCreated = QDateTime::fromMSecsSinceEpoch(query.value("date_created").toLongLong());
            p.dateModified = QDateTime::fromMSecsSinceEpoch(query.value("date_modified").toLongLong());
            playlists.append(p);
        }
    }
    return playlists;
}

QList<Track> DatabaseManager::getPlaylistTracks(int playlistId)
{
    QList<Track> tracks;
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT t.* FROM tracks t
        JOIN playlist_tracks pt ON t.id = pt.track_id
        WHERE pt.playlist_id = ?
        ORDER BY pt.position
    )");
    query.addBindValue(playlistId);
    if (query.exec()) {
        while (query.next()) {
            tracks.append(trackFromQuery(query));
        }
    }
    return tracks;
}

bool DatabaseManager::addTrackToPlaylist(int playlistId, int trackId, int position)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO playlist_tracks (playlist_id, track_id, position) VALUES (?, ?, ?)");
    query.addBindValue(playlistId);
    query.addBindValue(trackId);
    query.addBindValue(position);
    return query.exec();
}

bool DatabaseManager::removeTrackFromPlaylist(int playlistId, int trackId)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM playlist_tracks WHERE playlist_id = ? AND track_id = ?");
    query.addBindValue(playlistId);
    query.addBindValue(trackId);
    return query.exec();
}

bool DatabaseManager::moveTrackInPlaylist(int playlistId, int trackId, int newPosition)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE playlist_tracks SET position = ? WHERE playlist_id = ? AND track_id = ?");
    query.addBindValue(newPosition);
    query.addBindValue(playlistId);
    query.addBindValue(trackId);
    return query.exec();
}

bool DatabaseManager::setFavorite(int trackId, bool favorite)
{
    if (favorite) {
        QSqlQuery query(m_db);
        query.prepare("INSERT OR IGNORE INTO favorites (track_id, date_added) VALUES (?, ?)");
        query.addBindValue(trackId);
        query.addBindValue(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
        return query.exec();
    } else {
        QSqlQuery query(m_db);
        query.prepare("DELETE FROM favorites WHERE track_id = ?");
        query.addBindValue(trackId);
        return query.exec();
    }
}

bool DatabaseManager::isFavorite(int trackId) const
{
    QSqlQuery query(m_db);
    query.prepare("SELECT 1 FROM favorites WHERE track_id = ?");
    query.addBindValue(trackId);
    return query.exec() && query.next();
}

QList<Track> DatabaseManager::getFavorites()
{
    QList<Track> tracks;
    QSqlQuery query(m_db);
    if (query.exec(R"(
        SELECT t.* FROM tracks t
        JOIN favorites f ON t.id = f.track_id
        ORDER BY f.date_added DESC
    )")) {
        while (query.next()) {
            tracks.append(trackFromQuery(query));
        }
    }
    return tracks;
}

bool DatabaseManager::addRecentPlay(int trackId)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO recent_plays (track_id, played_at) VALUES (?, ?)");
    query.addBindValue(trackId);
    query.addBindValue(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    if (!query.exec()) {
        return false;
    }

    // Keep only the most recent 100 entries.
    QSqlQuery cleanup(m_db);
    cleanup.prepare(R"(
        DELETE FROM recent_plays
        WHERE id NOT IN (
            SELECT id FROM recent_plays ORDER BY played_at DESC LIMIT 100
        )
    )");
    cleanup.exec();
    return true;
}

QList<Track> DatabaseManager::getRecentPlays(int limit)
{
    QList<Track> tracks;
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT t.* FROM tracks t
        JOIN recent_plays r ON t.id = r.track_id
        ORDER BY r.played_at DESC
        LIMIT ?
    )");
    query.addBindValue(limit);
    if (query.exec()) {
        while (query.next()) {
            tracks.append(trackFromQuery(query));
        }
    }
    return tracks;
}

QList<QColor> DatabaseManager::getCoverColors(int trackId)
{
    QList<QColor> colors;
    QSqlQuery query(m_db);
    query.prepare("SELECT color0, color1, color2, color3, color4 FROM track_cover_colors WHERE track_id = ?");
    query.addBindValue(trackId);
    if (query.exec() && query.next()) {
        for (int i = 0; i < 5; ++i) {
            const QString value = query.value(i).toString();
            if (!value.isEmpty())
                colors.append(QColor(value));
        }
    }
    return colors;
}

bool DatabaseManager::setCoverColors(int trackId, const QList<QColor> &colors)
{
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT OR REPLACE INTO track_cover_colors
            (track_id, color0, color1, color2, color3, color4, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");
    query.addBindValue(trackId);
    for (int i = 0; i < 5; ++i) {
        if (i < colors.size() && colors.at(i).isValid())
            query.addBindValue(colors.at(i).name());
        else
            query.addBindValue(QString());
    }
    query.addBindValue(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());

    if (!query.exec()) {
        qWarning() << "setCoverColors error:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<Album> DatabaseManager::getAllAlbums()
{
    QList<Album> albums;
    QSqlQuery query(m_db);
    if (query.exec(R"(
        SELECT
            album,
            album_artist,
            MAX(year) AS year,
            COUNT(*) AS track_count,
            SUM(duration_ms) AS total_duration_ms,
            MAX(cover_path) AS cover_path
        FROM tracks
        WHERE album IS NOT NULL AND album <> ''
        GROUP BY album, album_artist
        ORDER BY album_artist, album
    )")) {
        while (query.next()) {
            Album album;
            album.name = query.value("album").toString();
            album.albumArtist = query.value("album_artist").toString();
            album.year = query.value("year").toInt();
            album.trackCount = query.value("track_count").toInt();
            album.totalDurationMs = query.value("total_duration_ms").toLongLong();
            album.coverPath = query.value("cover_path").toString();
            albums.append(album);
        }
    } else {
        qWarning() << "getAllAlbums error:" << query.lastError().text();
    }
    return albums;
}

QList<Artist> DatabaseManager::getAllArtists()
{
    QList<Artist> artists;
    QSqlQuery query(m_db);
    if (query.exec(R"(
        SELECT
            artist,
            COUNT(*) AS track_count,
            COUNT(DISTINCT album) AS album_count,
            MAX(cover_path) AS cover_path
        FROM tracks
        WHERE artist IS NOT NULL AND artist <> ''
        GROUP BY artist
        ORDER BY artist
    )")) {
        while (query.next()) {
            Artist artist;
            artist.name = query.value("artist").toString();
            artist.trackCount = query.value("track_count").toInt();
            artist.albumCount = query.value("album_count").toInt();
            artist.coverPath = query.value("cover_path").toString();
            artists.append(artist);
        }
    } else {
        qWarning() << "getAllArtists error:" << query.lastError().text();
    }
    return artists;
}

QList<Track> DatabaseManager::getTracksByAlbum(const QString &album, const QString &albumArtist)
{
    QList<Track> tracks;
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT * FROM tracks
        WHERE album = ? AND (album_artist = ? OR (? = '' AND (album_artist IS NULL OR album_artist = '')))
        ORDER BY track_number, file_path
    )");
    query.addBindValue(album);
    query.addBindValue(albumArtist);
    query.addBindValue(albumArtist);
    if (query.exec()) {
        while (query.next()) {
            tracks.append(trackFromQuery(query));
        }
    } else {
        qWarning() << "getTracksByAlbum error:" << query.lastError().text();
    }
    return tracks;
}

QList<Track> DatabaseManager::getTracksByArtist(const QString &artist)
{
    QList<Track> tracks;
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT * FROM tracks
        WHERE artist = ? OR album_artist = ?
        ORDER BY album, track_number, file_path
    )");
    query.addBindValue(artist);
    query.addBindValue(artist);
    if (query.exec()) {
        while (query.next()) {
            tracks.append(trackFromQuery(query));
        }
    } else {
        qWarning() << "getTracksByArtist error:" << query.lastError().text();
    }
    return tracks;
}

bool DatabaseManager::transaction()
{
    return m_db.transaction();
}

bool DatabaseManager::commit()
{
    return m_db.commit();
}

bool DatabaseManager::rollback()
{
    return m_db.rollback();
}

} // namespace Velvox
