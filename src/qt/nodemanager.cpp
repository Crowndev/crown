#include <qt/nodemanager.h>
#include <qt/forms/ui_nodemanager.h>

#include <init.h>
#include <key_io.h>
#include <masternode/activemasternode.h>
#include <masternode/masternode-budget.h>
#include <masternode/masternode-sync.h>
#include <masternode/masternodeconfig.h>
#include <masternode/masternodeman.h>
#include <systemnode/activesystemnode.h>
#include <systemnode/systemnode-sync.h>
#include <systemnode/systemnodeconfig.h>
#include <systemnode/systemnodeman.h>
#include <node/context.h>
#include <rpc/blockchain.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/walletmodel.h>
#include <sync.h>
#include <wallet/wallet.h>
#include <qt/privatekeywidget.h>
#include <qt/crownunits.h>
#include <qt/addresstablemodel.h>
#include <qt/optionsmodel.h>
#include <qt/datetablewidgetitem.h>
#include <qt/createmasternodedialog.h>
#include <qt/createsystemnodedialog.h>
#include <QMessageBox>
#include <QTimer>

#include <crown/legacysigner.h>


#include <nodeconfig.h>
#include <qt/startmissingdialog.h>
#include <util/time.h>



NodeManager::NodeManager(const PlatformStyle *_platformStyle, QWidget *parent) :
    QWidget(parent),
    clientModel(0),
    platformStyle(_platformStyle),
    walletModel(0),
    ui(new Ui::NodeManager)
{
    ui->setupUi(this);
    ui->startButton->setEnabled(false);
    ui->voteManyYesButton->setEnabled(false);
    ui->voteManyNoButton->setEnabled(false);

    int columnAliasWidth = 60;
    int columnAddressWidth = 160;
    int columnProtocolWidth = 50;
    int columnStatusWidth = 60;
    int columnActiveWidth = 100;
    int columnLastSeenWidth = 100;

    ui->tableWidgetMyMasternodes->setColumnWidth(0, columnAliasWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(1, columnAddressWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(2, columnProtocolWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(3, columnStatusWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(4, columnActiveWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(5, columnLastSeenWidth);

    ui->tableWidgetMasternodes->setColumnWidth(0, columnAddressWidth);
    ui->tableWidgetMasternodes->setColumnWidth(1, columnProtocolWidth);
    ui->tableWidgetMasternodes->setColumnWidth(2, columnStatusWidth);
    ui->tableWidgetMasternodes->setColumnWidth(3, columnActiveWidth);
    ui->tableWidgetMasternodes->setColumnWidth(4, columnLastSeenWidth);

    ui->tableWidgetMySystemnodes->setColumnWidth(0, columnAliasWidth);
    ui->tableWidgetMySystemnodes->setColumnWidth(1, columnAddressWidth);
    ui->tableWidgetMySystemnodes->setColumnWidth(2, columnProtocolWidth);
    ui->tableWidgetMySystemnodes->setColumnWidth(3, columnStatusWidth);
    ui->tableWidgetMySystemnodes->setColumnWidth(4, columnActiveWidth);
    ui->tableWidgetMySystemnodes->setColumnWidth(5, columnLastSeenWidth);

    ui->tableWidgetSystemnodes->setColumnWidth(0, columnAddressWidth);
    ui->tableWidgetSystemnodes->setColumnWidth(1, columnProtocolWidth);
    ui->tableWidgetSystemnodes->setColumnWidth(2, columnStatusWidth);
    ui->tableWidgetSystemnodes->setColumnWidth(3, columnActiveWidth);
    ui->tableWidgetSystemnodes->setColumnWidth(4, columnLastSeenWidth);

    ui->tableWidgetMySystemnodes->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->tableWidgetMyMasternodes->setContextMenuPolicy(Qt::CustomContextMenu);

    QAction* startAliasAction = new QAction(tr("Start alias"), this);
    QAction *editAction = new QAction(tr("Edit"), this);
    contextMenu = new QMenu();
    contextMenu->addAction(startAliasAction);
    contextMenu->addAction(editAction);

    connect(ui->tableWidgetMyMasternodes, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
    connect(ui->tableWidgetMySystemnodes, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));

    connect(startAliasAction, SIGNAL(triggered()), this, SLOT(on_startButton_clicked()));
    connect(editAction, SIGNAL(triggered()), this, SLOT(on_editButton_clicked()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNodeList()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateVoteList()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateMyNodeList()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNextSuperblock()));
    timer->start(1000);

    updateNodeList();
    updateVoteList();
    updateNextSuperblock();

    // Fill MN list
    fFilterUpdated = false;
    nTimeFilterUpdated = GetTime();
    updateNodeList();

    sendDialog = new SendCollateralDialog(platformStyle, SendCollateralDialog::MASTERNODE, parent);
    sendDialog = new SendCollateralDialog(platformStyle, SendCollateralDialog::SYSTEMNODE, parent);
}

NodeManager::~NodeManager()
{
    delete ui;
}


void NodeManager::setClientModel(ClientModel* model)
{
    if(!model)
        return;
    this->clientModel = model;
    // try to update list when masternode count changes
    connect(clientModel, SIGNAL(strMasternodesChanged(QString)), this, SLOT(updateNodeList()));
    connect(clientModel, SIGNAL(strSystemnodesChanged(QString)), this, SLOT(updateNodeList()));

}

void NodeManager::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
}

void NodeManager::showContextMenu(const QPoint& point)
{
    QTableWidgetItem* item = ui->tableWidgetMyMasternodes->itemAt(point);
    QTableWidgetItem* item2 = ui->tableWidgetMySystemnodes->itemAt(point);

    if (item || item2) contextMenu->exec(QCursor::pos());
}

void NodeManager::StartAlias(std::string strAlias)
{
    std::string strStatusHtml;
    strStatusHtml += "<center>Alias: " + strAlias;

    for (CNodeEntry mne : masternodeConfig.getEntries()) {
        if (mne.getAlias() == strAlias) {
            std::string strError;
            CMasternodeBroadcast mnb;

            bool fSuccess = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

            if (fSuccess) {
                strStatusHtml += "<br>Successfully started masternode.";
                mnodeman.UpdateMasternodeList(mnb, *g_rpc_node->connman);
                mnb.Relay(*g_rpc_node->connman);
            } else {
                strStatusHtml += "<br>Failed to start masternode.<br>Error: " + strError;
            }
            break;
        }
    }

    for (CNodeEntry mne : systemnodeConfig.getEntries()) {
        if (mne.getAlias() == strAlias) {
            std::string strError;
            CSystemnodeBroadcast mnb;

            bool fSuccess = CSystemnodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

            if (fSuccess) {
                strStatusHtml += "<br>Successfully started systemnode.";
                snodeman.UpdateSystemnodeList(mnb, *g_rpc_node->connman);
                mnb.Relay(*g_rpc_node->connman);
            } else {
                strStatusHtml += "<br>Failed to start systemnode.<br>Error: " + strError;
            }
            break;
        }
    }
    strStatusHtml += "</center>";

    QMessageBox msg;
    msg.setText(QString::fromStdString(strStatusHtml));
    msg.exec();

    updateMyNodeList(true);
}

void NodeManager::StartAll(std::string strCommand)
{
    int nCountSuccessful = 0;
    int nCountFailed = 0;
    std::string strFailedHtml;

    for (CNodeEntry mne : masternodeConfig.getEntries()) {
        std::string strError;
        CMasternodeBroadcast mnb;

        CTxIn txin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
        CMasternode* pmn = mnodeman.Find(txin);

        if (strCommand == "start-missing" && pmn) continue;

        bool fSuccess = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

        if (fSuccess) {
            nCountSuccessful++;
            mnodeman.UpdateMasternodeList(mnb, *g_rpc_node->connman);
            mnb.Relay(*g_rpc_node->connman);
        } else {
            nCountFailed++;
            strFailedHtml += "\nFailed to start " + mne.getAlias() + ". Error: " + strError;
        }
    }

    for (CNodeEntry mne : systemnodeConfig.getEntries()) {
        std::string strError;
        CSystemnodeBroadcast mnb;

        CTxIn txin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
        CSystemnode* pmn = snodeman.Find(txin);

        if (strCommand == "start-missing" && pmn) continue;

        bool fSuccess = CSystemnodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

        if (fSuccess) {
            nCountSuccessful++;
            snodeman.UpdateSystemnodeList(mnb, *g_rpc_node->connman);
            mnb.Relay(*g_rpc_node->connman);
        } else {
            nCountFailed++;
            strFailedHtml += "\nFailed to start " + mne.getAlias() + ". Error: " + strError;
        }
    }
    GetMainWallet()->Lock();

    std::string returnObj;
    returnObj = strprintf("Successfully started %d nodes, failed to start %d, total %d", nCountSuccessful, nCountFailed, nCountFailed + nCountSuccessful);
    if (nCountFailed > 0) {
        returnObj += strFailedHtml;
    }

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.exec();

    updateMyNodeList(true);
}

void NodeManager::selectAliasRow(const QString& alias)
{
    for(int i=0; i < ui->tableWidgetMyMasternodes->rowCount(); ++i)
    {
        if(ui->tableWidgetMyMasternodes->item(i, 0)->text() == alias)
        {
            ui->tableWidgetMyMasternodes->selectRow(i);
            ui->tableWidgetMyMasternodes->setFocus();
            ui->tabWidget->setCurrentIndex(0);
            return;
        }
    }

    for(int i=0; i < ui->tableWidgetMySystemnodes->rowCount(); i++)
    {
        if(ui->tableWidgetMySystemnodes->item(i, 0)->text() == alias)
        {
            ui->tableWidgetMySystemnodes->selectRow(i);
            ui->tableWidgetMySystemnodes->setFocus();
            ui->tabWidget->setCurrentIndex(0);
            return;
        }
    }
}

void NodeManager::updateMyMasternodeInfo(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, CMasternode *pmn)
{
    LOCK(cs_mnlistupdate);
    bool fOldRowFound = false;
    int nNewRow = 0;

    for (int i = 0; i < ui->tableWidgetMyMasternodes->rowCount(); i++) {
        if (ui->tableWidgetMyMasternodes->item(i, 0)->text() == alias) {
            fOldRowFound = true;
            nNewRow = i;
            break;
        }
    }

    if (nNewRow == 0 && !fOldRowFound) {
        nNewRow = ui->tableWidgetMyMasternodes->rowCount();
        ui->tableWidgetMyMasternodes->insertRow(nNewRow);
    }

    QTableWidgetItem* aliasItem = new QTableWidgetItem(alias);
    QTableWidgetItem* addrItem = new QTableWidgetItem(pmn ? QString::fromStdString(pmn->addr.ToString()) : addr);
    PrivateKeyWidget *privateKeyWidget = new PrivateKeyWidget(privkey);
    QTableWidgetItem* protocolItem = new QTableWidgetItem(QString::number(pmn ? pmn->protocolVersion : -1));
    QTableWidgetItem* statusItem = new QTableWidgetItem(QString::fromStdString(pmn ? pmn->Status() : "MISSING"));
    QTableWidgetItem* activeSecondsItem = new QTableWidgetItem(QString::fromStdString(DurationToDHMS(pmn ? (int64_t)(pmn->lastPing.sigTime - pmn->sigTime) : 0)));
    QTableWidgetItem* lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M", pmn ? (int64_t)pmn->lastPing.sigTime : 0)));
    QTableWidgetItem* pubkeyItem = new QTableWidgetItem(QString::fromStdString(pmn ? EncodeDestination(PKHash(pmn->pubkey)) : ""));

    ui->tableWidgetMyMasternodes->setItem(nNewRow, 0, aliasItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 1, addrItem);
    ui->tableWidgetMyMasternodes->setCellWidget(nNewRow, 2, privateKeyWidget);
    ui->tableWidgetMyMasternodes->setColumnWidth(2, 150);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 3, protocolItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 4, statusItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 5, activeSecondsItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 6, lastSeenItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 7, pubkeyItem);
}

void NodeManager::updateMyNodeList(bool fForce)
{
    static int64_t nTimeMyListUpdated = 0;

    // automatically update my masternode list only once in MY_MASTERNODELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click
    int64_t nSecondsTillUpdate = nTimeMyListUpdated + MY_NODELIST_UPDATE_SECONDS - GetTime();
    ui->secondsLabel->setText(QString::number(nSecondsTillUpdate));

    if (nSecondsTillUpdate > 0 && !fForce) return;
    nTimeMyListUpdated = GetTime();

    ui->tableWidgetMyMasternodes->setSortingEnabled(false);
    ui->tableWidgetMySystemnodes->setSortingEnabled(false);

    for (CNodeEntry mne : masternodeConfig.getEntries()) {
        CTxIn txin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
        CMasternode* pmn = mnodeman.Find(txin);

        updateMyMasternodeInfo(QString::fromStdString(mne.getAlias()), QString::fromStdString(mne.getIp()), QString::fromStdString(mne.getPrivKey()), QString::fromStdString(mne.getTxHash()),
            QString::fromStdString(mne.getOutputIndex()), pmn);
    }

    for (CNodeEntry mne : systemnodeConfig.getEntries()) {
        CTxIn txin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
        CSystemnode* pmn = snodeman.Find(txin);

        updateMySystemnodeInfo(QString::fromStdString(mne.getAlias()), QString::fromStdString(mne.getIp()), QString::fromStdString(mne.getPrivKey()), QString::fromStdString(mne.getTxHash()),
            QString::fromStdString(mne.getOutputIndex()), pmn);
    }
    ui->tableWidgetMyMasternodes->setSortingEnabled(true);
    ui->tableWidgetMySystemnodes->setSortingEnabled(true);

    // reset "timer"
    ui->secondsLabel->setText("0");
}

void NodeManager::updateNodeList()
{
    TRY_LOCK(cs_mnlistupdate, fLockAcquired);
    if(!fLockAcquired) {
        return;
    }

    static int64_t nTimeListUpdated = GetTime();

    // to prevent high cpu usage update only once in MASTERNODELIST_UPDATE_SECONDS seconds
    // or MASTERNODELIST_FILTER_COOLDOWN_SECONDS seconds after filter was last changed
    int64_t nSecondsToWait = fFilterUpdated
                             ? nTimeFilterUpdated - GetTime() + NODELIST_FILTER_COOLDOWN_SECONDS
                             : nTimeListUpdated - GetTime() + NODELIST_UPDATE_SECONDS;

    if(fFilterUpdated) ui->mncountLabel->setText(QString::fromStdString(strprintf("Please wait... %d", nSecondsToWait)));
    if(fFilterUpdated) ui->sncountLabel->setText(QString::fromStdString(strprintf("Please wait... %d", nSecondsToWait)));

    if(nSecondsToWait > 0) return;

    nTimeListUpdated = GetTime();
    fFilterUpdated = false;

    QString strToFilter;
    ui->mncountLabel->setText("Updating...");
    ui->sncountLabel->setText("Updating...");

    ui->tableWidgetMasternodes->setSortingEnabled(false);
    ui->tableWidgetMasternodes->clearContents();
    ui->tableWidgetMasternodes->setRowCount(0);
    ui->tableWidgetSystemnodes->setSortingEnabled(false);
    ui->tableWidgetSystemnodes->clearContents();
    ui->tableWidgetSystemnodes->setRowCount(0);

    std::vector<CMasternode> vMasternodes = mnodeman.GetFullMasternodeVector();
    std::vector<CSystemnode> vSystemnodes = snodeman.GetFullSystemnodeVector();

    for (CMasternode& mn : vMasternodes)
    {
        // populate list
        // Address, Protocol, Status, Active Seconds, Last Seen, Pub Key
        QTableWidgetItem *addressItem = new QTableWidgetItem(QString::fromStdString(mn.addr.ToString()));
        QTableWidgetItem *protocolItem = new QTableWidgetItem(QString::number(mn.protocolVersion));
        QTableWidgetItem *statusItem = new QTableWidgetItem(QString::fromStdString(mn.Status()));
        QTableWidgetItem *activeSecondsItem = new QTableWidgetItem(QString::fromStdString(DurationToDHMS(mn.lastPing.sigTime - mn.sigTime)));
        QTableWidgetItem *lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M", mn.lastPing.sigTime + QDateTime::currentDateTime().offsetFromUtc())));
        QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(EncodeDestination(PKHash(mn.pubkey))));

        if (strCurrentFilter != "")
        {
            strToFilter =   addressItem->text() + " " +
                            protocolItem->text() + " " +
                            statusItem->text() + " " +
                            activeSecondsItem->text() + " " +
                            lastSeenItem->text() + " " +
                            pubkeyItem->text();
            if (!strToFilter.contains(strCurrentFilter)) continue;
        }

        ui->tableWidgetMasternodes->insertRow(0);
        ui->tableWidgetMasternodes->setItem(0, 0, addressItem);
        ui->tableWidgetMasternodes->setItem(0, 1, protocolItem);
        ui->tableWidgetMasternodes->setItem(0, 2, statusItem);
        ui->tableWidgetMasternodes->setItem(0, 3, activeSecondsItem);
        ui->tableWidgetMasternodes->setItem(0, 4, lastSeenItem);
        ui->tableWidgetMasternodes->setItem(0, 5, pubkeyItem);
    }


    for (CSystemnode& mn : vSystemnodes)
    {
        // populate list
        // Address, Protocol, Status, Active Seconds, Last Seen, Pub Key
        QTableWidgetItem *addressItem = new QTableWidgetItem(QString::fromStdString(mn.addr.ToString()));
        QTableWidgetItem *protocolItem = new QTableWidgetItem(QString::number(mn.protocolVersion));
        QTableWidgetItem *statusItem = new QTableWidgetItem(QString::fromStdString(mn.Status()));
        QTableWidgetItem *activeSecondsItem = new QTableWidgetItem(QString::fromStdString(DurationToDHMS(mn.lastPing.sigTime - mn.sigTime)));
        QTableWidgetItem *lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M", mn.lastPing.sigTime + QDateTime::currentDateTime().offsetFromUtc())));
        QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(EncodeDestination(PKHash(mn.pubkey))));

        if (strCurrentFilter != "")
        {
            strToFilter =   addressItem->text() + " " +
                            protocolItem->text() + " " +
                            statusItem->text() + " " +
                            activeSecondsItem->text() + " " +
                            lastSeenItem->text() + " " +
                            pubkeyItem->text();
            if (!strToFilter.contains(strCurrentFilter)) continue;
        }

        ui->tableWidgetSystemnodes->insertRow(0);
        ui->tableWidgetSystemnodes->setItem(0, 0, addressItem);
        ui->tableWidgetSystemnodes->setItem(0, 1, protocolItem);
        ui->tableWidgetSystemnodes->setItem(0, 2, statusItem);
        ui->tableWidgetSystemnodes->setItem(0, 3, activeSecondsItem);
        ui->tableWidgetSystemnodes->setItem(0, 4, lastSeenItem);
        ui->tableWidgetSystemnodes->setItem(0, 5, pubkeyItem);
    }

    ui->mncountLabel->setText(QString::number(ui->tableWidgetMasternodes->rowCount()));
    ui->sncountLabel->setText(QString::number(ui->tableWidgetSystemnodes->rowCount()));

    ui->tableWidgetMasternodes->setSortingEnabled(true);
    ui->tableWidgetSystemnodes->setSortingEnabled(true);
}

void NodeManager::updateNextSuperblock()
{
    Q_ASSERT(::ChainActive().Tip() != NULL);

    CBlockIndex* pindexPrev = ::ChainActive().Tip();
    if (pindexPrev)
    {
        const int nextBlock = GetNextSuperblock(pindexPrev->nHeight);
        ui->superblockLabel->setText(QString::number(nextBlock));
    }
}

void NodeManager::on_filterLineEdit_textChanged(const QString &strFilterIn)
{
    strCurrentFilter = strFilterIn;
    nTimeFilterUpdated = GetTime();
    fFilterUpdated = true;
    ui->mncountLabel->setText(QString::fromStdString(strprintf("Please wait... %d", NODELIST_FILTER_COOLDOWN_SECONDS)));
    ui->sncountLabel->setText(QString::fromStdString(strprintf("Please wait... %d", NODELIST_FILTER_COOLDOWN_SECONDS)));
}

void NodeManager::on_startButton_clicked()
{
    // Find selected node alias
    QItemSelectionModel* selectionModel = ui->tableWidgetMyMasternodes->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();

    if (selected.count() == 0) {
        selectionModel = ui->tableWidgetMySystemnodes->selectionModel();
        selected = selectionModel->selectedRows();
    }

    if (selected.count() == 0) return;

    QModelIndex index = selected.at(0);
    int nSelectedRow = index.row();
    std::string strAlias = (ui->tableWidgetMyMasternodes->item(nSelectedRow, 0)->text().toStdString() != "" ?  ui->tableWidgetMyMasternodes->item(nSelectedRow, 0)->text().toStdString() : ui->tableWidgetMySystemnodes->item(nSelectedRow, 0)->text().toStdString()) ;


    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm masternode start"),
        tr("Are you sure you want to start masternode %1?").arg(QString::fromStdString(strAlias)),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if(encStatus == walletModel->Locked) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if (!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAlias(strAlias);
        return;
    }

    StartAlias(strAlias);
}

void NodeManager::on_editButton_clicked()
{
    int g =0;
    QItemSelectionModel* selectionModel = ui->tableWidgetMyMasternodes->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0){
        selectionModel = ui->tableWidgetMySystemnodes->selectionModel();
        selected = selectionModel->selectedRows();
        g=1;
    }
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    QString strAlias = ui->tableWidgetMyMasternodes->item(r, 0)->text() == "" ? ui->tableWidgetMyMasternodes->item(r, 0)->text() : ui->tableWidgetMySystemnodes->item(r, 0)->text();
    QString strIP = ui->tableWidgetMyMasternodes->item(r, 1)->text() == "" ? ui->tableWidgetMyMasternodes->item(r, 0)->text() : ui->tableWidgetMySystemnodes->item(r, 1)->text();
    strIP.replace(QRegExp(":+\\d*"), "");

    CreateMasternodeDialog dialog(platformStyle, this);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setEditMode();

    if(g==0)
        dialog.setWindowTitle("Edit Masternode");
    else
        dialog.setWindowTitle("Edit Systemnode");

    dialog.setAlias(strAlias);
    dialog.setIP(strIP);
    if (dialog.exec())
    {
        // OK pressed
        std::string port = "9340";
        if (Params().NetworkIDString() == CBaseChainParams::TESTNET) {
            port = "18333";
        }
        BOOST_FOREACH(CNodeEntry &mne, masternodeConfig.getEntries()) {
            if (mne.getAlias() == strAlias.toStdString())
            {
                mne.setAlias(dialog.getAlias().toStdString());
                mne.setIp(dialog.getIP().toStdString() + ":" + port);
                masternodeConfig.write();
                ui->tableWidgetMyMasternodes->removeRow(r);
                updateMyNodeList(true);
            }
        }

        BOOST_FOREACH(CNodeEntry &sne, systemnodeConfig.getEntries()) {
            if (sne.getAlias() == strAlias.toStdString())
            {
                sne.setAlias(dialog.getAlias().toStdString());
                sne.setIp(dialog.getIP().toStdString() + ":" + port);
                systemnodeConfig.write();
                ui->tableWidgetMySystemnodes->removeRow(r);
                updateMyNodeList(true);
            }
        }
    }
}

