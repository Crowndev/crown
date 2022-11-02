// Copyright (c) 2017-2019 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_COINCONTROLFILTERPROXY_H
#define CROWN_COINCONTROLFILTERPROXY_H
#include <amount.h>
#include <QObject>
#include <QSortFilterProxyModel>

class CoinControlFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit CoinControlFilterProxy(QObject *parent = 0);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;   
    void setSearchString(const QString &);
    void setTypeFilter(quint32 modes);
    void setLimit(int limit);
    void setMaxAmount(const CAmount& maximum, bool sweep);
    void setReset();


protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;

private:
    QString m_search_string;
    quint32 typeFilter;
    CAmount maxAmount;
    int limitRows;
    bool sweeping;
};
#endif //CROWN_COINCONTROLFILTERPROXY_H
