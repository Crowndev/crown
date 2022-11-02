#include <qt/newcontractpage.h>
#include <qt/forms/ui_newcontractpage.h>

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

QString NewContractPage::getName(){
    return ui->contractName->text();
}

QString NewContractPage::getSymbol(){
    return ui->contractSymbol->text();
}

QString NewContractPage::getAddress(){
	return ui->addresscomboBox->currentText(); 
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
