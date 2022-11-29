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

    ui->inputAmount->setStatusTip(tr("Crown to use to create Asset"));
    ui->inputAmount->setToolTip(ui->inputAmount->statusTip());

    ui->outputAmount->setStatusTip(tr("Asset amount to create"));
    ui->outputAmount->setToolTip(ui->outputAmount->statusTip());

    ui->contractcomboBox->setStatusTip(tr("Select Contract to use"));
    ui->contractcomboBox->setToolTip(ui->contractcomboBox->statusTip());
    
    ui->typecomboBox->setStatusTip(tr("Select Asset Type"));
    ui->typecomboBox->setToolTip(ui->typecomboBox->statusTip());

    ui->expiry->setStatusTip(tr("Set Expiry Date"));
    ui->expiry->setToolTip(ui->expiry->statusTip());        

    ui->convertcheckBox->setStatusTip(tr("Can the asset be later liquidated"));
    ui->convertcheckBox->setToolTip(ui->convertcheckBox->statusTip()); 

    ui->transfercheckBox->setStatusTip(tr("Can the asset be sent"));
    ui->transfercheckBox->setToolTip(ui->transfercheckBox->statusTip()); 

    ui->restrictedcheckBox->setStatusTip(tr("Restrict sending to Issuer"));
    ui->restrictedcheckBox->setToolTip(ui->restrictedcheckBox->statusTip()); 

    ui->limitedcheckBox->setStatusTip(tr("Prevent conversion from other assets"));
    ui->limitedcheckBox->setToolTip(ui->limitedcheckBox->statusTip()); 
    
    ui->divisiblecheckBox->setStatusTip(tr("Allow/Disallow division into smaller units"));
    ui->divisiblecheckBox->setToolTip(ui->divisiblecheckBox->statusTip());     
}

NewAssetPage::~NewAssetPage()
{
    delete ui;
}

void NewAssetPage::setWalletModel(WalletModel* model, ContractFilterProxy *mycontractFilter)
{
    if (!model)
        return;
    this->walletModel = model;

    ui->contractcomboBox->setModel(mycontractFilter);

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
    bool divisible = getdivisible();
    QString expiry  = getexpiry();
    std::string strFailReason;

    QString format = "dd-MM-yyyy hh:mm:ss";
    QDateTime expiryDate = QDateTime::fromString(expiry, format);
    QMessageBox* msgbox = new QMessageBox(this);

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid()) {
            return;
        }
        if(!walletModel->CreateAsset(inputamount, outputamount, assettype, assetcontract, transferable, convertable, restricted, limited, divisible, expiryDate, strFailReason)){
            msgbox->setWindowTitle("Failed To Create Asset");
            msgbox->setText(QString::fromStdString(strFailReason));
            msgbox->setStandardButtons(QMessageBox::Cancel);
            msgbox->setDefaultButton(QMessageBox::Cancel);
        }
        else {
            msgbox->setWindowTitle("New Asset Created");
            msgbox->setText("Success");
        }
        return;
    }

    if(!walletModel->CreateAsset(inputamount, outputamount, assettype, assetcontract, transferable, convertable, restricted, limited, divisible, expiryDate, strFailReason)){
        msgbox->setWindowTitle("Failed To Create Asset");
        msgbox->setText(QString::fromStdString(strFailReason));
        msgbox->setStandardButtons(QMessageBox::Cancel);
        msgbox->setDefaultButton(QMessageBox::Cancel);
    }
    else {
        msgbox->setWindowTitle("New Asset Created");
        msgbox->setText("Success");
    }

    int ret = msgbox->exec();

    switch (ret) {
      case QMessageBox::Cancel:
          msgbox->hide();
          break;
      default:
          accept();
          break;
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
    return ui->contractcomboBox->currentText();
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

bool NewAssetPage::getdivisible(){
    return ui->divisiblecheckBox->isChecked();
}

QString NewAssetPage::getexpiry(){
    return ui->expiry->text();
}
