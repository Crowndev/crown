#include <qt/masternodelist.h>
#include <qt/forms/ui_masternodelist.h>

#include <crown/legacysigner.h>
#include <init.h>
#include <key_io.h>
#include <masternode/activemasternode.h>
#include <masternode/masternode-budget.h>
#include <masternode/masternode-sync.h>
#include <masternode/masternodeconfig.h>
#include <masternode/masternodeman.h>
#include <node/context.h>
#include <nodeconfig.h>
#include <rpc/blockchain.h>
#include <qt/addresstablemodel.h>
#include <qt/crownunits.h>
#include <qt/clientmodel.h>
#include <qt/privatekeywidget.h>
#include <qt/createmasternodedialog.h>
#include <qt/datetablewidgetitem.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/startmissingdialog.h>
#include <qt/walletmodel.h>
#include <sync.h>
#include <util/time.h>
#include <wallet/wallet.h>

#include <QMessageBox>
#include <QTimer>

int GetOffsetFromUtc()
{
#if QT_VERSION < 0x050200
    const QDateTime dateTime1 = QDateTime::currentDateTime();
    const QDateTime dateTime2 = QDateTime(dateTime1.date(), dateTime1.time(), Qt::UTC);
    return dateTime1.secsTo(dateTime2);
#else
    return QDateTime::currentDateTime().offsetFromUtc();
#endif
}

RecursiveMutex cs_masternodes;

MasternodeList::MasternodeList(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MasternodeList),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);

    ui->startButton->setEnabled(false);
    ui->voteManyYesButton->setEnabled(false);
    ui->voteManyNoButton->setEnabled(false);

    int columnAliasWidth = 100;
    int columnAddressWidth = 200;
    int columnProtocolWidth = 60;
    int columnStatusWidth = 80;
    int columnActiveWidth = 130;
    int columnLastSeenWidth = 130;

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

    ui->tableWidgetMyMasternodes->setContextMenuPolicy(Qt::CustomContextMenu);

    QAction* startAliasAction = new QAction(tr("Start alias"), this);
    QAction *editAction = new QAction(tr("Edit"), this);
    contextMenu = new QMenu();
    contextMenu->addAction(startAliasAction);
    contextMenu->addAction(editAction);
    connect(ui->tableWidgetMyMasternodes, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
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
}

MasternodeList::~MasternodeList()
{
    delete ui;
}

void MasternodeList::notReady()
{
     QMessageBox::critical(this, tr("Command is not available right now"),
         tr("You can't use this command until masternode list is synced"));
     return;
}

void MasternodeList::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    if(model) {
        // try to update list when masternode count changes
        connect(clientModel, SIGNAL(strMasternodesChanged(QString)), this, SLOT(updateNodeList()));
    }
}

void MasternodeList::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
}

void MasternodeList::showContextMenu(const QPoint& point)
{
    QTableWidgetItem* item = ui->tableWidgetMyMasternodes->itemAt(point);
    if (item) contextMenu->exec(QCursor::pos());
}

void MasternodeList::StartAlias(std::string strAlias)
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
    strStatusHtml += "</center>";

    QMessageBox msg;
    msg.setText(QString::fromStdString(strStatusHtml));
    msg.exec();

    updateMyNodeList(true);
}

void MasternodeList::StartAll(std::string strCommand)
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
    GetMainWallet()->Lock();

    std::string returnObj;
    returnObj = strprintf("Successfully started %d masternodes, failed to start %d, total %d", nCountSuccessful, nCountFailed, nCountFailed + nCountSuccessful);
    if (nCountFailed > 0) {
        returnObj += strFailedHtml;
    }

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.exec();

    updateMyNodeList(true);
}

void MasternodeList::selectAliasRow(const QString& alias)
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
}

void MasternodeList::updateMyMasternodeInfo(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, CMasternode *pmn)
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

