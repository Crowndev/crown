#ifndef ASSETMANAGERPAGE_H
#define ASSETMANAGERPAGE_H

#include <QWidget>
#include <QTimer>

class ClientModel;
class PlatformStyle;

class WalletModel;
class AssetFilterProxy;
class CoinControlFilterProxy;
class AddressFilterProxyModel;

class AssetTableModel;
class CoinControlModel;

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
    CoinControlModel *coinControlModel;

    AssetFilterProxy *assetFilter = nullptr;
    CoinControlFilterProxy *coincontrolfilter = nullptr;

    void updateAssetList();
    void update();
    void chooseAssetType(int idx);
    void chooseAssetMode(QString idx);
    void changedAssetSearch(QString search);

private:
    ClientModel *clientModel;
    WalletModel *walletModel;
    const PlatformStyle *platformStyle;
    Ui::AssetManagerPage *ui;
    QTimer* timer;
    
private Q_SLOTS:
    void on_CreateNewAsset_clicked();

};

#endif // ASSETMANAGERPAGE_H
