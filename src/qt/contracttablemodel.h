// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_QT_CONTRACTTABLEMODEL_H
#define CROWN_QT_CONTRACTTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <interfaces/wallet.h>

class ContractTablePriv;
class WalletModel;
class ContractRecord;
class ClientModel;
class CContract;
enum class SynchronizationState;

/** Models table of Global assets.
 */
class ContractTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ContractTableModel(WalletModel *parent = nullptr);
    ~ContractTableModel();

    enum ColumnIndex {
        Name,
        Shortname,
        ContractURL,
        WebsiteURL,
        Issuer,
        Description,
        Script,
        Mine
    };

    /** Roles to get specific information from a row.
        These are independent of column.
    */
    enum RoleIndex {
        NameRole = 0,
        ShortnameRole = 1,
        ContractURLRole = 2,
        WebsiteURLRole = 3,
        IssuerRole = 4,
        DescriptionRole = 5,
        ScriptRole = 6,
        MineRole
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
		roles[ShortnameRole] = "shortname";
		roles[ContractURLRole] = "contract_url";
		roles[WebsiteURLRole] = "website_url";
		roles[DescriptionRole] = "description";
		roles[IssuerRole] = "issuer";
		roles[ScriptRole] = "script";
		roles[MineRole] = "mine";
		return roles;
	}
*/
    void checkBlocksChanged(int count, const QDateTime& blockDate, double nVerificationProgress, bool header, SynchronizationState sync_state);

    Q_INVOKABLE void update();

private:
    WalletModel *walletModel;
    ClientModel* m_client_model;
    QStringList columns;
    ContractTablePriv *priv;
    friend class ContractTablePriv;
};

#endif // CROWN_QT_CONTRACTTABLEMODEL_H
