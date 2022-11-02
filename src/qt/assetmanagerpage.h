#ifndef ASSETMANAGERPAGE_H
#define ASSETMANAGERPAGE_H

#include <QWidget>

class ClientModel;
class PlatformStyle;

class WalletModel;
class AssetFilterProxy;
class ContractFilterProxy;
class CoinControlFilterProxy;

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
private:
    ClientModel *clientModel;
    WalletModel *walletModel;
    const PlatformStyle *platformStyle;
    Ui::AssetManagerPage *ui;
private Q_SLOTS:
    void on_CreateNewContract_clicked();
    void on_CreateNewAsset_clicked();
    
};

#endif // ASSETMANAGERPAGE_H