void NodeManager::on_startAllButton_clicked()
{
    if (!masternodeSync.IsSynced() || !systemnodeSync.IsSynced()) {
        notReady();
        return;
    }

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm all nodes start"),
        tr("Are you sure you want to start ALL nodes?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if(encStatus == walletModel->Locked) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if (!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAll();
        return;
    }

    StartAll();
}

void NodeManager::on_startMissingButton_clicked()
{

    if(masternodeSync.RequestedMasternodeAssets <= MASTERNODE_SYNC_LIST ||
      masternodeSync.RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED) {
        QMessageBox::critical(this, tr("Command is not available right now"),
            tr("You can't use this command until masternode list is synced"));
        return;
    }

    if (!systemnodeSync.IsSynced()) {
        notReady();
        return;
    }

    StartMissingDialog dg(this);
    dg.setWindowTitle("Confirm missing nodes start");
    dg.setText("Are you sure you want to start MISSING nodes?");
    dg.setCheckboxText("Start all nodes");
    dg.setWarningText(QString("<b>") + tr("WARNING!") + QString("</b>") +
            " If checked all ACTIVE nodes will be reset.");
    bool startAll = false;

    // Display message box
    if (dg.exec()) {
        if (dg.checkboxChecked()) {
            startAll = true;
        }

        WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
        if(encStatus == walletModel->Locked)
        {
            WalletModel::UnlockContext ctx(walletModel->requestUnlock());
            if(!ctx.isValid())
            {
                // Unlock wallet was cancelled
                return;
            }
            startAll ? StartAll() : StartAll("start-missing");
            return;
        }
        startAll ? StartAll() : StartAll("start-missing");
    }
}

