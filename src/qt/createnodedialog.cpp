#include <qt/createnodedialog.h>
#include <qt/forms/ui_createnodedialog.h>
#include <node/ui_interface.h>
#include <net.h>
#include <key_io.h>
#include <qt/sendcollateraldialog.h>

#include <qt/walletmodel.h>
#include <qt/addresstablemodel.h>
#include <qt/crownunits.h>
#include <wallet/wallet.h>
#include <masternode/masternodeconfig.h>
#include <systemnode/systemnodeconfig.h>

#include <QMessageBox>
#include <QPushButton>
#include <QDebug>

CreateNodeDialog::CreateNodeDialog(const PlatformStyle *_platformStyle, QWidget *parent) :
    QDialog(parent),
    editMode(false),
    startAlias(""),
    platformStyle(_platformStyle),    
    ui(new Ui::CreateNodeDialog)
{
    ui->setupUi(this);
}

CreateNodeDialog::~CreateNodeDialog()
{
    delete ui;
}

void CreateNodeDialog::setWalletModel(WalletModel* model)
{
    if(!model)
        return;	
    walletmodel = model;
}

QString CreateNodeDialog::getAlias()
{
    return ui->aliasEdit->text();
}

void CreateNodeDialog::setAlias(QString alias)
{
    ui->aliasEdit->setText(alias);
    startAlias = alias;
}

QString CreateNodeDialog::getIP()
{
    return ui->ipEdit->text();
}

void CreateNodeDialog::setIP(QString ip)
{
    ui->ipEdit->setText(ip);
}

QString CreateNodeDialog::getLabel()
{
    return ui->labelEdit->text();
}

void CreateNodeDialog::setNoteLabel(QString text)
{
    ui->noteLabel->setText(text);
}

void CreateNodeDialog::setEditMode()
{
    ui->labelEdit->setVisible(false);
    ui->label->setVisible(false);
    ui->buttonBox->move(ui->buttonBox->pos().x(),
                        ui->buttonBox->pos().y() - 50);
    resize(size().width(), size().height() - 50);
    editMode = true;
}

bool CreateNodeDialog::CheckAlias()
{
    // Check alias
    if (ui->aliasEdit->text().isEmpty())
    {
        ui->aliasEdit->setValid(false);
        QMessageBox::warning(this, windowTitle(), tr("Alias is Required"), QMessageBox::Ok, QMessageBox::Ok);
        return false;
    }
    // Check white-space characters
    if (ui->aliasEdit->text().contains(QRegExp("\\s")))
    {
        ui->aliasEdit->setValid(false);
        QMessageBox::warning(this, windowTitle(), tr("Alias cannot contain white-space characters"), QMessageBox::Ok, QMessageBox::Ok);
        return false;
    }
    // Check if alias exists
    if (aliasExists(ui->aliasEdit->text()))
    {
        QString aliasEditText = ui->aliasEdit->text();
        if (!(startAlias != "" && aliasEditText == startAlias))
        {
            ui->aliasEdit->setValid(false);
            QMessageBox::warning(this, windowTitle(), tr("Alias %1 Already Exists").arg(ui->aliasEdit->text()), 
                    QMessageBox::Ok, QMessageBox::Ok);
            return false;
        }
    }
    return true;
}

bool CreateNodeDialog::CheckIP()
{
    QString ip = ui->ipEdit->text();
    // Check ip
    if (ip.isEmpty())
    {
        ui->ipEdit->setValid(false);
        QMessageBox::warning(this, windowTitle(), tr("IP is Required"), QMessageBox::Ok, QMessageBox::Ok);
        return false;
    }
    // Check if port is not entered
    if (ip.contains(QRegExp(":+[0-9]")))
    {
        ui->ipEdit->setValid(false);
        QMessageBox::warning(this, windowTitle(), tr("Enter IP Without Port"), QMessageBox::Ok, QMessageBox::Ok);
        return false;
    }
    // Validate ip address
    // This is only for validation so port doesn't matter
    if (!(CService(ip.toStdString() + ":9340").IsIPv4() && CService(ip.toStdString()).IsRoutable())) {
        ui->ipEdit->setValid(false);
        QMessageBox::warning(this, windowTitle(), tr("Invalid IP Address. IPV4 ONLY"), QMessageBox::Ok, QMessageBox::Ok);
        return false;
    }
    return true;
}

void CreateNodeDialog::accept()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setFocus();
    if (!CheckAlias())
    {
        return;
    }
    if (!CheckIP())
    {
        return;
    }
    
    if(!walletmodel)
        return;
        
    if(!editMode){

		// OK Pressed
		QString label = getLabel();
		QString address = walletmodel->getAddressTableModel()->addRow(AddressTableModel::Receive, label, "", OutputType::LEGACY);
		SendAssetsRecipient recipient(address, label,(mode == 0 ? 10000 * COIN : 500 * COIN) , "");
		recipient.asset = Params().GetConsensus().subsidy_asset;
		QList<SendAssetsRecipient> recipients;
		recipients.append(recipient);

		//std::vector<std::shared_ptr<CWallet>> wallets = GetWallets();

		// Get outputs before and after transaction
		std::vector<COutput> vPossibleCoinsBefore;
		walletmodel->AvailableCoins(vPossibleCoinsBefore, Params().GetConsensus().subsidy_asset, true, nullptr, (mode == 0 ? ONLY_10000 : ONLY_500), 0, MAX_MONEY, MAX_MONEY, 0);

        if(mode == 0)
            sendDialog = new SendCollateralDialog(platformStyle, SendCollateralDialog::MASTERNODE, this);
        else
            sendDialog = new SendCollateralDialog(platformStyle, SendCollateralDialog::SYSTEMNODE, this);
    
		sendDialog->setModel(walletmodel);
		sendDialog->vsend(recipients);

		std::vector<COutput> vPossibleCoinsAfter;
		walletmodel->AvailableCoins(vPossibleCoinsAfter, Params().GetConsensus().subsidy_asset, true, nullptr, (mode == 0 ? ONLY_10000 : ONLY_500), 0, MAX_MONEY, MAX_MONEY, 0);

		for (auto& out : vPossibleCoinsAfter)
		{
			std::vector<COutput>::iterator it = std::find(vPossibleCoinsBefore.begin(), vPossibleCoinsBefore.end(), out);
			if (it == vPossibleCoinsBefore.end()) {
				// Not found so this is a new element

				COutPoint outpoint = COutPoint(out.tx->GetHash(), out.i);
				walletmodel->lockCoin(outpoint);

				// Generate a key
				CKey secret;
				secret.MakeNewKey(false);
				std::string privateKey = EncodeSecret(secret);
				std::string port = "9340";
				if (Params().NetworkIDString() == CBaseChainParams::TESTNET) {
					port = "18333";
				}
				
				if(mode == 0){
					masternodeConfig.add(getAlias().toStdString(), getIP().toStdString() + ":" + port, privateKey, out.tx->GetHash().ToString(), strprintf("%d", out.i));
					masternodeConfig.write();
				}
				else{
					systemnodeConfig.add(getAlias().toStdString(), getIP().toStdString() + ":" + port, privateKey, out.tx->GetHash().ToString(), strprintf("%d", out.i));
					systemnodeConfig.write();	
				}
			}
		}
	}
    QDialog::accept();	
}

void CreateNodeDialog::setMode(int m){

this->mode = m;
}
