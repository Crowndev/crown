#ifndef ASSETMANAGERPAGE_H
#define ASSETMANAGERPAGE_H

#include <QWidget>
#include <QTimer>

class ClientModel;
class PlatformStyle;

class WalletModel;
class AssetFilterProxy;
class ContractFilterProxy;
class CoinControlFilterProxy;
class AddressFilterProxyModel;

class AssetTableModel;
class CoinControlModel;
class ContractTableModel;

namespace Ui {
class AssetManagerPage;
}

class AssetManagerPage : public QWidget
{
    Q_OBJECT

public:
    explicit AssetManagerPage(const PlatformStyle *platformStyle);
    ~AssetManagerPage();

    void setWalletModel(WalletModel *model, ClientModel *clientModel);

    AssetTableModel *assetTableModel;
    ContractTableModel *contractTableModel;
    CoinControlModel *coinControlModel;

    AssetFilterProxy *assetFilter = nullptr;
    ContractFilterProxy *contractFilter = nullptr;
    CoinControlFilterProxy *coincontrolfilter = nullptr;

    void updateAssetList();
    void updateContractList();
    void update();
    void chooseAssetType(int idx);
    void chooseAssetMode(QString idx);
    void changedAssetSearch(QString search);
    void chooseContractType(int idx);
    void changedContractSearch(QString search);

private:
    ClientModel *clientModel;
    WalletModel *walletModel;
    const PlatformStyle *platformStyle;
    Ui::AssetManagerPage *ui;
    QTimer* timer;
    
private Q_SLOTS:
    void on_CreateNewContract_clicked();
    void on_CreateNewAsset_clicked();

};

#endif // ASSETMANAGERPAGE_H