void NodeManager::on_tableWidgetMyMasternodes_itemSelectionChanged()
{
    if(ui->tableWidgetMyMasternodes->selectedItems().count() > 0)
    {
        ui->startButton->setEnabled(true);
    }
}

void NodeManager::on_UpdateButton_clicked()
{
    if (!systemnodeSync.IsSynced() || !masternodeSync.IsSynced()) {
        notReady();
        return;
    }

    updateMyNodeList(true);
}

void NodeManager::on_UpdateVotesButton_clicked()
{
    updateVoteList(true);
}

void NodeManager::updateVoteList(bool reset)
{
    Q_ASSERT(::ChainActive().Tip() != NULL);

    static int64_t lastVoteListUpdate = 0;

    // automatically update my masternode list only once in MY_MASTERNODELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click
    int64_t timeTillUpdate = lastVoteListUpdate + MY_NODELIST_UPDATE_SECONDS - GetTime();
    ui->voteSecondsLabel->setText(QString::number(timeTillUpdate));

    if(timeTillUpdate > 0 && !reset) return;
    lastVoteListUpdate = GetTime();

    QString strToFilter;
    ui->tableWidgetVoting->setSortingEnabled(false);
    ui->tableWidgetVoting->clearContents();
    ui->tableWidgetVoting->setRowCount(0);

    int64_t nTotalAllotted = 0;
    CBlockIndex* pindexPrev = ::ChainActive().Tip();
    if(pindexPrev == NULL)
        return;

    int blockStart = GetNextSuperblock(pindexPrev->nHeight);
    int blockEnd = blockStart + GetBudgetPaymentCycleBlocks() - 1;

    std::vector<CBudgetProposal*> winningProps = budget.GetAllProposals();
    for (CBudgetProposal* pbudgetProposal : winningProps)
    {
        CTxDestination address;
        ExtractDestination(pbudgetProposal->GetPayee(), address);

        if((int64_t)pbudgetProposal->GetRemainingPaymentCount() <= 0) continue;

        if (pbudgetProposal->fValid &&
            pbudgetProposal->nBlockStart <= blockStart &&
            pbudgetProposal->nBlockEnd >= blockEnd)
        {

            // populate list
            QTableWidgetItem *nameItem = new QTableWidgetItem(QString::fromStdString(pbudgetProposal->GetName()));
            QLabel *urlItem = new QLabel("<a href=\"" + QString::fromStdString(pbudgetProposal->GetURL()) + "\">" +
                                         QString::fromStdString(pbudgetProposal->GetURL()) + "</a>");
            urlItem->setOpenExternalLinks(true);
            urlItem->setStyleSheet("background-color: transparent;");
            QTableWidgetItem *hashItem = new QTableWidgetItem(QString::fromStdString(pbudgetProposal->GetHash().ToString()));
            GUIUtil::QTableWidgetNumberItem *blockStartItem = new GUIUtil::QTableWidgetNumberItem((int64_t)pbudgetProposal->GetBlockStart());
            GUIUtil::QTableWidgetNumberItem *blockEndItem = new GUIUtil::QTableWidgetNumberItem((int64_t)pbudgetProposal->GetBlockEnd());
            GUIUtil::QTableWidgetNumberItem *paymentsItem = new GUIUtil::QTableWidgetNumberItem((int64_t)pbudgetProposal->GetTotalPaymentCount());
            GUIUtil::QTableWidgetNumberItem *remainingPaymentsItem = new GUIUtil::QTableWidgetNumberItem((int64_t)pbudgetProposal->GetRemainingPaymentCount());
            GUIUtil::QTableWidgetNumberItem *yesVotesItem = new GUIUtil::QTableWidgetNumberItem((int64_t)pbudgetProposal->GetYeas());
            GUIUtil::QTableWidgetNumberItem *noVotesItem = new GUIUtil::QTableWidgetNumberItem((int64_t)pbudgetProposal->GetNays());
            GUIUtil::QTableWidgetNumberItem *abstainVotesItem = new GUIUtil::QTableWidgetNumberItem((int64_t)pbudgetProposal->GetAbstains());
            QTableWidgetItem *AddressItem = new QTableWidgetItem(QString::fromStdString(EncodeDestination(address)));
            GUIUtil::QTableWidgetNumberItem *totalPaymentItem = new GUIUtil::QTableWidgetNumberItem((pbudgetProposal->GetAmount()*pbudgetProposal->GetTotalPaymentCount())/100000000);
            GUIUtil::QTableWidgetNumberItem *monthlyPaymentItem = new GUIUtil::QTableWidgetNumberItem(pbudgetProposal->GetAmount()/100000000);

            ui->tableWidgetVoting->insertRow(0);
            ui->tableWidgetVoting->setItem(0, 0, nameItem);
            ui->tableWidgetVoting->setCellWidget(0, 1, urlItem);
            ui->tableWidgetVoting->setItem(0, 2, hashItem);
            ui->tableWidgetVoting->setItem(0, 3, blockStartItem);
            ui->tableWidgetVoting->setItem(0, 4, blockEndItem);
            ui->tableWidgetVoting->setItem(0, 5, paymentsItem);
            ui->tableWidgetVoting->setItem(0, 6, remainingPaymentsItem);
            ui->tableWidgetVoting->setItem(0, 7, yesVotesItem);
            ui->tableWidgetVoting->setItem(0, 8, noVotesItem);
            ui->tableWidgetVoting->setItem(0, 9, abstainVotesItem);
            ui->tableWidgetVoting->setItem(0, 10, AddressItem);
            ui->tableWidgetVoting->setItem(0, 11, totalPaymentItem);
            ui->tableWidgetVoting->setItem(0, 12, monthlyPaymentItem);

            std::string projected;
            if ((int64_t)pbudgetProposal->GetYeas() - (int64_t)pbudgetProposal->GetNays() > (ui->tableWidgetMasternodes->rowCount()/10)){
                nTotalAllotted += pbudgetProposal->GetAmount()/100000000;
                projected = "Yes";
            } else {
                projected = "No";
            }
            QTableWidgetItem *projectedItem = new QTableWidgetItem(QString::fromStdString(projected));
            ui->tableWidgetVoting->setItem(0, 13, projectedItem);
        }
    }

    ui->totalAllottedLabel->setText(QString::number(nTotalAllotted));
    ui->tableWidgetVoting->setSortingEnabled(true);

    // reset "timer"
    ui->voteSecondsLabel->setText("0");
}

