#include <qt/assetmanagerpage.h>
#include <qt/forms/ui_assetmanagerpage.h>

#include <qt/clientmodel.h>
#include <qt/walletmodel.h>

#include <qt/assettablemodel.h>
#include <qt/assetfilterproxy.h>
#include <qt/contracttablemodel.h>
#include <qt/contractfilterproxy.h>
#include <qt/coincontrolfilterproxy.h>
#include <qt/coincontrolmodel.h>
#include <qt/newassetpage.h>
#include <qt/newcontractpage.h>
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

    assetTableModel = new AssetTableModel(walletModel, clientModel);
    coinControlModel = new CoinControlModel(walletModel);
    contractTableModel = new ContractTableModel(walletModel);

    contractFilter = new ContractFilterProxy(this);
    contractFilter->setSourceModel(contractTableModel);
    contractFilter->setDynamicSortFilter(true);
    contractFilter->setSortRole(ContractTableModel::NameRole);
    contractFilter->sort(ContractTableModel::Name, Qt::AscendingOrder);

    assetFilter = new AssetFilterProxy(this);
    assetFilter->setSourceModel(assetTableModel);
    assetFilter->setDynamicSortFilter(true);
    assetFilter->setSortRole(AssetTableModel::NameRole);
    assetFilter->sort(AssetTableModel::Name, Qt::AscendingOrder);
        
    updateAssetList();
    updateContractList();
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

void AssetManagerPage::updateContractList()
{

    ui->contracttableView->setModel(contractFilter);
    ui->contracttableView->setAlternatingRowColors(true);
    ui->contracttableView->setSortingEnabled(true);
    ui->contracttableView->sortByColumn(1, Qt::AscendingOrder);
    ui->contracttableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    for (int i = 0; i < assetFilter->columnCount(); ++i)
        ui->contracttableView->resizeColumnToContents(i);
}

void AssetManagerPage::update()
{
    updateAssetList();
    updateContractList();
}

void AssetManagerPage::on_CreateNewContract_clicked()
{
    if(!walletModel)
        return;

    if(!walletModel->getOptionsModel())
        return;

    NewContractPage dialog(this);
    dialog.setModal(true);
    //dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setWindowTitle("Create a New Contract");
    dialog.setWalletModel(walletModel);
    dialog.show();
}


void AssetManagerPage::on_CreateNewAsset_clicked()
{
    if(!walletModel)
        return;

    if(!walletModel->getOptionsModel())
        return;

    NewAssetPage dialog(this);
    dialog.setModal(true);
    //dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setWindowTitle("Create a New Asset");
    dialog.setWalletModel(walletModel);
    dialog.show();
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

void AssetManagerPage::chooseContractType(int idx)
{
    if(!contractFilter)
        return;
    contractFilter->setOnlyMine(idx);
}

void AssetManagerPage::changedContractSearch(QString search)
{
    if(!contractFilter)
        return;
    contractFilter->setSearchString(search);
}
