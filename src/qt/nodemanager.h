#ifndef NODEMANAGER_H
#define NODEMANAGER_H

#include <QWidget>
#include <masternode/masternode.h>
#include <qt/platformstyle.h>
#include <qt/sendcollateraldialog.h>
#include <sync.h>
#include <util/system.h>

#include <QMenu>
#include <QTimer>
#include <QWidget>

#define MY_NODELIST_UPDATE_SECONDS                 60
#define NODELIST_UPDATE_SECONDS                    15
#define NODELIST_FILTER_COOLDOWN_SECONDS            3

class ClientModel;
class WalletModel;
class CSystemnode;
class CMasternode;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

//class SendCollateralDialog;

namespace Ui {
class NodeManager;
}

class NodeManager : public QWidget
{
    Q_OBJECT

public:
    explicit NodeManager(const PlatformStyle *_platformStyle, QWidget *parent = nullptr);
    ~NodeManager();
    void setClientModel(ClientModel* clientModel);
    void setWalletModel(WalletModel* walletModel);
    void StartAlias(std::string strAlias);
    void StartAll(std::string strCommand = "start-all");
    void selectAliasRow(const QString& alias);
    void VoteMany(std::string strCommand);


public Q_SLOTS:
    void updateMySystemnodeInfo(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, CSystemnode *pmn);
    void updateMyMasternodeInfo(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, CMasternode *pmn);
    void updateMyNodeList(bool fForce = false);
    void updateNodeList();
    void updateVoteList(bool reset = false);
    void updateNextSuperblock();
    
    SendCollateralDialog* getSendCollateralDialog()
    {
        return sendDialog;
    }

Q_SIGNALS:



private Q_SLOTS:
    void notReady();
    void showContextMenu(const QPoint&);
    void on_filterLineEdit_textChanged(const QString &filterString);
    void on_startButton_clicked();
    void on_editButton_clicked();
    void on_startAllButton_clicked();
    void on_startMissingButton_clicked();
    void on_tableWidgetMySystemnodes_itemSelectionChanged();
    void on_UpdateButton_clicked();
    void on_CreateNewSystemnode_clicked();

    void on_tableWidgetMyMasternodes_itemSelectionChanged();

    void on_voteManyYesButton_clicked();
    void on_voteManyNoButton_clicked();
    void on_voteManyAbstainButton_clicked();
    void on_tableWidgetVoting_itemSelectionChanged();
    void on_UpdateVotesButton_clicked();
    void on_CreateNewMasternode_clicked();
     
private:
    QTimer* timer;
    Ui::NodeManager *ui;
    ClientModel* clientModel;
    WalletModel* walletModel;
    SendCollateralDialog *sendDialog;
    RecursiveMutex cs_mnlistupdate;
    const PlatformStyle *platformStyle; 

    QString strCurrentFilter;
    QMenu* contextMenu;
    int64_t nTimeFilterUpdated{0};
    bool fFilterUpdated{false};
};

#endif // NODEMANAGER_H
