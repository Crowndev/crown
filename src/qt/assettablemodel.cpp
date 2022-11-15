// Copyright (c) 2011-2016 The Crown Core developers
// Copyright (c) 2017-2020 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/assettablemodel.h>
#include <assetdb.h>
#include <interfaces/node.h>
#include <qt/clientmodel.h>

#include <key_io.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/walletmodel.h>
#include <validation.h>
#include <QDebug>
#include <QStringList>
#include <QDateTime>

class AssetRecord
{
public:

    AssetRecord(): sAssetName("")
    {
    }

    AssetRecord(const QString _sAssetName): sAssetName(_sAssetName)
    {
    }

    /** @name Immutable attributes
      @{*/

    QString sAssetName;
    QString sAssetShortName;
    CAmount balance;
    QString issuer; // issuingaddress
    QString issuerAddress; // issuingaddress
    QString type;
    QString txhash;
    QString contract_hash;
    CAmount inputAmount;
    QString issuedAmount;
    QString inputAssetID;
    uint32_t nTime;
    bool fIsAdministrator;
    int32_t nVersion;
    uint32_t nExpiry;
    bool transferable;
    bool convertable;
    bool limited;
    bool restricted;
    bool stakeable;
    bool inflatable;

    /**@}*/
};


class AssetTablePriv {
public:
    AssetTablePriv(WalletModel *_walletModel, ClientModel *_clientModel, AssetTableModel *_parent) :
            parent(_parent),
            walletModel(_walletModel),
            clientModel(_clientModel)
    {
    }

    // loads all current balances into cache
    void refreshWallet() {
        cachedAssets.clear();
        //iterate ALL chain Assets
        //Fill values, where we hold balances
        // Keep up to date with wallet
        interfaces::Wallet& wallet = walletModel->wallet();
        interfaces::WalletBalances balances = wallet.getBalances();

        for(auto const& x : passetsCache->GetItemsMap()){
            AssetRecord rec(QString::fromStdString(x.first));
            bool fIsAdministrator = false;
            CAssetData data = x.second->second;
            CAsset asset = x.second->second.asset;

                rec.sAssetShortName = QString::fromStdString(asset.getShortName());
                rec.inputAmount = data.inputAmount;
                //qDebug() << "ISSUED AMOUNT : " << QString::fromStdString(asset.getShortName()) <<  " "  << clientModel->node().getMoneySupply()[asset] ;
                rec.issuedAmount = QString::number(clientModel->node().getMoneySupply()[asset]/COIN);
                rec.inputAssetID = QString::fromStdString(HexStr(data.inputAssetID));
                rec.nTime = data.nTime;
                rec.txhash = QString::fromStdString(HexStr(data.txhash));
                rec.contract_hash = QString::fromStdString(HexStr(asset.contract_hash));
                rec.nVersion = asset.nVersion;
                rec.nExpiry = asset.nExpiry;
                rec.transferable = asset.isTransferable();
                rec.convertable = asset.isConvertable();
                rec.limited = asset.isLimited();
                rec.restricted = asset.isRestricted();
                rec.stakeable = asset.isStakeable();
                rec.inflatable = asset.isInflatable();
                rec.type = QString::fromStdString(AssetTypeToString(asset.nType));
                rec.balance = 0;

                auto it = balances.balance.find(asset);
                if (it != balances.balance.end()){
                    if(it->second > 0)
                        rec.balance = it->second;
                }
            

            CTxDestination address;
            QString issuer = "";
            if (ExtractDestination(data.issuingAddress, address)){
                if(walletModel->IsMine(address))
                   fIsAdministrator = true;
                issuer = QString::fromStdString(EncodeDestination(address));
            }

                if (data.asset == Params().GetConsensus().subsidy_asset){
                    if(ExtractDestination(Params().GetConsensus().mandatory_coinbase_destination, address))
                        issuer = QString::fromStdString(EncodeDestination(address));
                }
            
            rec.issuerAddress = issuer;
            rec.fIsAdministrator = fIsAdministrator;
            cachedAssets.append(rec);
        }
        //qDebug() << "AssetTablePriv::refreshWallet cache size" << passetsCache->Size();

    }

    int size() {
        return cachedAssets.size();
    }

    AssetRecord *index(int idx) {
        if (idx >= 0 && idx < cachedAssets.size()) {
            return &cachedAssets[idx];
        }
        return 0;
    }

private:

    AssetTableModel *parent;
    WalletModel *walletModel;
    ClientModel *clientModel;
    QList<AssetRecord> cachedAssets;

};

AssetTableModel::AssetTableModel(WalletModel *parent, ClientModel *_clientModel) :
        QAbstractTableModel(parent), walletModel(parent), clientModel(_clientModel), priv(new AssetTablePriv(parent, _clientModel, this))
{
    columns << tr("Name") << tr("Shortname") << tr("Balance") << tr("Issued") << tr("Issuer");
    //connect(walletModel->getClientModel(), &ClientModel::numBlocksChanged, this, &AssetTableModel::checkBlocksChanged);
    priv->refreshWallet();
};

AssetTableModel::~AssetTableModel()
{
    delete priv;
};

void AssetTableModel::checkBlocksChanged(int count, const QDateTime& blockDate, double nVerificationProgress, bool header, SynchronizationState sync_state) {
    // TODO: optimize by 1) updating cache incrementally; and 2) emitting more specific dataChanged signals
    Q_UNUSED(count);
    Q_UNUSED(blockDate);
    Q_UNUSED(nVerificationProgress);
    Q_UNUSED(header);
    Q_UNUSED(sync_state);
    update();
}

int AssetTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int AssetTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant AssetTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    if (role == Qt::DisplayRole) {
        AssetRecord *rec = static_cast<AssetRecord*>(index.internalPointer());

        switch (index.column())
        {
            case NameRole:
                return rec->sAssetName;
            case SymbolRole:
                return rec->sAssetShortName;
            case BalanceRole:
                return (double) rec->balance/COIN;
            case IssuedRole:
                return rec->issuedAmount;
            case IssuerRole:
                return rec->issuerAddress;
        case TypeRole:
            return rec->type;
        case OwnedRole:
            return rec->fIsAdministrator;
            default:
                return QVariant();
        }
    }
    return QVariant();
}

QVariant AssetTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            if (section < columns.size())
                return columns.at(section);
        } else {
            return section;
        }
    } else if (role == Qt::SizeHintRole) {
        if (orientation == Qt::Vertical)
            return QSize(30, 50);
    } else if (role == Qt::TextAlignmentRole) {
        if (orientation == Qt::Vertical)
            return Qt::AlignLeft + Qt::AlignVCenter;

        return Qt::AlignHCenter + Qt::AlignVCenter;
    }

    return QVariant();
}

QModelIndex AssetTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    AssetRecord *data = priv->index(row);
    if(data)
    {
        QModelIndex idx = createIndex(row, column, priv->index(row));
        return idx;
    }

    return QModelIndex();
}

void AssetTableModel::update()
{
    Q_EMIT layoutAboutToBeChanged();
    beginResetModel();
    endResetModel();
    priv->refreshWallet();
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(priv->size(), columns.length()-1, QModelIndex()));
    Q_EMIT layoutChanged();
}
