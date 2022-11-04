// Copyright (c) 2011-2016 The Crown Core developers
// Copyright (c) 2017-2019 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_QT_ASSETTABLEMODEL_H
#define CROWN_QT_ASSETTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <interfaces/wallet.h>

class AssetTablePriv;
class WalletModel;
class ClientModel;
class AssetRecord;

struct CAsset;
enum class SynchronizationState;

/** Models table of Global assets.
 */
class AssetTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit AssetTableModel(WalletModel *parent = nullptr, ClientModel *_clientModel = nullptr);
    ~AssetTableModel();

    enum ColumnIndex {
        Name,
        Shortname,
        Balance,
        Issued,
        Issuer,       
        Type,
        Owned
   };

    /** Roles to get specific information from a row.
        These are independent of column.
    */
    enum RoleIndex {
        NameRole = 0,
        SymbolRole = 1,
        BalanceRole = 2,
        IssuedRole = 3,
        IssuerRole = 4,
        TypeRole = 5,
        OwnedRole = 6,
    };

    Q_INVOKABLE int rowCount(const QModelIndex &parent=QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Q_INVOKABLE QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
 /*
	QHash<int, QByteArray> roleNames() const override
	{
		QHash<int, QByteArray> roles;
		roles[NameRole] = "name";
		roles[SymbolRole] = "symbol";
		roles[BalanceRole] = "balance";
		roles[InputRole] = "input";
		roles[InputAssetRole] = "inputasset";
		roles[IssuedRole] = "issued";
		roles[IssuerRole] = "issuer";
		roles[IssuerAddressRole] = "issueraddress";
		roles[DateRole] = "date";
		roles[TypeRole] = "type";
		roles[TxHashRole] = "txhash";
		roles[ContractHashRole] = "contracthash";
		roles[OwnedRole] = "owned";
		roles[VersionRole] = "version";
		roles[ExpiryRole] = "expiry";
		roles[TransferableRole] = "transferable";
		roles[ConvertableRole] = "convertable";
		roles[LimitedRole] = "limited";
		roles[RestrictedRole] = "restricted";
		roles[StakeableRole] = "stakeable";
		roles[InflatableRole] = "inflatable";
		return roles;
	}
	*/
    void checkBlocksChanged(int count, const QDateTime& blockDate, double nVerificationProgress, bool header, SynchronizationState sync_state);

    Q_INVOKABLE void update();

private:
    WalletModel *walletModel;
    ClientModel *clientModel;

    QStringList columns;
    AssetTablePriv *priv;

    friend class AssetTablePriv;
};
/*
class AssetTableModelWrapper: public QObject
{
public:
	explicit AssetTableModelWrapper(QObject* parent=nullptr);

    ~AssetTableModelWrapper() {}

	void initialize(interfaces::Node& node, WalletModel *parent = 0)
	{
	    m_obj = new AssetTableModel(node,parent);
	}
    //... + additional member functions you need.
protected:
    AssetTableModel * m_obj =nullptr;
};*/
#endif // CROWN_QT_ASSETTABLEMODEL_H
