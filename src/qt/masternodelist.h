#ifndef MASTERNODELIST_H
#define MASTERNODELIST_H

#include <masternode/masternode.h>
#include <qt/platformstyle.h>
#include <qt/sendcollateraldialog.h>
#include <sync.h>
#include <util/system.h>

#include <QMenu>
#include <QTimer>
#include <QWidget>

#define MY_MASTERNODELIST_UPDATE_SECONDS                 60
#define MASTERNODELIST_UPDATE_SECONDS                    15
#define MASTERNODELIST_FILTER_COOLDOWN_SECONDS            3

namespace Ui
{
class MasternodeList;
}

class ClientModel;
class WalletModel;
class SendCollateralDialog;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Masternode Manager page widget */
class MasternodeList : public QWidget
{
    Q_OBJECT

public:
    explicit MasternodeList(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~MasternodeList();

    void setClientModel(ClientModel* clientModel);
    void setWalletModel(WalletModel* walletModel);
    void StartAlias(std::string strAlias);
    void StartAll(std::string strCommand = "start-all");
    void VoteMany(std::string strCommand);
    void on_filterLineEdit_textChanged(const QString &strFilterIn);

private:
    QMenu* contextMenu;
    int64_t nTimeFilterUpdated{0};
    bool fFilterUpdated{false};

public Q_SLOTS:
    void updateMyMasternodeInfo(QString strAlias, QString strAddr, CMasternode* pmn);
    void updateMyNodeList(bool fForce = false);
    void updateNodeList();
    void updateVoteList(bool reset = false);
    void updateNextSuperblock();
    SendCollateralDialog* getSendCollateralDialog()
    {
        return sendDialog;
    }

Q_SIGNALS:

private:
    QTimer* timer;
    Ui::MasternodeList* ui;
    ClientModel* clientModel;
    WalletModel* walletModel;
    SendCollateralDialog *sendDialog;
    RecursiveMutex cs_mnlistupdate;
    QString strCurrentFilter;

private Q_SLOTS:
    void notReady();
    void showContextMenu(const QPoint&);
    void on_startButton_clicked();
    void on_startAllButton_clicked();
    void on_startMissingButton_clicked();
    void on_tableWidgetMyMasternodes_itemSelectionChanged();
    void on_UpdateButton_clicked();

    void on_voteManyYesButton_clicked();
    void on_voteManyNoButton_clicked();
    void on_voteManyAbstainButton_clicked();
    void on_tableWidgetVoting_itemSelectionChanged();
    void on_UpdateVotesButton_clicked();
};
#endif // MASTERNODELIST_H
