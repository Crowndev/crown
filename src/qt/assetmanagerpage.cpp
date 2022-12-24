#include <qt/assetmanagerpage.h>
#include <qt/forms/ui_assetmanagerpage.h>

#include <qt/clientmodel.h>
#include <qt/walletmodel.h>

#include <qt/assettablemodel.h>
#include <qt/assetfilterproxy.h>
#include <qt/coincontrolfilterproxy.h>
#include <qt/coincontrolmodel.h>
#include <qt/newassetpage.h>
#include <qt/addressfilterproxymodel.h>
#include <QMessageBox>
#include <QDebug>

AssetManagerPage::AssetManagerPage(const PlatformStyle *_platformStyle) :
    clientModel(nullptr),
    walletModel(nullptr),
    platformStyle(_platformStyle),
    ui(new Ui::AssetManagerPage)
{
    ui->setupUi(this);
}

AssetManagerPage::~AssetManagerPage()
{
    delete ui;
}

void AssetManagerPage::setWalletModel(WalletModel *_walletModel, ClientModel *_clientModel)
{
    if (!_walletModel || !_clientModel)
        return;

    clientModel = _clientModel;

    walletModel = _walletModel;

    assetFilter = new AssetFilterProxy(this);
    assetFilter->setSourceModel(walletModel->getAssetTableModel());
    assetFilter->setDynamicSortFilter(true);
    assetFilter->setSortRole(AssetTableModel::NameRole);
    assetFilter->sort(AssetTableModel::Name, Qt::AscendingOrder);

    connect(clientModel, &ClientModel::numBlocksChanged, this, &AssetManagerPage::update);
    update();

}

void AssetManagerPage::updateAssetList()
{
    ui->assettableView->setModel(assetFilter);
    ui->assettableView->setAlternatingRowColors(true);
    ui->assettableView->setSortingEnabled(true);
    ui->assettableView->sortByColumn(1, Qt::AscendingOrder);
    ui->assettableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    for (int i = 0; i < assetFilter->columnCount(); ++i)
        ui->assettableView->resizeColumnToContents(i);

   //ui->assettableView->show();
}

void AssetManagerPage::update()
{
    updateAssetList();
}

void AssetManagerPage::on_CreateNewAsset_clicked()
{
    if(!walletModel)
        return;

    if(!walletModel->getOptionsModel())
        return;

    NewAssetPage dialog(platformStyle, this);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setWindowTitle("Create a New Asset");
    dialog.setWalletModel(walletModel);
    if(dialog.exec())
        update();
}

void AssetManagerPage::chooseAssetType(int idx)
{
    if(!assetFilter)
        return;
    assetFilter->setOnlyMine(idx);
}

void AssetManagerPage::chooseAssetMode(QString idx)
{
    if(!assetFilter)
        return;
    assetFilter->setType(idx);
}

void AssetManagerPage::changedAssetSearch(QString search)
{
    if(!assetFilter)
        return;
    assetFilter->setSearchString(search);
}
