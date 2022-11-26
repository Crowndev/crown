#ifndef NEWASSETPAGE_H
#define NEWASSETPAGE_H

#include <QDialog>
class WalletModel;
class ContractFilterProxy;
class ContractTableModel;

namespace Ui {
class NewAssetPage;
}

class NewAssetPage : public QDialog
{
    Q_OBJECT

public:
    explicit NewAssetPage(QWidget *parent = nullptr);
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
    void setWalletModel(WalletModel* walletModel);
    ContractFilterProxy *mycontractFilter = nullptr;
    ContractTableModel *contractTableModel;

private:
    WalletModel* walletModel;
    Ui::NewAssetPage *ui;
private Q_SLOTS:
    void on_Create_clicked();
};

#endif // NEWASSETPAGE_H
