#ifndef NEWASSETPAGE_H
#define NEWASSETPAGE_H

#include <QDialog>
class WalletModel;
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
    QString getname();
    QString getsymbol();
    QString getcontracturl();
    QString getscript();
    QString getaddress();

	bool gettransferable();
	bool getconvertable();
	bool getrestricted();
	bool getlimited();
	bool getdivisible();
	QString getexpiry();
    QString getnftdata();	
    void setWalletModel(WalletModel* walletModel);

private:
    WalletModel* walletModel;
    Ui::NewAssetPage *ui;
    const PlatformStyle *platformStyle;

private Q_SLOTS:
    void on_Create_clicked();
    void on_addressBookButton_clicked();
};

#endif // NEWASSETPAGE_H
