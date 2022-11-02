// Copyright (c) 2017-2019 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <qt/coincontrolfilterproxy.h>
#include <qt/coincontrolmodel.h>

CoinControlFilterProxy::CoinControlFilterProxy(QObject *parent) :
        QSortFilterProxyModel(parent)
{
}

bool CoinControlFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    
    double amount = llabs(index.data(CoinControlModel::AmountRole).toDouble());
    if (sweeping && amount > maxAmount)
        return false;
        
    return true;
}

int CoinControlFilterProxy::rowCount(const QModelIndex& parent) const
{
    return QSortFilterProxyModel::rowCount(parent);
}

void CoinControlFilterProxy::setMaxAmount(const CAmount& maximum, bool sweep)
{
    this->maxAmount = maximum;
    this->sweeping = sweep;
    invalidateFilter();
}

void CoinControlFilterProxy::setSearchString(const QString &search_string)
{
    if (m_search_string == search_string) return;
    m_search_string = search_string;
    invalidateFilter();
}

void CoinControlFilterProxy::setTypeFilter(quint32 modes)
{
    this->typeFilter = modes;
    invalidateFilter();
}

void CoinControlFilterProxy::setLimit(int limit)
{
    this->limitRows = limit;
}

void CoinControlFilterProxy::setReset()
{
    beginResetModel();
    endResetModel();	
}
