// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <node/ui_interface.h>
#include <qt/sendcollateraldialog.h>
#include <qt/sendcoinsdialog.h>
#include <qt/crownunits.h>
#include <qt/optionsmodel.h>
#include <QDebug>

SendCollateralDialog::SendCollateralDialog(const PlatformStyle *platformStyle, Node node, QWidget *parent) :
    SendCoinsDialog(platformStyle, nullptr),
    node(node)
{
}

void SendCollateralDialog::vsend(QList<SendAssetsRecipient> &recipients)
{
	qDebug() << __func__  <<  "1 : ";
    QStringList formatted = constructConfirmationMessage(recipients);
    qDebug() << __func__  <<  "2 : ";
    checkAndSend(recipients, formatted);
    qDebug() << __func__  <<  "3 : ";
}

bool SendCollateralDialog::instantXChecked()
{
    return false;
}

void SendCollateralDialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    msgParams.second = CClientUIInterface::MSG_WARNING;
    int collateral = Params().SystemnodeCollateral() / COIN;
    QString nodeType = "Systemnode";
    if (node == MASTERNODE) {
        collateral = Params().MasternodeCollateral() / COIN;
        nodeType = "Masternode";
    }
    QString formattedAmount = CrownUnits::formatWithUnit(getModel()->getOptionsModel()->getDisplayUnit(), collateral * COIN);

    switch(sendCoinsReturn.status)
    {
        case WalletModel::AmountExceedsBalance:
            msgParams.first = tr("Your %1 has not been generated at this time due to not having enough Crown coins.").arg(nodeType);
            msgParams.first += QString("<br>") + tr("For a %1 you will require %2").arg(nodeType).arg(formattedAmount);
            break;
        case WalletModel::AmountWithFeeExceedsBalance:
            msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
            msgParams.first += QString("<br>") + tr("Your %1 has not been generated at this time due to not having enough Crown coins.").arg(nodeType);
            msgParams.first += QString("<br>") + tr("For a %1 you will require %2 + fee").arg(nodeType).arg(formattedAmount);
            break;
        default:
            // Here handled only the case when amount exceeds balance. Other errors handled in base.
            SendCoinsDialog::processSendCoinsReturn(sendCoinsReturn, msgArg);
            return;
    }

    Q_EMIT message(tr("Send Coins"), msgParams.first, msgParams.second);
}