void MasternodeList::updateMyNodeList(bool fForce)
{
    static int64_t nTimeMyListUpdated = 0;

    // automatically update my masternode list only once in MY_MASTERNODELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click
    int64_t nSecondsTillUpdate = nTimeMyListUpdated + MY_MASTERNODELIST_UPDATE_SECONDS - GetTime();
    ui->secondsLabel->setText(QString::number(nSecondsTillUpdate));

    if (nSecondsTillUpdate > 0 && !fForce) return;
    nTimeMyListUpdated = GetTime();

    ui->tableWidgetMyMasternodes->setSortingEnabled(false);
    for (CNodeEntry mne : masternodeConfig.getEntries()) {
        CTxIn txin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
        CMasternode* pmn = mnodeman.Find(txin);

        updateMyMasternodeInfo(QString::fromStdString(mne.getAlias()), QString::fromStdString(mne.getIp()), QString::fromStdString(mne.getPrivKey()), QString::fromStdString(mne.getTxHash()),
            QString::fromStdString(mne.getOutputIndex()), pmn);
    }
    ui->tableWidgetMyMasternodes->setSortingEnabled(true);

    // reset "timer"
    ui->secondsLabel->setText("0");
}

void MasternodeList::updateNodeList()
{
    TRY_LOCK(cs_mnlistupdate, fLockAcquired);
    if(!fLockAcquired) {
        return;
    }

    static int64_t nTimeListUpdated = GetTime();

    // to prevent high cpu usage update only once in MASTERNODELIST_UPDATE_SECONDS seconds
    // or MASTERNODELIST_FILTER_COOLDOWN_SECONDS seconds after filter was last changed
    int64_t nSecondsToWait = fFilterUpdated
                             ? nTimeFilterUpdated - GetTime() + MASTERNODELIST_FILTER_COOLDOWN_SECONDS
                             : nTimeListUpdated - GetTime() + MASTERNODELIST_UPDATE_SECONDS;

    if(fFilterUpdated) ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", nSecondsToWait)));
    if(nSecondsToWait > 0) return;

    nTimeListUpdated = GetTime();
    fFilterUpdated = false;

    QString strToFilter;
    ui->countLabel->setText("Updating...");
    ui->tableWidgetMasternodes->setSortingEnabled(false);
    ui->tableWidgetMasternodes->clearContents();
    ui->tableWidgetMasternodes->setRowCount(0);
    std::vector<CMasternode> vMasternodes = mnodeman.GetFullMasternodeVector();

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

    ui->countLabel->setText(QString::number(ui->tableWidgetMasternodes->rowCount()));
    ui->tableWidgetMasternodes->setSortingEnabled(true);
}

void MasternodeList::updateNextSuperblock()
{
    Q_ASSERT(::ChainActive().Tip() != NULL);

    CBlockIndex* pindexPrev = ::ChainActive().Tip();
    if (pindexPrev)
    {
        const int nextBlock = GetNextSuperblock(pindexPrev->nHeight);
        ui->superblockLabel->setText(QString::number(nextBlock));
    }
}

void MasternodeList::on_filterLineEdit_textChanged(const QString &strFilterIn)
{
    strCurrentFilter = strFilterIn;
    nTimeFilterUpdated = GetTime();
    fFilterUpdated = true;
    ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", MASTERNODELIST_FILTER_COOLDOWN_SECONDS)));
}

void MasternodeList::on_startButton_clicked()
{
    // Find selected node alias
    QItemSelectionModel* selectionModel = ui->tableWidgetMyMasternodes->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();

    if (selected.count() == 0) return;

    QModelIndex index = selected.at(0);
    int nSelectedRow = index.row();
    std::string strAlias = ui->tableWidgetMyMasternodes->item(nSelectedRow, 0)->text().toStdString();

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

void MasternodeList::on_editButton_clicked()
{
    CreateMasternodeDialog dialog(this);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setEditMode();
    dialog.setWindowTitle("Edit Masternode");
    
    QItemSelectionModel* selectionModel = ui->tableWidgetMyMasternodes->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    QString strAlias = ui->tableWidgetMyMasternodes->item(r, 0)->text();
    QString strIP = ui->tableWidgetMyMasternodes->item(r, 1)->text();
    strIP.replace(QRegExp(":+\\d*"), "");

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

    }
    else
    {
        // Cancel pressed
    }
}

