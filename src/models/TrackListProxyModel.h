#pragma once

#include "models/TrackListModel.h"

#include <QSortFilterProxyModel>
#include <QString>

namespace Velvox {

/**
 * @brief Sort/filter proxy model that wraps TrackListModel.
 *
 * Replaces the previous full-model reset approach with QSortFilterProxyModel's
 * incremental insert/remove/move notifications, keeping the ListView delegate
 * pool stable while the user types a search query or changes sort order.
 *
 * All QML-visible methods/properties from the old TrackListModel are mirrored
 * here so that existing QML code continues to work unchanged.
 */
class TrackListProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    // Note: we intentionally do NOT expose sortRole/sortOrder/filter as
    // Q_PROPERTY here because QSortFilterProxyModel already declares them as
    // int-based properties. QML only calls the Q_INVOKABLE setters and reads
    // count, so property conflicts are avoided.
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit TrackListProxyModel(TrackListModel *sourceModel, QObject *parent = nullptr);

    Q_INVOKABLE void setFilter(const QString &filter);
    Q_INVOKABLE void setSortRole(const QString &role);
    Q_INVOKABLE void setSortOrder(Qt::SortOrder order);
    Q_INVOKABLE void toggleFavorite(int row);
    Q_INVOKABLE void removeTrack(int row);
    Q_INVOKABLE int trackIdAt(int row) const;
    Q_INVOKABLE void reload();

    int count() const;

signals:
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;

private:
    TrackListModel *m_sourceModel = nullptr;
    QString m_filter;
    QString m_filterLower;
    QString m_sortRole;
};

} // namespace Velvox
