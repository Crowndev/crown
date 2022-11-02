#include <qt/sortfilterproxymodel.h>

SortFilterProxyModel::SortFilterProxyModel(QObject* parent) :
    QSortFilterProxyModel(parent)
{
    connect(this, &SortFilterProxyModel::searchStringChanged, this, &SortFilterProxyModel::invalidate); //
}

void SortFilterProxyModel::setSearchString(const QString &searchString)
{
    if (_searchString == searchString)
        return;

    _searchString = searchString;
    Q_EMIT searchStringChanged(searchString); // connected with invalidateFilter, internally invalidateFilter makes call to filterAcceptsRow function
}

void SortFilterProxyModel::setSortOrder(const Qt::SortOrder &sortOrder)
{
    if(_sortOrder == sortOrder)
        return ;
    _sortOrder = sortOrder;
    sort(0, sortOrder);         // responsible call to make sorting, internally it will make a call to lessthan function
    Q_EMIT sortOrderChanged(sortOrder);
}

bool SortFilterProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    QVariant left = sourceModel()->data(source_left);
    QVariant right = sourceModel()->data(source_right);

    if(left.isValid() && right.isValid())
    {
        return left.toString() > right.toString();
    } else
    {
        return false;
    }

        int role = sourceModel()->roleNames().key(roleName.toLocal8Bit(), 0);
        return left.data(role) < right.data(role);
}

bool SortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    QString data = sourceModel()->data(index, Qt::DisplayRole).value<QString>();

    if(_searchString.isEmpty() || _searchString.isNull())
        return true;

    if(data.contains(_searchString, Qt::CaseInsensitive))
        return true;

    return false;
}

void SortFilterProxyModel::setSortRole(QString const& roleName) // Used to select the sort role
{
	this->roleName = roleName; 
}

void SortFilterProxyModel::sort(int /*column*/, Qt::SortOrder order = Qt::AscendingOrder)
{
	QSortFilterProxyModel::sort(0, order); // Always use the first column.
}
