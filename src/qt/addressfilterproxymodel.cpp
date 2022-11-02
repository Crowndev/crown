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
    bool registered = index.data(AddressTableModel::RegisteredRole).toBool();
    bool mine = index.data(AddressTableModel::MineRole).toBool();
    //quint64 amount = llabs(index.data(AddressTableModel::AmountRole).toLongLong());
    //int64_t balance = static_cast<int64_t>(amount);
    
    //qDebug() << "ADDRESS BALANCE = " << amount;

    //LogPrintf("ADDRESS BALANCE %d\n", balance);

    if (!m_types.contains(type))
        return false;

    if(fOnlySegwit && fRegReady && type == "R"){

        if (!address.startsWith(m_search_string, Qt::CaseInsensitive))
            return false;
        
        //if(balance < 10.0)
          //  return false;
    }

    if(fRegisteredOnly && !registered)
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
    m_search_string = onlyseg ? "hms1" : "";
    invalidateFilter();
}
