// Copyright (c) 2017-2019 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <qt/contractfilterproxy.h>
#include <qt/contracttablemodel.h>
#include <QDebug>

ContractFilterProxy::ContractFilterProxy(QObject *parent) :
        QSortFilterProxyModel(parent)
{
}

bool ContractFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    QModelIndex index1 = sourceModel()->index(sourceRow, 7, sourceParent);

    QString sContractName = index.data(ContractTableModel::NameRole).toString();
    bool isMine = sourceModel()->data(index1).toBool();

    if(!sContractName.startsWith(m_search_string, Qt::CaseInsensitive))
        return false;

    if (fOnlyMine && !isMine)
        return false;

    return true;
}

void ContractFilterProxy::setContractNamePrefix(const QString &_sContractNamePrefix)
{
    this->sContractNamePrefix = _sContractNamePrefix;
    invalidateFilter();
}

int ContractFilterProxy::rowCount(const QModelIndex& parent) const
{
    return QSortFilterProxyModel::rowCount(parent);
}

void ContractFilterProxy::setOnlyMine(bool _fOnlyMine){
    fOnlyMine = _fOnlyMine;
    invalidateFilter();
}

void ContractFilterProxy::setSearchString(const QString &search_string)
{
    if (m_search_string == search_string) return;
    m_search_string = search_string;
    invalidateFilter();
}

void ContractFilterProxy::setLimit(int limit)
{
    this->limitRows = limit;
}
