#ifndef SORTPROXYMODEL_H
#define SORTPROXYMODEL_H

#include <QSortFilterProxyModel>

class SortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString searchString READ searchString WRITE setSearchString NOTIFY searchStringChanged)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged)
public:
    explicit SortFilterProxyModel(QObject* parent = nullptr);

    QString searchString() const
    {
        return _searchString;
    }
    Qt::SortOrder sortOrder() const
    {
        return _sortOrder;
    }
    void setSearchString(const QString &searchString);
    void setSortOrder(const Qt::SortOrder &sortOrder);
    Q_INVOKABLE void setSortRole(QString const& roleName); // Used to select the sort role
    Q_INVOKABLE virtual void sort(int /*column*/, Qt::SortOrder order = Qt::AscendingOrder);
    
Q_SIGNALS:
    void searchStringChanged(QString searchString);
    void sortOrderChanged(Qt::SortOrder sortOrder);

protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;   // custom sort logic
    bool filterAcceptsRow(int sourceRow,const QModelIndex &sourceParent) const;             // custom filter logic

private:
    QString _searchString;
    Qt::SortOrder _sortOrder;
    QString roleName; // Role used to sort the model
};

#endif // SORTPROXYMODEL_H
