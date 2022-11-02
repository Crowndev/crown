#include <qt/newassetpage.h>
#include <qt/forms/ui_newassetpage.h>

NewAssetPage::NewAssetPage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewAssetPage)
{
    ui->setupUi(this);
}

NewAssetPage::~NewAssetPage()
{
    delete ui;
}

QString NewAssetPage::getinputamount(){
    return ui->inputAmount->text();
}
QString NewAssetPage::outputamount(){
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
QString NewAssetPage::getexpiry(){
    return ui->expiry->text();
}