void NodeManager::VoteMany(std::string strCommand)
{
    std::vector<CNodeEntry> mnEntries;
    mnEntries = masternodeConfig.getEntries();

    int nVote = VOTE_ABSTAIN;
    if(strCommand == "yea") nVote = VOTE_YES;
    if(strCommand == "nay") nVote = VOTE_NO;

    // Find selected Budget Hash
    QItemSelectionModel* selectionModel = ui->tableWidgetVoting->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string strHash = ui->tableWidgetVoting->item(r, 2)->text().toStdString();
    uint256 hash;
    hash.SetHex(strHash);

    const int blockStart = ui->tableWidgetVoting->item(r, 3)->text().toInt();
    const int blockEnd = ui->tableWidgetVoting->item(r, 4)->text().toInt();
    if (!budget.CanSubmitVotes(blockStart, blockEnd))
    {
        QMessageBox::critical(
            this,
            tr("Voting Details"),
            tr(
                "Sorry, your vote has not been added to the proposal. "
                "The proposal voting is currently disabled as we're too close to the proposal payment."
            )
        );
        return;
    }

    int success = 0;
    int failed = 0;
    std::string statusObj;

    for (CNodeEntry mne : masternodeConfig.getEntries()) {
        std::string errorMessage;
        std::vector<unsigned char> vchMasterNodeSignature;
        std::string strMasterNodeSignMessage;

        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;
        CPubKey pubKeyMasternode;
        CKey keyMasternode;

        if(!legacySigner.SetKey(mne.getPrivKey(), keyMasternode, pubKeyMasternode)){
            failed++;
            statusObj += "\nFailed to vote with " + mne.getAlias() + ". Masternode signing error, could not set key correctly: " + errorMessage;
            continue;
        }

        CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
        if(pmn == NULL)
        {
            failed++;
            statusObj += "\nFailed to vote with " + mne.getAlias() + ". Error: Can't find masternode by pubkey";
            continue;
        }

        CBudgetVote vote(pmn->vin, hash, nVote);
        if(!vote.Sign(keyMasternode, pubKeyMasternode)){
            failed++;
            statusObj += "\nFailed to vote with " + mne.getAlias() + ". Error: Failure to sign";
            continue;
        }

        std::string strError = "";
        if(budget.SubmitProposalVote(vote, strError)) {
            vote.Relay(*g_rpc_node->connman);
            success++;
        } else {
            failed++;
            statusObj += "\nFailed to vote with " + mne.getAlias() + ". Error: " + strError;
        }
    }
    std::string returnObj;
    returnObj = strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed);
    if (failed > 0)
        returnObj += statusObj;

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.setWindowTitle(tr("Voting Details"));
    msg.exec();
    updateVoteList(true);
}

