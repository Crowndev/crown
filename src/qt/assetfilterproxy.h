// Copyright (c) 2017-2019 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_ASSETFILTERPROXY_H
#define CROWN_ASSETFILTERPROXY_H
#include <amount.h>

#include <QObject>
#include <QSortFilterProxyModel>

class AssetFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit AssetFilterProxy(QObject *parent = 0);

    void setAssetNamePrefix(const QString &sAssetNamePrefix);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    /** Only Mine **/
    void setOnlyMine(bool fOnlyMine);
    /** Only Held **/
    void setOnlyHeld(bool fOnlyHeld);

    void setSearchString(const QString &);
    /**
      @note Type filter takes a bit field created with TYPE() or ALL_TYPES
     */
    void setType(QString modes);

    /** Set maximum number of rows returned, -1 if unlimited. */
    void setLimit(int limit);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;

private:
    QString m_search_string;
    QString typeFilter;
    int limitRows;
    QString sAssetNamePrefix;
    bool fOnlyMine = false;
    bool fOnlyHeld = false;
};
#endif //CROWN_ASSETFILTERPROXY_H
