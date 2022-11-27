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
    
    //qDebug() << "ADDRESS  = " << chainID;
    
    QString website_url  = getwebsiteUrl();
    QString contract_url  = getcontractURL();
    QString description  = getdescription();
    QString script  = getscript();
    qWarning() << name << " " << shortname << " " << chainID << " " << website_url << " " << contract_url << " " << description << " " << script;
    std::string strFailReason;
    QMessageBox* msgbox = new QMessageBox(this);

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid()) {
            return;
        }
        if(!walletModel->CreateContract(chainID, contract_url, website_url, description, script, name, shortname, strFailReason)){
            msgbox->setWindowTitle("Failed To Create Contract");
            msgbox->setText(QString::fromStdString(strFailReason));            
            msgbox->setStandardButtons(QMessageBox::Cancel);
            msgbox->setDefaultButton(QMessageBox::Cancel);
        }
        else {
            msgbox->setWindowTitle("New Contract Created");
            msgbox->setText("Success");
        }
        return;
    }
    if(!walletModel->CreateContract(chainID, contract_url, website_url, description, script, name, shortname, strFailReason)){
        msgbox->setWindowTitle("Failed To Create Contract");
        msgbox->setText(QString::fromStdString(strFailReason));
        msgbox->setStandardButtons(QMessageBox::Cancel);
        msgbox->setDefaultButton(QMessageBox::Cancel);
    }
    else {
        msgbox->setWindowTitle("New Contract Created");
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
    
    //if (msgbox->exec()) 
    //    accept();
}


QString NewContractPage::getName(){
    return ui->contractName->text();
}

QString NewContractPage::getSymbol(){
    return ui->contractSymbol->text();
}

QString NewContractPage::getAddress(){
	
	QString address = ui->addresscomboBox->itemData(ui->addresscomboBox->currentIndex(), AddressTableModel::AddressRole).toString();

    return address;

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
