#ifndef NEWCONTRACTPAGE_H
#define NEWCONTRACTPAGE_H

#include <QDialog>
class WalletModel;
class AddressFilterProxyModel;

namespace Ui {
class NewContractPage;
}

class NewContractPage : public QDialog
{
    Q_OBJECT

public:
    explicit NewContractPage(QWidget *parent = nullptr);
    ~NewContractPage();

     QString getName();
     QString getSymbol();
     QString getAddress();
     QString getwebsiteUrl();
     QString getcontractURL();
     QString getdescription();
     QString getscript();
     void setWalletModel(WalletModel* walletModel);
     AddressFilterProxyModel *addsfilter = nullptr;

private:
    WalletModel* walletModel;
    Ui::NewContractPage *ui;
private Q_SLOTS:
    void on_Create_clicked();
};

#endif // NEWCONTRACTPAGE_H
