#include <qt/systemnodelist.h>
#include <qt/forms/ui_systemnodelist.h>

#include <init.h>
#include <key_io.h>
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

#include <QMessageBox>
#include <QTimer>

RecursiveMutex cs_systemnodes;

SystemnodeList::SystemnodeList(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SystemnodeList),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);

    ui->startButton->setEnabled(false);

    int columnAliasWidth = 100;
    int columnAddressWidth = 200;
    int columnProtocolWidth = 60;
    int columnStatusWidth = 80;
    int columnActiveWidth = 130;
    int columnLastSeenWidth = 130;

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

    QAction* startAliasAction = new QAction(tr("Start alias"), this);
    contextMenu = new QMenu();
    contextMenu->addAction(startAliasAction);
    connect(ui->tableWidgetMySystemnodes, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
    connect(startAliasAction, SIGNAL(triggered()), this, SLOT(on_startButton_clicked()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNodeList()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateMyNodeList()));
    timer->start(1000);

    // Fill MN list
    fFilterUpdated = false;
    nTimeFilterUpdated = GetTime();
    updateNodeList();
}

SystemnodeList::~SystemnodeList()
{
    delete ui;
}

void SystemnodeList::notReady()
{
     QMessageBox::critical(this, tr("Command is not available right now"),
         tr("You can't use this command until systemnode list is synced"));
     return;
}

void SystemnodeList::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    if(model) {
        // try to update list when systemnode count changes
        connect(clientModel, SIGNAL(strSystemnodesChanged(QString)), this, SLOT(updateNodeList()));
    }
}

void SystemnodeList::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
}

void SystemnodeList::showContextMenu(const QPoint& point)
{
    QTableWidgetItem* item = ui->tableWidgetMySystemnodes->itemAt(point);
    if (item) contextMenu->exec(QCursor::pos());
}