void NodeManager::on_voteManyYesButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm vote-many"),
        tr("Are you sure you want to vote with ALL of your masternodes?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked)
    {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid())
        {
            // Unlock wallet was cancelled
            return;
        }
        VoteMany("yea");
        return;
    }

    VoteMany("yea");
}

void NodeManager::on_voteManyNoButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm vote-many"),
        tr("Are you sure you want to vote with ALL of your masternodes?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked)
    {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid())
        {
            // Unlock wallet was cancelled
            return;
        }
        VoteMany("nay");
        return;
    }

    VoteMany("nay");
}

void NodeManager::on_voteManyAbstainButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm vote-many"),
        tr("Are you sure you want to vote with ALL of your masternodes?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked)
    {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid())
        {
            // Unlock wallet was cancelled
            return;
        }
        VoteMany("abstain");
        return;
    }

    VoteMany("abstain");
}

void NodeManager::on_tableWidgetVoting_itemSelectionChanged()
{
    if(ui->tableWidgetVoting->selectedItems().count() > 0)
    {
        ui->voteManyYesButton->setEnabled(true);
        ui->voteManyNoButton->setEnabled(true);
    }
}

void NodeManager::on_CreateNewMasternode_clicked()
{
    if(!walletModel)
        return;

    if(!walletModel->getOptionsModel())
        return;

    CreateMasternodeDialog dialog(platformStyle, this);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setWindowTitle("Create a New Masternode");
    dialog.setWalletModel(walletModel);
    dialog.setMode(0);
    QString formattedAmount = CrownUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(),
                                                           10000 * COIN);
    dialog.setNoteLabel("*This action will send " + formattedAmount + " to yourself");
    dialog.exec();
}

