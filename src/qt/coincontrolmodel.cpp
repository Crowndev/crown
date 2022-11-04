// Copyright (c) 2011-2016 The Crown Core developers
// Copyright (c) 2017-2020 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/coincontrolmodel.h>
#include <qt/addresstablemodel.h>
#include <qt/optionsmodel.h>
#include <qt/crownunits.h>
#include <qt/clientmodel.h>

#include <key_io.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/walletmodel.h>
#include <validation.h>
#include <QDebug>
#include <QStringList>
#include <QDateTime>

class CoinControlRecord
{
public:

    CoinControlRecord(): address("")
    {
    }

    CoinControlRecord(const QString _address): address(_address)
    {
    }

    /** @name Immutable attributes
      @{*/

    QString address;
    QString asset;
    QString label;
    QString txhash;
    int index;
    QString amount;
    qint64 time;   
    int confirmations;
    bool locked;

    /**@}*/
};


class CoinControlPriv {
public:
    CoinControlPriv(WalletModel *_walletModel, CoinControlModel *_parent) :
            parent(_parent),
            walletModel(_walletModel)
    {
    }

    // loads all current balances into cache
    void refreshWallet() {
        cachedRecords.clear();

        // Keep up to date with wallet
        interfaces::Wallet& wallet = walletModel->wallet();
        int unit = walletModel->getOptionsModel()->getDisplayUnit();

        for (const auto& coins : wallet.listCoins()) {
            QString sWalletAddress = QString::fromStdString(EncodeDestination(coins.first));
            QString sWalletLabel = walletModel->getAddressTableModel()->labelForAddress(sWalletAddress);
            if (sWalletLabel.isEmpty())
                sWalletLabel = "(no label)";

            for (const auto& outpair : coins.second) {
                CoinControlRecord rec;

                const COutPoint& output = std::get<0>(outpair);
                const interfaces::WalletTxOut& out = std::get<1>(outpair);

                // address
                CTxDestination outputAddress;
                QString sAddress = "";
                if(ExtractDestination(out.txout.scriptPubKey, outputAddress))
                    sAddress = QString::fromStdString(EncodeDestination(outputAddress));

                // label
                QString sLabel = walletModel->getAddressTableModel()->labelForAddress(sAddress);
                if (sLabel.isEmpty())
                    sLabel = "(no label)";
                
                rec.address = sAddress;
                rec.label = sLabel;
                std::string assetname =out.txout.nAsset.getShortName();
                CAmount assetamount = out.txout.nValue;
                                                   
                rec.asset = QString::fromStdString(assetname);
                rec.amount = CrownUnits::simplestFormat(unit, assetamount, 4, false, CrownUnits::SeparatorStyle::ALWAYS);
                rec.time = out.time;
                rec.confirmations = out.depth_in_main_chain;
                rec.txhash = QString::fromStdString(output.hash.GetHex());
                rec.index = output.n;
                rec.locked = wallet.isLockedCoin(output);
                cachedRecords.append(rec);
            }
        }

        QString msg = QString("CoinControlPriv size: %1\n").arg(cachedRecords.size());
        qDebug() << msg;
    }

    int size() {
        return cachedRecords.size();
    }

    CoinControlRecord *index(int idx) {
        if (idx >= 0 && idx < cachedRecords.size()) {
            return &cachedRecords[idx];
        }
        return 0;
    }

private:

    CoinControlModel *parent;
    WalletModel *walletModel;
    QList<CoinControlRecord> cachedRecords;

};

CoinControlModel::CoinControlModel(WalletModel *parent) :
        QAbstractItemModel(parent), walletModel(parent), priv(new CoinControlPriv(parent, this))
{
    columns << tr("Label") << tr("Address") << tr("Asset") << tr("Amount") << tr("Date") << tr("Confirms");
    connect(walletModel->getClientModel(), &ClientModel::numBlocksChanged, this, &CoinControlModel::checkBlocksChanged);
    priv->refreshWallet();
};

CoinControlModel::~CoinControlModel()
{
    delete priv;
};

void CoinControlModel::checkBlocksChanged(int count, const QDateTime& blockDate, double nVerificationProgress, bool header, SynchronizationState sync_state) {
    // TODO: optimize by 1) updating cache incrementally; and 2) emitting more specific dataChanged signals
    Q_UNUSED(count);
    Q_UNUSED(blockDate);
    Q_UNUSED(nVerificationProgress);
    Q_UNUSED(header);
    Q_UNUSED(sync_state);
    update();
}

int CoinControlModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int CoinControlModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant CoinControlModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    CoinControlRecord *rec = static_cast<CoinControlRecord*>(index.internalPointer());

    switch (role)
    {
        case LabelRole:{
            return rec->label;
            break;
		}
        case AddressRole:{
            return rec->address;
            break;
		}
        case AssetRole:{
            return rec->asset;
            break;
		}
        case AmountRole:{
            return rec->amount;
            break;
		}
        case DateRole:{
            return QDateTime::fromTime_t(static_cast<uint>(rec->time));
            break;
		}
        case ConfirmsRole:{
            return rec->confirmations;
            break;
		}
        case NindexRole:{
            return rec->index;
            break;
		}
        case TxHashRole:{
            return rec->txhash;
            break;
		}
        case LockedRole:{
            return rec->locked;
            break;
		}
        default:
            return QVariant();
    }
}

QVariant CoinControlModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QModelIndex CoinControlModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    CoinControlRecord *data = priv->index(row);
    if(data)
    {
        QModelIndex idx = createIndex(row, column, priv->index(row));
        return idx;
    }

    return QModelIndex();
}

void CoinControlModel::update()
{
    Q_EMIT layoutAboutToBeChanged();
    beginResetModel();
    endResetModel();
    priv->refreshWallet();
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(priv->size(), columns.length()-1, QModelIndex()));
    Q_EMIT layoutChanged();

}
