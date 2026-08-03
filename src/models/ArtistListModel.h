#pragma once

#include "core/DatabaseManager.h"
#include "core/Track.h"

#include <QAbstractListModel>
#include <QString>

namespace Velvox {

/**
 * @brief List model exposing aggregated artist information from the database.
 */
class ArtistListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        CoverPathRole,
        TrackCountRole,
        AlbumCountRole
    };
    Q_ENUM(Roles)

    explicit ArtistListModel(DatabaseManager *database, QObject *parent = nullptr);

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
    QList<Artist> m_artists;
};

} // namespace Velvox
