#pragma once

#include "core/DatabaseManager.h"
#include "core/Track.h"

#include <QAbstractListModel>
#include <QString>

namespace Velvox {

/**
 * @brief Source list model exposing all tracks from the local database.
 *
 * This model holds the raw track list and delegates sorting/filtering to
 * TrackListProxyModel. Keeping the source model free of filter/sort state
 * avoids full-model resets when the user searches or changes sort order.
 */
class TrackListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        FilePathRole,
        TitleRole,
        ArtistRole,
        AlbumRole,
        GenreRole,
        YearRole,
        TrackNumberRole,
        DurationMsRole,
        DurationTextRole,
        CoverPathRole,
        DateAddedRole,
        PlayCountRole,
        IsFavoriteRole
    };
    Q_ENUM(Roles)

    explicit TrackListModel(DatabaseManager *database, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void reload();
    Q_INVOKABLE void toggleFavorite(int row);
    Q_INVOKABLE void removeTrack(int row);
    Q_INVOKABLE int trackIdAt(int row) const;

    int count() const;

signals:
    void countChanged();

private:
    static QString formatDuration(qint64 ms);

    DatabaseManager *m_db = nullptr;
    QList<Track> m_tracks;
};

} // namespace Velvox
