// Copyright (c) 2019 The HEMIS developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/addressfilterproxymodel.h>
#include <amount.h>
#include <logging.h>
#include <key_io.h>

#include <iostream>
#include <QDebug>
bool AddressFilterProxyModel::filterAcceptsRow(int row, const QModelIndex& parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);

    QString label = index.data(AddressTableModel::Label).toString();
    QString type = index.data(AddressTableModel::TypeRole).toString();
    QString address = index.data(AddressTableModel::AddressRole).toString();

    if (!m_types.contains(type))
        return false;

    if (!address.startsWith(m_search_string, Qt::CaseInsensitive))
        return false;

    return true;
}

void AddressFilterProxyModel::setType(const QString& type)
{
    setType(QStringList(type));
}

void AddressFilterProxyModel::setType(const QStringList& types)
{
    this->m_types = types;
    invalidateFilter();
}

int AddressFilterProxyModel::rowCount(const QModelIndex& parent) const
{
    return QSortFilterProxyModel::rowCount(parent);
}

void AddressFilterProxyModel::setRegisteredOnly(bool registered)
{
    this->fRegisteredOnly = registered;
    invalidateFilter();
}

void AddressFilterProxyModel::setOnlySegwit(bool onlyseg, bool regready)
{
    this->fOnlySegwit= onlyseg;
    this->fRegReady = regready;
    m_search_string = onlyseg ? "crw" : "";
    invalidateFilter();
}