void SystemnodeList::StartAlias(std::string strAlias)
{
    std::string strStatusHtml;
    strStatusHtml += "<center>Alias: " + strAlias;

    for (CNodeEntry mne : systemnodeConfig.getEntries()) {
        if (mne.getAlias() == strAlias) {
            std::string strError;
            CSystemnodeBroadcast mnb;

            bool fSuccess = CSystemnodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

            if (fSuccess) {
                strStatusHtml += "<br>Successfully started systemnode.";
                snodeman.UpdateSystemnodeList(mnb);
                mnb.Relay();
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

void SystemnodeList::StartAll(std::string strCommand)
{
    int nCountSuccessful = 0;
    int nCountFailed = 0;
    std::string strFailedHtml;

    for (CNodeEntry mne : systemnodeConfig.getEntries()) {
        std::string strError;
        CSystemnodeBroadcast mnb;

        CTxIn txin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
        CSystemnode* pmn = snodeman.Find(txin);

        if (strCommand == "start-missing" && pmn) continue;

        bool fSuccess = CSystemnodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

        if (fSuccess) {
            nCountSuccessful++;
            snodeman.UpdateSystemnodeList(mnb);
            mnb.Relay();
        } else {
            nCountFailed++;
            strFailedHtml += "\nFailed to start " + mne.getAlias() + ". Error: " + strError;
        }
    }
    GetMainWallet()->Lock();

    std::string returnObj;
    returnObj = strprintf("Successfully started %d systemnodes, failed to start %d, total %d", nCountSuccessful, nCountFailed, nCountFailed + nCountSuccessful);
    if (nCountFailed > 0) {
        returnObj += strFailedHtml;
    }

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.exec();

    updateMyNodeList(true);
}

void SystemnodeList::updateMySystemnodeInfo(QString strAlias, QString strAddr, CSystemnode* pmn)
{
    LOCK(cs_mnlistupdate);
    bool fOldRowFound = false;
    int nNewRow = 0;

    for (int i = 0; i < ui->tableWidgetMySystemnodes->rowCount(); i++) {
        if (ui->tableWidgetMySystemnodes->item(i, 0)->text() == strAlias) {
            fOldRowFound = true;
            nNewRow = i;
            break;
        }
    }

    if (nNewRow == 0 && !fOldRowFound) {
        nNewRow = ui->tableWidgetMySystemnodes->rowCount();
        ui->tableWidgetMySystemnodes->insertRow(nNewRow);
    }

    QTableWidgetItem* aliasItem = new QTableWidgetItem(strAlias);
    QTableWidgetItem* addrItem = new QTableWidgetItem(pmn ? QString::fromStdString(pmn->addr.ToString()) : strAddr);
    QTableWidgetItem* protocolItem = new QTableWidgetItem(QString::number(pmn ? pmn->protocolVersion : -1));
    QTableWidgetItem* statusItem = new QTableWidgetItem(QString::fromStdString(pmn ? pmn->Status() : "MISSING"));
    QTableWidgetItem* activeSecondsItem = new QTableWidgetItem(QString::fromStdString(DurationToDHMS(pmn ? (int64_t)(pmn->lastPing.sigTime - pmn->sigTime) : 0)));
    QTableWidgetItem* lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M", pmn ? (int64_t)pmn->lastPing.sigTime : 0)));
    QTableWidgetItem* pubkeyItem = new QTableWidgetItem(QString::fromStdString(pmn ? EncodeDestination(PKHash(pmn->pubkey)) : ""));

    ui->tableWidgetMySystemnodes->setItem(nNewRow, 0, aliasItem);
    ui->tableWidgetMySystemnodes->setItem(nNewRow, 1, addrItem);
    ui->tableWidgetMySystemnodes->setItem(nNewRow, 2, protocolItem);
    ui->tableWidgetMySystemnodes->setItem(nNewRow, 3, statusItem);
    ui->tableWidgetMySystemnodes->setItem(nNewRow, 4, activeSecondsItem);
    ui->tableWidgetMySystemnodes->setItem(nNewRow, 5, lastSeenItem);
    ui->tableWidgetMySystemnodes->setItem(nNewRow, 6, pubkeyItem);
}

void SystemnodeList::updateMyNodeList(bool fForce)
{
    static int64_t nTimeMyListUpdated = 0;

    // automatically update my systemnode list only once in MY_SYSTEMNODELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click
    int64_t nSecondsTillUpdate = nTimeMyListUpdated + MY_SYSTEMNODELIST_UPDATE_SECONDS - GetTime();
    ui->secondsLabel->setText(QString::number(nSecondsTillUpdate));

    if (nSecondsTillUpdate > 0 && !fForce) return;
    nTimeMyListUpdated = GetTime();

    ui->tableWidgetMySystemnodes->setSortingEnabled(false);
    for (CNodeEntry mne : systemnodeConfig.getEntries()) {
        CTxIn txin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
        CSystemnode* pmn = snodeman.Find(txin);
        updateMySystemnodeInfo(QString::fromStdString(mne.getAlias()), QString::fromStdString(mne.getIp()), pmn);
    }
    ui->tableWidgetMySystemnodes->setSortingEnabled(true);

    // reset "timer"
    ui->secondsLabel->setText("0");
}

void SystemnodeList::updateNodeList()
{
    TRY_LOCK(cs_mnlistupdate, fLockAcquired);
    if(!fLockAcquired) {
        return;
    }

    static int64_t nTimeListUpdated = GetTime();

    // to prevent high cpu usage update only once in SYSTEMNODELIST_UPDATE_SECONDS seconds
    // or SYSTEMNODELIST_FILTER_COOLDOWN_SECONDS seconds after filter was last changed
    int64_t nSecondsToWait = fFilterUpdated
                             ? nTimeFilterUpdated - GetTime() + SYSTEMNODELIST_FILTER_COOLDOWN_SECONDS
                             : nTimeListUpdated - GetTime() + SYSTEMNODELIST_UPDATE_SECONDS;

    if(fFilterUpdated) ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", nSecondsToWait)));
    if(nSecondsToWait > 0) return;

    nTimeListUpdated = GetTime();
    fFilterUpdated = false;

    QString strToFilter;
    ui->countLabel->setText("Updating...");
    ui->tableWidgetSystemnodes->setSortingEnabled(false);
    ui->tableWidgetSystemnodes->clearContents();
    ui->tableWidgetSystemnodes->setRowCount(0);
    std::vector<CSystemnode> vSystemnodes = snodeman.GetFullSystemnodeVector();

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

    ui->countLabel->setText(QString::number(ui->tableWidgetSystemnodes->rowCount()));
    ui->tableWidgetSystemnodes->setSortingEnabled(true);
}

void SystemnodeList::on_filterLineEdit_textChanged(const QString &strFilterIn)
{
    strCurrentFilter = strFilterIn;
    nTimeFilterUpdated = GetTime();
    fFilterUpdated = true;
    ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", SYSTEMNODELIST_FILTER_COOLDOWN_SECONDS)));
}

void SystemnodeList::on_startButton_clicked()
{
    // Find selected node alias
    QItemSelectionModel* selectionModel = ui->tableWidgetMySystemnodes->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();

    if (selected.count() == 0) return;

    QModelIndex index = selected.at(0);
    int nSelectedRow = index.row();
    std::string strAlias = ui->tableWidgetMySystemnodes->item(nSelectedRow, 0)->text().toStdString();

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm systemnode start"),
        tr("Are you sure you want to start systemnode %1?").arg(QString::fromStdString(strAlias)),
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

void SystemnodeList::on_startAllButton_clicked()
{
    if (!systemnodeSync.IsSynced()) {
        notReady();
        return;
    }

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm all systemnodes start"),
        tr("Are you sure you want to start ALL systemnodes?"),
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

void SystemnodeList::on_startMissingButton_clicked()
{
    if (!systemnodeSync.IsSynced()) {
        notReady();
        return;
    }

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this,
        tr("Confirm missing systemnodes start"),
        tr("Are you sure you want to start MISSING systemnodes?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if(encStatus == walletModel->Locked) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if (!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAll("start-missing");
        return;
    }

    StartAll("start-missing");
}

void SystemnodeList::on_tableWidgetMySystemnodes_itemSelectionChanged()
{
    if (ui->tableWidgetMySystemnodes->selectedItems().count() > 0) {
        ui->startButton->setEnabled(true);
    }
}

void SystemnodeList::on_UpdateButton_clicked()
{
    if (!systemnodeSync.IsSynced()) {
        notReady();
        return;
    }

    updateMyNodeList(true);
}
