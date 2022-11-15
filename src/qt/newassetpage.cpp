#include <qt/newassetpage.h>
#include <qt/forms/ui_newassetpage.h>
#include <qt/walletmodel.h>
#include <qt/contractfilterproxy.h>
#include <qt/contracttablemodel.h>

#include <QMessageBox>
#include <QDebug>
#include <QDateTime>

NewAssetPage::NewAssetPage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewAssetPage)
{
    ui->setupUi(this);
    ui->inputAmount->setValidator( new QDoubleValidator(0, 1000000, 4, this) );
    ui->outputAmount->setValidator( new QDoubleValidator(0, 1000000, 4, this) );

}

NewAssetPage::~NewAssetPage()
{
    delete ui;
}

void NewAssetPage::setWalletModel(WalletModel* model)
{
    if (!model)
        return;
    this->walletModel = model;

    contractTableModel = new ContractTableModel(walletModel);

	mycontractFilter = new ContractFilterProxy(this);
	mycontractFilter->setSourceModel(contractTableModel);
	mycontractFilter->setDynamicSortFilter(true);
	mycontractFilter->setOnlyMine(1);
	mycontractFilter->setSortRole(Qt::EditRole);
	mycontractFilter->setSortRole(ContractTableModel::NameRole);
	mycontractFilter->sort(ContractTableModel::Name, Qt::AscendingOrder);

    //ui->contractcomboBox->setModel(mycontractFilter);
    //qDebug() << "NewAssetPage, filter size " << mycontractFilter->rowCount();
    
}

void NewAssetPage::on_Create_clicked()
{

    QString inputamount  = getinputamount();
    QString outputamount  = getoutputamount();
    QString assettype  = getassettype();
    QString assetcontract  = getassetcontract();
    bool transferable = gettransferable();
    bool convertable = getconvertable();
    bool restricted = getrestricted();
    bool limited = getlimited();
    QString expiry  = getexpiry();
    std::string strFailReason;

    QString format = "dd-MM-yyyy hh:mm:ss";
    QDateTime expiryDate = QDateTime::fromString(expiry, format);
    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid()) {
            return;
        }
		if(!walletModel->CreateAsset(inputamount, outputamount, assettype, assetcontract, transferable, convertable, restricted, limited, expiryDate, strFailReason)){
			QMessageBox* msgbox = new QMessageBox(this);
			msgbox->setWindowTitle("Note");
			msgbox->setText(QString::fromStdString(strFailReason));
			msgbox->open();
		}
		else {
			QMessageBox* msgbox = new QMessageBox(this);
			msgbox->setWindowTitle("Note");
			msgbox->setText("Success");
			msgbox->open();
			close();
		}
        return;
    }

    if(!walletModel->CreateAsset(inputamount, outputamount, assettype, assetcontract, transferable, convertable, restricted, limited, expiryDate, strFailReason)){
        QMessageBox* msgbox = new QMessageBox(this);
        msgbox->setWindowTitle("Note");
        msgbox->setText(QString::fromStdString(strFailReason));
        msgbox->open();
    }
    else {
        QMessageBox* msgbox = new QMessageBox(this);
        msgbox->setWindowTitle("Note");
        msgbox->setText("Success");
        msgbox->open();
        close();
    }
}

QString NewAssetPage::getinputamount(){
    return ui->inputAmount->text();
}

QString NewAssetPage::getoutputamount(){
    return ui->outputAmount->text();
}

QString NewAssetPage::getassettype(){
    return ui->typecomboBox->currentText();
}

QString NewAssetPage::getassetcontract(){
    return ui->name->text();
}

bool NewAssetPage::gettransferable(){
    return ui->transfercheckBox->isChecked();
}

bool NewAssetPage::getconvertable(){
    return ui->convertcheckBox->isChecked();
}

bool NewAssetPage::getrestricted(){
    return ui->restrictedcheckBox->isChecked();
}

bool NewAssetPage::getlimited(){
    return ui->limitedcheckBox->isChecked();
}

QString NewAssetPage::getexpiry(){
    return ui->expiry->text();
}