void NodeManager::on_CreateNewSystemnode_clicked()
{
    if(!walletModel)
        return;

    if(!walletModel->getOptionsModel())
        return;

    CreateSystemnodeDialog dialog(platformStyle, this);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setWindowTitle("Create a New Systemnode");
    dialog.setWalletModel(walletModel);
    dialog.setMode(1);
    QString formattedAmount = CrownUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(),
                                                           500 * COIN);
    dialog.setNoteLabel("This action will send " + formattedAmount + " to yourself");
    dialog.exec();

}

void NodeManager::notReady()
{
     QMessageBox::critical(this, tr("Command is not available right now"),
         tr("You can't use this command until systemnode list is synced"));
     return;
}

void NodeManager::updateMySystemnodeInfo(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, CSystemnode *pmn)
{
    LOCK(cs_mnlistupdate);
    bool fOldRowFound = false;
    int nNewRow = 0;

    for (int i = 0; i < ui->tableWidgetMySystemnodes->rowCount(); i++) {
        if (ui->tableWidgetMySystemnodes->item(i, 0)->text() == alias) {
            fOldRowFound = true;
            nNewRow = i;
            break;
        }
    }

    if (nNewRow == 0 && !fOldRowFound) {
        nNewRow = ui->tableWidgetMySystemnodes->rowCount();
        ui->tableWidgetMySystemnodes->insertRow(nNewRow);
    }

    QTableWidgetItem* aliasItem = new QTableWidgetItem(alias);
    QTableWidgetItem* addrItem = new QTableWidgetItem(pmn ? QString::fromStdString(pmn->addr.ToString()) : addr);
    PrivateKeyWidget *privateKeyWidget = new PrivateKeyWidget(privkey);
    QTableWidgetItem* protocolItem = new QTableWidgetItem(QString::number(pmn ? pmn->protocolVersion : -1));
    QTableWidgetItem* statusItem = new QTableWidgetItem(QString::fromStdString(pmn ? pmn->Status() : "MISSING"));
    QTableWidgetItem* activeSecondsItem = new QTableWidgetItem(QString::fromStdString(DurationToDHMS(pmn ? (int64_t)(pmn->lastPing.sigTime - pmn->sigTime) : 0)));
    QTableWidgetItem* lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M", pmn ? (int64_t)pmn->lastPing.sigTime : 0)));
    QTableWidgetItem* pubkeyItem = new QTableWidgetItem(QString::fromStdString(pmn ? EncodeDestination(PKHash(pmn->pubkey)) : ""));

    ui->tableWidgetMySystemnodes->setItem(nNewRow, 0, aliasItem);
    ui->tableWidgetMySystemnodes->setItem(nNewRow, 1, addrItem);
    ui->tableWidgetMySystemnodes->setCellWidget(nNewRow, 2, privateKeyWidget);
    ui->tableWidgetMySystemnodes->setColumnWidth(2, 150);
    ui->tableWidgetMySystemnodes->setItem(nNewRow, 3, protocolItem);
    ui->tableWidgetMySystemnodes->setItem(nNewRow, 4, statusItem);
    ui->tableWidgetMySystemnodes->setItem(nNewRow, 5, activeSecondsItem);
    ui->tableWidgetMySystemnodes->setItem(nNewRow, 6, lastSeenItem);
    ui->tableWidgetMySystemnodes->setItem(nNewRow, 7, pubkeyItem);
}

void NodeManager::on_tableWidgetMySystemnodes_itemSelectionChanged()
{
    if (ui->tableWidgetMySystemnodes->selectedItems().count() > 0) {
        ui->startButton->setEnabled(true);
    }
}
