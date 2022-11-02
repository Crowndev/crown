// Copyright (c) 2011-2020 The Xequium Capital developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_QT_COINCONTROLMODEL_H
#define CROWN_QT_COINCONTROLMODEL_H

#include <QAbstractItemModel>
#include <QStringList>
#include <interfaces/wallet.h>
#include <interfaces/node.h>

class CoinControlPriv;
class WalletModel;
class CoinControlRecord;

#define ASYMP_UTF8 "\xE2\x89\x88"
/** Models table of Global assets.
 */
class CoinControlModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit CoinControlModel(WalletModel *parent = nullptr);
    ~CoinControlModel();

    enum ColumnIndex {
        Label,
        Address,
        Asset,
        Amount,
        Date,
        Confirms,
        TxHash,
        Nindex,
        Locked
    };


    /** Roles to get specific information from a row.
        These are independent of column.
    */
    enum RoleIndex {
        LabelRole = 100,
        AddressRole = 101,
        AssetRole = 102,
        AmountRole = 103,
        DateRole = 104,
        ConfirmsRole = 105,
        TxHashRole = 106,
        NindexRole = 107,
        LockedRole = 108,
    };

    QModelIndex parent(const QModelIndex &child) const override
    {
        return QModelIndex();
    }

    Q_INVOKABLE int rowCount(const QModelIndex &parent=QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Q_INVOKABLE QVariant headerData(int section, Qt::Orientation orientation, int role=Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
	QHash<int, QByteArray> roleNames() const override
	{
		QHash<int, QByteArray> roles;
		roles[LabelRole] = "label";
		roles[AddressRole] = "address";
		roles[AssetRole] = "asset";
		roles[AmountRole] = "amount";
		roles[DateRole] = "date";
		roles[ConfirmsRole] = "confirms";
		roles[TxHashRole] = "txhash";
		roles[NindexRole] = "nindex";
		roles[LockedRole] = "locked";
		return roles;
	}

    void checkBlocksChanged(int count, const QDateTime& blockDate, double nVerificationProgress, bool header, SynchronizationState sync_state);

    Q_INVOKABLE void update();
    
    void buttonToggleLockClicked();

private:
    WalletModel *walletModel;
    QStringList columns;
    CoinControlPriv *priv;

    friend class CoinControlPriv;
};

#endif // CROWN_QT_COINCONTROLMODEL_H