void MasternodeList::on_startAllButton_clicked()
{
    if (!masternodeSync.IsSynced()) {
        notReady();
        return;
    }

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm all masternodes start"),
        tr("Are you sure you want to start ALL masternodes?"),
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

void MasternodeList::on_startMissingButton_clicked()
{

    if(masternodeSync.RequestedMasternodeAssets <= MASTERNODE_SYNC_LIST ||
      masternodeSync.RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED) {
        QMessageBox::critical(this, tr("Command is not available right now"),
            tr("You can't use this command until masternode list is synced"));
        return;
    }

    StartMissingDialog dg(this);
    dg.setWindowTitle("Confirm missing masternodes start");
    dg.setText("Are you sure you want to start MISSING masternodes?");
    dg.setCheckboxText("Start all nodes");
    dg.setWarningText(QString("<b>") + tr("WARNING!") + QString("</b>") +
            " If checked all ACTIVE masternodes will be reset.");
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

void MasternodeList::on_tableWidgetMyMasternodes_itemSelectionChanged()
{
    if(ui->tableWidgetMyMasternodes->selectedItems().count() > 0)
    {
        ui->startButton->setEnabled(true);
    }
}

void MasternodeList::on_UpdateButton_clicked()
{
    updateMyNodeList(true);
}

void MasternodeList::on_UpdateVotesButton_clicked()
{
    updateVoteList(true);
}

void MasternodeList::updateVoteList(bool reset)
{
    Q_ASSERT(::ChainActive().Tip() != NULL);

    static int64_t lastVoteListUpdate = 0;

    // automatically update my masternode list only once in MY_MASTERNODELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click
    int64_t timeTillUpdate = lastVoteListUpdate + MY_MASTERNODELIST_UPDATE_SECONDS - GetTime();
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

void MasternodeList::VoteMany(std::string strCommand)
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

void MasternodeList::on_voteManyYesButton_clicked()
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

void MasternodeList::on_voteManyNoButton_clicked()
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

void MasternodeList::on_voteManyAbstainButton_clicked()
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

void MasternodeList::on_tableWidgetVoting_itemSelectionChanged()
{
    if(ui->tableWidgetVoting->selectedItems().count() > 0)
    {
        ui->voteManyYesButton->setEnabled(true);
        ui->voteManyNoButton->setEnabled(true);
    }
}

void MasternodeList::on_CreateNewMasternode_clicked()
{
	if(!walletModel)
	    return;
	    
	if(!walletModel->getOptionsModel())
	    return;

    CreateMasternodeDialog dialog(this);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setWindowTitle("Create a New Masternode");
    QString formattedAmount = CrownUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), 
                                                           10000 * COIN);
    dialog.setNoteLabel("*This action will send " + formattedAmount + " to yourself");
    if (dialog.exec())
    {
        // OK Pressed
        QString label = dialog.getLabel();
        QString address = walletModel->getAddressTableModel()->addRow(AddressTableModel::Receive, label, "", OutputType::LEGACY);
        SendAssetsRecipient recipient(address, label, 10000 * COIN, "");
        recipient.asset = Params().GetConsensus().subsidy_asset;
        QList<SendAssetsRecipient> recipients;
        recipients.append(recipient);

        std::vector<std::shared_ptr<CWallet>> wallets = GetWallets();

        // Get outputs before and after transaction
        std::vector<COutput> vPossibleCoinsBefore;
        wallets[0]->AvailableCoins(vPossibleCoinsBefore, Params().GetConsensus().subsidy_asset, true, nullptr, 0, MAX_MONEY, MAX_MONEY, 0);

        sendDialog->setModel(walletModel);
        sendDialog->send(recipients);

        std::vector<COutput> vPossibleCoinsAfter;
        wallets[0]->AvailableCoins(vPossibleCoinsAfter, Params().GetConsensus().subsidy_asset, true, nullptr, 0, MAX_MONEY, MAX_MONEY, 0);

        for (auto& out : vPossibleCoinsAfter)
        {
            std::vector<COutput>::iterator it = std::find(vPossibleCoinsBefore.begin(), vPossibleCoinsBefore.end(), out);
            if (it == vPossibleCoinsBefore.end()) {
                // Not found so this is a new element

                COutPoint outpoint = COutPoint(out.tx->GetHash(), out.i);
                wallets[0]->LockCoin(outpoint);

                // Generate a key
                CKey secret;
                secret.MakeNewKey(false);
                std::string privateKey = EncodeSecret(secret);
                std::string port = "9340";
                if (Params().NetworkIDString() == CBaseChainParams::TESTNET) {
                    port = "18333";
                }

                masternodeConfig.add(dialog.getAlias().toStdString(), dialog.getIP().toStdString() + ":" + port, 
                        privateKey, out.tx->GetHash().ToString(), strprintf("%d", out.i));
                masternodeConfig.write();
                updateMyNodeList(true);
            }
        }
    } else {
        // Cancel Pressed
    }
}
