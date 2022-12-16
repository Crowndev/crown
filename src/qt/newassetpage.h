#ifndef NEWASSETPAGE_H
#define NEWASSETPAGE_H

#include <QDialog>
class WalletModel;
class ContractFilterProxy;
class ContractTableModel;
class PlatformStyle;

namespace Ui {
class NewAssetPage;
}

class NewAssetPage : public QDialog
{
    Q_OBJECT

public:
    explicit NewAssetPage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~NewAssetPage();

	QString getinputamount();
	QString getoutputamount();
	QString getassettype();
	QString getassetcontract();
	bool gettransferable();
	bool getconvertable();
	bool getrestricted();
	bool getlimited();
	bool getdivisible();
	QString getexpiry();
    QString getnftdata();	
    void setWalletModel(WalletModel* walletModel, ContractFilterProxy *mycontractFilter);

private:
    WalletModel* walletModel;
    Ui::NewAssetPage *ui;
    const PlatformStyle *platformStyle;

private Q_SLOTS:
    void on_Create_clicked();
    void on_addressBookButton_clicked();
};

#endif // NEWASSETPAGE_H
