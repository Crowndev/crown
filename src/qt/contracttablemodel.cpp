// Copyright (c) 2011-2016 The Crown Core developers
// Copyright (c) 2017-2020 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/contracttablemodel.h>
#include <contractdb.h>
#include <interfaces/node.h>
#include <qt/clientmodel.h>

#include <key_io.h>
#include <outputtype.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/walletmodel.h>
#include <validation.h>
#include <QDebug>
#include <QStringList>

class ContractRecord
{
public:

    ContractRecord(): sContractName("")
    {
    }

    ContractRecord(const QString _sContractName): sContractName(_sContractName)
    {
    }

    /** @name Immutable attributes
      @{*/

    QString sContractName;
    QString sContractShortName;
    QString issuer;
    QString contract_url; //online human readable contract
    QString description;
    QString website_url;
    QString scriptcode;
    bool mine;

    /**@}*/
};


class ContractTablePriv {
public:
    ContractTablePriv(WalletModel *_walletModel, ContractTableModel *_parent) :
            parent(_parent),
            walletModel(_walletModel)
    {
    }

    // loads all current balances into cache
    void refreshWallet() {
        cachedContracts.clear();
        //iterate ALL chain Contracts
        //Fill values, where we hold balances
        int minecount =0;

        for(auto const& x : pcontractCache->GetItemsMap()){

            ContractRecord rec(QString::fromStdString(x.first));
            CContract contract = x.second->second.contract;
            rec.sContractName = QString::fromStdString(contract.asset_name);
            rec.sContractShortName = QString::fromStdString(contract.asset_symbol);

            CTxDestination address = DecodeDestination(contract.sIssuingaddress);
            CScript script = GetScriptForDestination(address);
            QString issuer = "";
            if (ExtractDestination(script, address)){
                issuer = QString::fromStdString(EncodeDestination(address));
            }
            else
            {
                issuer = QString::fromStdString(contract.sIssuingaddress);
            }
            rec.issuer = issuer;
            rec.contract_url = QString::fromStdString(contract.contract_url);
            rec.description = QString::fromStdString(contract.description);
            rec.website_url = QString::fromStdString(contract.website_url);
            rec.scriptcode = QString::fromStdString(HexStr(contract.scriptcode));
            bool minew = walletModel->IsMine(address);
            if(minew)
                minecount++;
            rec.mine = minew;

            cachedContracts.append(rec);
        }
        qDebug() << "ContractTablePriv::refreshWallet, cache size " << pcontractCache->Size()  <<  "MINE : "  << minecount;

    }

    int size() {
        return cachedContracts.size();
    }

    ContractRecord *index(int idx) {
        if (idx >= 0 && idx < cachedContracts.size()) {
            return &cachedContracts[idx];
        }
        return 0;
    }

private:

    ContractTableModel *parent;
    WalletModel *walletModel;
    QList<ContractRecord> cachedContracts;

};

ContractTableModel::ContractTableModel(WalletModel *parent) :
        QAbstractTableModel(parent), walletModel(parent), priv(new ContractTablePriv(parent, this))
{
    columns << tr("Name") << tr("Shortname") << tr("Contract URL") << tr("Website URL") << tr("Issuer") << tr("Description") << tr("Script");
    connect(walletModel->getClientModel(), &ClientModel::numBlocksChanged, this, &ContractTableModel::checkBlocksChanged);
    priv->refreshWallet();
};

ContractTableModel::~ContractTableModel()
{
    delete priv;
};

void ContractTableModel::checkBlocksChanged(int count, const QDateTime& blockDate, double nVerificationProgress, bool header, SynchronizationState sync_state) {
    // TODO: optimize by 1) updating cache incrementally; and 2) emitting more specific dataChanged signals
    Q_UNUSED(count);
    Q_UNUSED(blockDate);
    Q_UNUSED(nVerificationProgress);
    Q_UNUSED(header);
    Q_UNUSED(sync_state);
    update();
}

int ContractTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int ContractTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant ContractTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {

        ContractRecord *rec = static_cast<ContractRecord*>(index.internalPointer());

        switch (index.column())
        {
            case NameRole:
                return rec->sContractName;
            case ShortnameRole:
                return rec->sContractShortName;
            case ContractURLRole:
                return rec->contract_url;
            case WebsiteURLRole:
                return rec->website_url;
            case IssuerRole:
                return rec->issuer;
            case DescriptionRole:
                return rec->description;
            case ScriptRole:
                return rec->scriptcode;
            case MineRole:
                return rec->mine;
            default:
                return QVariant();
        }
    }
    return QVariant();

}

QVariant ContractTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QModelIndex ContractTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    ContractRecord *data = priv->index(row);
    if(data)
    {
        QModelIndex idx = createIndex(row, column, priv->index(row));
        return idx;
    }

    return QModelIndex();
}


void ContractTableModel::update()
{
    //Q_EMIT layoutAboutToBeChanged();
    //beginResetModel();
    //endResetModel();
    priv->refreshWallet();
    //Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(priv->size(), columns.length()-1, QModelIndex()));
    //Q_EMIT layoutChanged();
}
