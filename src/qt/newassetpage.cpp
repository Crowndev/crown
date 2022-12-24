#include <qt/newassetpage.h>
#include <qt/forms/ui_newassetpage.h>
#include <qt/walletmodel.h>
#include <qt/addressbookpage.h>
#include <qt/guiutil.h>
#include <qt/platformstyle.h>

#include <QMessageBox>
#include <QDebug>
#include <QDateTime>

NewAssetPage::NewAssetPage(const PlatformStyle *_platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewAssetPage),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);

    ui->addressBookButton->setIcon(platformStyle->SingleColorIcon(":/icons/address-book"));
    ui->pasteButton->setIcon(platformStyle->SingleColorIcon(":/icons/editpaste"));
    ui->deleteButton->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
     // normal crown address field
    GUIUtil::setupAddressWidget(ui->payTo, this);


    ui->inputAmount->setValidator( new QDoubleValidator(0, 1000000, 4, this) );
    ui->outputAmount->setValidator( new QDoubleValidator(0, 1000000, 4, this) );

    ui->inputAmount->setStatusTip(tr("Crown to use to create Asset"));
    ui->inputAmount->setToolTip(ui->inputAmount->statusTip());

    ui->outputAmount->setStatusTip(tr("Asset amount to create"));
    ui->outputAmount->setToolTip(ui->outputAmount->statusTip());

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

void NewAssetPage::on_addressBookButton_clicked()
{
    if(!walletModel)
        return;
    AddressBookPage dlg(platformStyle, AddressBookPage::ForSelection, AddressBookPage::ReceivingTab, this);
    dlg.setModel(walletModel->getAddressTableModel(), false);
    if(dlg.exec())
    {
        ui->payTo->setText(dlg.getReturnValue());
    }
}

void NewAssetPage::setWalletModel(WalletModel* model)
{
    if (!model)
        return;
    this->walletModel = model;

}

void NewAssetPage::on_Create_clicked()
{

    QString inputamount  = getinputamount();
    QString outputamount  = getoutputamount();
    QString assettype  = getassettype();

    QString name  = getname();
    QString shortname  = getsymbol();
    QString contracturl  = getcontracturl();
    QString scriptcode  = getscript();
    QString address = getaddress();

    std::vector<unsigned char> scriptcodedata(ParseHex(scriptcode.toStdString()));
    CScript script(scriptcodedata.begin(), scriptcodedata.end());

    std::string mdata  = getnftdata().toStdString();
    bool transferable = gettransferable();
    bool convertable = getconvertable();
    bool restricted = getrestricted();
    bool limited = getlimited();
    bool divisible = getdivisible();
    QString expiry  = getexpiry();
    std::string strFailReason;

    CTxData rdata;
    rdata.vData.assign(mdata.begin(), mdata.end());

    QString format = "dd-MM-yyyy hh:mm:ss";
    QDateTime expiryDate = QDateTime::fromString(expiry, format);
    QMessageBox* msgbox = new QMessageBox(this);

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid()) {
            return;
        }
        if(!walletModel->CreateAsset(name, shortname, address, contracturl, script, inputamount, outputamount, assettype, transferable, convertable, restricted, limited, divisible, expiryDate, strFailReason, rdata)){
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

    if(!walletModel->CreateAsset(name, shortname, address, contracturl, script, inputamount, outputamount, assettype, transferable, convertable, restricted, limited, divisible, expiryDate, strFailReason, rdata)){
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

QString NewAssetPage::getname(){
    return ui->name->text();
}

QString NewAssetPage::getsymbol(){
    return ui->symbol->text();
}

QString NewAssetPage::getcontracturl(){
    return ui->contracturl->text();
}

QString NewAssetPage::getscript(){
    return ui->scriptcode->toPlainText();
}

QString NewAssetPage::getnftdata(){
    return ui->nftTextEdit->toPlainText();
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

QString NewAssetPage::getaddress(){
    return ui->payTo->text();
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
