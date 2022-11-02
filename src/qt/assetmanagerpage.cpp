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

AssetManagerPage::AssetManagerPage(const PlatformStyle *_platformStyle) :
    clientModel(nullptr),
    walletModel(nullptr),
    platformStyle(_platformStyle),
    ui(new Ui::AssetManagerPage)
{
    ui->setupUi(this);
    connect(clientModel, &ClientModel::numBlocksChanged, this, &AssetManagerPage::update);
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

   //ui->contracttableView->show();
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
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setWindowTitle("Create a New Contract");

    if (dialog.exec())
    {
        QString name  = dialog.getName();
        QString shortname  = dialog.getSymbol();
        QString chainID  = dialog.getAddress();
        QString website_url  = dialog.getwebsiteUrl();
        QString contract_url  = dialog.getcontractURL();
        QString description  = dialog.getdescription();
        QString script  = dialog.getscript();
        std::string strFailReason;

        if(!walletModel->CreateContract(chainID, contract_url, website_url, description, script, name, shortname, strFailReason)){
            //QMetaObject::invokeMethod(m_contractPane, "setresponse",Q_ARG(QVariant, QString::fromStdString(strFailReason)));
            return;
        }

    } else {
        // Cancel Pressed
    }
}


void AssetManagerPage::on_CreateNewAsset_clicked()
{
    if(!walletModel)
        return;

    if(!walletModel->getOptionsModel())
        return;

    NewAssetPage dialog(this);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setWindowTitle("Create a New Asset");

    if (dialog.exec())
    {
        QString inputamount  = dialog.getinputamount();
        QString outputamount  = dialog.outputamount();
        QString assettype  = dialog.getassettype();
        QString assetcontract  = dialog.getassetcontract();
        bool transferable = dialog.gettransferable();
        bool convertable = dialog.getconvertable();
        bool restricted = dialog.getrestricted();
        bool limited = dialog.getlimited();
        QString expiry  = dialog.getexpiry();
        std::string strFailReason;

        QString format = "dd-MM-yyyy hh:mm:ss";
        QDateTime expiryDate = QDateTime::fromString(expiry, format);

        if(!walletModel->CreateAsset(inputamount, outputamount, assettype, assetcontract, transferable, convertable, restricted, limited, expiryDate, strFailReason)){
            //QMetaObject::invokeMethod(m_assetPane, "setresponse",Q_ARG(QVariant, QString::fromStdString(strFailReason)));
            return;
        }
    } else {
        // Cancel Pressed
    }
}
