#pragma once

#include "core/DatabaseManager.h"
#include "core/Track.h"

#include <QAbstractListModel>
#include <QString>

namespace Velvox {

/**
 * @brief List model exposing aggregated album information from the database.
 */
class AlbumListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        AlbumArtistRole,
        CoverPathRole,
        YearRole,
        TrackCountRole,
        TotalDurationMsRole,
        TotalDurationTextRole
    };
    Q_ENUM(Roles)

    explicit AlbumListModel(DatabaseManager *database, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void reload();
    Q_INVOKABLE QVariantList tracksAt(int row) const;

    int count() const;

signals:
    void countChanged();

private:
    static QString formatDuration(qint64 ms);

    DatabaseManager *m_db = nullptr;
    QList<Album> m_albums;
};

} // namespace Velvox
