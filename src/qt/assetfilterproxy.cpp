// Copyright (c) 2017-2019 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <qt/assetfilterproxy.h>
#include <qt/assettablemodel.h>

AssetFilterProxy::AssetFilterProxy(QObject *parent) :
        QSortFilterProxyModel(parent),
        sAssetNamePrefix(),
        typeFilter("ALL")
{
}

bool AssetFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{

    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    QString sAssetName = index.data(AssetTableModel::NameRole).toString();

    if(!sAssetName.startsWith(m_search_string, Qt::CaseInsensitive))
        return false;

    QString sType = index.data(AssetTableModel::TypeRole).toString();    

    if (typeFilter != "ALL" && sType != typeFilter)
        return false;

    qint64 balance = llabs(index.data(AssetTableModel::BalanceRole).toLongLong());

    if (fOnlyHeld && balance <= 0)
        return false;

    bool isMine = index.data(AssetTableModel::OwnedRole).toBool();

    if (fOnlyMine && !isMine)
        return false;

    return true;
}

void AssetFilterProxy::setAssetNamePrefix(const QString &_sAssetNamePrefix)
{
    this->sAssetNamePrefix = _sAssetNamePrefix;
    invalidateFilter();
}

int AssetFilterProxy::rowCount(const QModelIndex& parent) const
{
    return QSortFilterProxyModel::rowCount(parent);
}

void AssetFilterProxy::setOnlyMine(bool fOnlyMine){
    this->fOnlyMine = fOnlyMine;
    invalidateFilter();
}

void AssetFilterProxy::setOnlyHeld(bool fOnlyHeld){
    this->fOnlyHeld = fOnlyHeld;
    invalidateFilter();
}

void AssetFilterProxy::setSearchString(const QString &search_string)
{
    if (m_search_string == search_string) return;
    m_search_string = search_string;
    invalidateFilter();
}

void AssetFilterProxy::setType(QString modes)
{
    if (typeFilter == modes) return;
    typeFilter = modes;
    invalidateFilter();
}

void AssetFilterProxy::setLimit(int limit)
{
    this->limitRows = limit;
}
