// Copyright (c) 2017-2019 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_CONTRACTFILTERPROXY_H
#define CROWN_CONTRACTFILTERPROXY_H
#include <amount.h>
#include <QObject>
#include <QSortFilterProxyModel>

class ContractFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ContractFilterProxy(QObject *parent = 0);

    void setContractNamePrefix(const QString &sContractNamePrefix);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    /** Only Mine **/
    void setOnlyMine(int fOnlyMine);
    /** Only Held **/
    void setSearchString(const QString &);
    void setLimit(int limit);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;

private:
    QString m_search_string;
    quint32 typeFilter;
    CAmount minAmount;
    int limitRows;
    QString sContractNamePrefix;
    bool fOnlyMine = false;
    bool fOnlyHeld = false;
};
#endif //CROWN_CONTRACTFILTERPROXY_H
