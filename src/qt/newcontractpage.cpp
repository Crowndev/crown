#include <qt/newcontractpage.h>
#include <qt/forms/ui_newcontractpage.h>
#include <qt/addressfilterproxymodel.h>
#include <qt/walletmodel.h>
#include <QMessageBox>
#include <QDebug>
NewContractPage::NewContractPage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewContractPage)
{
    ui->setupUi(this);
}

NewContractPage::~NewContractPage()
{
    delete ui;
}

void NewContractPage::setWalletModel(WalletModel* model)
{
    if (!model)
        return;
    this->walletModel = model;

    addsfilter = new AddressFilterProxyModel(AddressTableModel::Receive, this);
    addsfilter->setSourceModel(walletModel->getAddressTableModel());
    addsfilter->setSortRole(AddressTableModel::AddressRole);
    addsfilter->sort(AddressTableModel::Address, Qt::DescendingOrder);

    ui->addresscomboBox->setModel(addsfilter);
}

void NewContractPage::on_Create_clicked()
{

    QString name  = getName();
    QString shortname  = getSymbol();
    QString chainID  =getAddress();
    QString website_url  = getwebsiteUrl();
    QString contract_url  = getcontractURL();
    QString description  = getdescription();
    QString script  = getscript();
    qWarning() << name << " " << shortname << " " << chainID << " " << website_url << " " << contract_url << " " << description << " " << script;
    std::string strFailReason;
    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid()) {
            return;
        }
        if(!walletModel->CreateContract(chainID, contract_url, website_url, description, script, name, shortname, strFailReason)){
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
    if(!walletModel->CreateContract(chainID, contract_url, website_url, description, script, name, shortname, strFailReason)){
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


QString NewContractPage::getName(){
    return ui->contractName->text();
}

QString NewContractPage::getSymbol(){
    return ui->contractSymbol->text();
}

QString NewContractPage::getAddress(){
    //return ui->addresscomboBox->currentText();
    return ui->addresslineEdit->text();
}

QString NewContractPage::getwebsiteUrl(){
    return ui->websiteURL->text();
}

QString NewContractPage::getcontractURL(){
    return ui->contractURL->text();
}

QString NewContractPage::getdescription(){
    return ui->contractDescription->toPlainText();
}

QString NewContractPage::getscript(){
    return ui->contractScript->toPlainText();
}
