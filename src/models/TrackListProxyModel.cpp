#include "models/TrackListProxyModel.h"

#include <QDateTime>

namespace Velvox {

TrackListProxyModel::TrackListProxyModel(TrackListModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_sourceModel(sourceModel)
    , m_sortRole(QStringLiteral("date_added"))
{
    setSourceModel(sourceModel);
    setDynamicSortFilter(true);

    // Mirror the original TrackListModel defaults so the QML views start sorted.
    sort(0, Qt::DescendingOrder);

    connect(this, &QAbstractItemModel::rowsInserted,
            this, &TrackListProxyModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved,
            this, &TrackListProxyModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset,
            this, &TrackListProxyModel::countChanged);
}

void TrackListProxyModel::setFilter(const QString &filter)
{
    if (m_filter == filter)
        return;

    m_filter = filter;
    m_filterLower = filter.toLower();
    invalidateFilter();
    // countChanged is emitted automatically via rowsInserted/rowsRemoved connections
    // when the filter actually changes the visible row count.
}

void TrackListProxyModel::setSortRole(const QString &role)
{
    if (m_sortRole == role)
        return;

    m_sortRole = role;
    // Re-sort using the current sort order. This emits layoutChanged, not a
    // full model reset, so the ListView keeps its delegate pool.
    sort(0, sortOrder());
}

void TrackListProxyModel::setSortOrder(Qt::SortOrder order)
{
    if (sortOrder() == order)
        return;

    sort(0, order);
}

void TrackListProxyModel::toggleFavorite(int row)
{
    if (row < 0 || row >= count())
        return;

    const QModelIndex sourceIndex = mapToSource(index(row, 0));
    if (sourceIndex.isValid())
        m_sourceModel->toggleFavorite(sourceIndex.row());
}

void TrackListProxyModel::removeTrack(int row)
{
    if (row < 0 || row >= count())
        return;

    const QModelIndex sourceIndex = mapToSource(index(row, 0));
    if (sourceIndex.isValid())
        m_sourceModel->removeTrack(sourceIndex.row());
}

int TrackListProxyModel::trackIdAt(int row) const
{
    if (row < 0 || row >= count())
        return -1;

    const QModelIndex sourceIndex = mapToSource(index(row, 0));
    if (!sourceIndex.isValid())
        return -1;
    return m_sourceModel->trackIdAt(sourceIndex.row());
}

void TrackListProxyModel::reload()
{
    m_sourceModel->reload();
}

int TrackListProxyModel::count() const
{
    return rowCount();
}

bool TrackListProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_filterLower.isEmpty())
        return true;

    const QModelIndex idx = m_sourceModel->index(sourceRow, 0, sourceParent);
    const QString title = m_sourceModel->data(idx, TrackListModel::TitleRole).toString().toLower();
    const QString artist = m_sourceModel->data(idx, TrackListModel::ArtistRole).toString().toLower();
    const QString album = m_sourceModel->data(idx, TrackListModel::AlbumRole).toString().toLower();

    return title.contains(m_filterLower)
           || artist.contains(m_filterLower)
           || album.contains(m_filterLower);
}

bool TrackListProxyModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
    bool less = false;

    if (m_sortRole == QStringLiteral("title")) {
        const QString a = m_sourceModel->data(sourceLeft, TrackListModel::TitleRole).toString();
        const QString b = m_sourceModel->data(sourceRight, TrackListModel::TitleRole).toString();
        less = a.compare(b, Qt::CaseInsensitive) < 0;
    } else if (m_sortRole == QStringLiteral("artist")) {
        const QString a = m_sourceModel->data(sourceLeft, TrackListModel::ArtistRole).toString();
        const QString b = m_sourceModel->data(sourceRight, TrackListModel::ArtistRole).toString();
        less = a.compare(b, Qt::CaseInsensitive) < 0;
    } else if (m_sortRole == QStringLiteral("album")) {
        const QString a = m_sourceModel->data(sourceLeft, TrackListModel::AlbumRole).toString();
        const QString b = m_sourceModel->data(sourceRight, TrackListModel::AlbumRole).toString();
        less = a.compare(b, Qt::CaseInsensitive) < 0;
    } else if (m_sortRole == QStringLiteral("date_added")) {
        const QDateTime a = m_sourceModel->data(sourceLeft, TrackListModel::DateAddedRole).toDateTime();
        const QDateTime b = m_sourceModel->data(sourceRight, TrackListModel::DateAddedRole).toDateTime();
        less = a < b;
    } else if (m_sortRole == QStringLiteral("duration")) {
        const qint64 a = m_sourceModel->data(sourceLeft, TrackListModel::DurationMsRole).toLongLong();
        const qint64 b = m_sourceModel->data(sourceRight, TrackListModel::DurationMsRole).toLongLong();
        less = a < b;
    } else {
        return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
    }

    return sortOrder() == Qt::AscendingOrder ? less : !less;
}

} // namespace Velvox
