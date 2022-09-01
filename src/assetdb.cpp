// Copyright (c) 2017-2020 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <auxpow.h>

#include <assetdb.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <tinyformat.h>
#include <util/system.h>
#include <validation.h>
#include <wallet/ismine.h>

static const char ASSET_FLAG = 'A';

std::unique_ptr<CAssetsDB> passetsdb;
CLRUCache<std::string, CAssetData> *passetsCache = nullptr;

CAssetData::CAssetData(const CAsset& _asset, const CTransactionRef& assetTx, const int& nOut, CCoinsViewCache& view, uint32_t _nTime)
{
    SetNull();
    asset = _asset;
    nTime = _nTime;
    txhash = assetTx->GetHash();
    //CTxOutAsset txout = assetTx->vpout[nOut];
    CTxOutAsset txout = (assetTx->nVersion >= TX_ELE_VERSION ? assetTx->vpout[nOut] : assetTx->vout[nOut]);

    issuingAddress = txout.scriptPubKey;
    if(txout.nValue)
       issuedAmount = txout.nValue;

    CAmountMap nValueOutMap;
    CAmountMap nValueInMap;

    if(!assetTx->IsCoinBase()){
		nValueInMap += view.GetValueInMap(*assetTx.get());
		nValueOutMap += assetTx->GetValueOutMap();
    }
    
    inputAssetID = nValueInMap.begin()->first.GetAssetID();
    inputAmount = nValueInMap[Params().GetConsensus().subsidy_asset];
}

CAssetData::CAssetData()
{
    this->SetNull();
}

CAssetsDB::CAssetsDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assets", nCacheSize, fMemory, fWipe) {
}

bool CAssetsDB::WriteAssetData(const CAsset &asset, const CTransactionRef& assetTx, const int& nOut, uint32_t _nTime)
{
	CCoinsViewCache view(&::ChainstateActive().CoinsTip());
    CAssetData data(asset, assetTx, nOut, view, _nTime);
    return Write(std::make_pair(ASSET_FLAG, asset.getAssetName()), data);
}

bool CAssetsDB::ReadAssetData(const std::string& strName, CAssetData &data)
{
    return Read(std::make_pair(ASSET_FLAG, strName), data);
}

bool CAssetsDB::EraseAssetData(const std::string& sAssetName)
{
    return Erase(std::make_pair(ASSET_FLAG, sAssetName));
}

void DumpAssets()
{
    int64_t n_start = GetTimeMillis();

    for(auto const& x : passetsCache->GetItemsMap()){
        CAssetData data = x.second->second;
        passetsdb->Write(std::make_pair(ASSET_FLAG, x.first), data);
    }
    LogPrint(BCLog::NET, "Finished assets dump  %dms\n", GetTimeMillis() - n_start);

}

bool CAssetsDB::LoadAssets()
{
    std::unique_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(std::make_pair(ASSET_FLAG, std::string()));

    // Load assets
    while (pcursor->Valid()) {
        std::pair<char, std::string> key;
        if (pcursor->GetKey(key) && key.first == ASSET_FLAG) {
            CAssetData data;
            if (pcursor->GetValue(data)) {
                passetsCache->Put(data.asset.getAssetName(), data);
                pcursor->Next();
            } else {
                return error("%s: failed to read asset", __func__);
            }
        } else {
            break;
        }
    }

    return true;
}

bool CAssetsDB::AssetDir(std::vector<CAssetData>& assets, const std::string filter, const size_t count, const long start)
{
    //FlushStateToDisk();
    BlockValidationState state_dummy;
    const CChainParams& chainparams = Params();
    ::ChainstateActive().FlushStateToDisk(chainparams, state_dummy, FlushStateMode::PERIODIC);

    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(ASSET_FLAG, std::string()));

    auto prefix = filter;
    bool wildcard = prefix.back() == '*';
    if (wildcard)
        prefix.pop_back();

    size_t skip = 0;
    if (start >= 0) {
        skip = start;
    }
    else {
        // compute table size for backwards offset
        long table_size = 0;
        while (pcursor->Valid()) {
            std::pair<char, std::string> key;
            if (pcursor->GetKey(key) && key.first == ASSET_FLAG) {
                if (prefix == "" ||
                    (wildcard && key.second.find(prefix) == 0) ||
                    (!wildcard && key.second == prefix)) {
                    table_size += 1;
                }
            }
            pcursor->Next();
        }
        skip = table_size + start;
        pcursor->SeekToFirst();
    }


    size_t loaded = 0;
    size_t offset = 0;

    // Load assets
    while (pcursor->Valid() && loaded < count) {
        std::pair<char, std::string> key;
        if (pcursor->GetKey(key) && key.first == ASSET_FLAG) {
            if (prefix == "" ||
                    (wildcard && key.second.find(prefix) == 0) ||
                    (!wildcard && key.second == prefix)) {
                if (offset < skip) {
                    offset += 1;
                }
                else {
                    CAssetData data;
                    if (pcursor->GetValue(data)) {
                        assets.push_back(data);
                        loaded += 1;
                    } else {
                        return error("%s: failed to read asset", __func__);
                    }
                }
            }
            pcursor->Next();
        } else {
            break;
        }
    }

    return true;
}


bool CAssetsDB::AssetDir(std::vector<CAssetData>& assets)
{
    return CAssetsDB::AssetDir(assets, "*", MAX_SIZE, 0);
}

CAsset GetAsset(const std::string& name)
{
	CAsset cCheck;
    for(auto const& x : passetsCache->GetItemsMap()){
		if(iequals(x.first , name)){
		    cCheck = x.second->second.asset;
		}
    }	
	return cCheck;
}

std::vector<CAsset> GetAllAssets(){
    std::vector<CAsset> tmp;

    for(auto const& x : passetsCache->GetItemsMap())
       tmp.push_back(x.second->second.asset);

    return tmp;
}

bool assetExists(CAsset assetToCheck){
	
    for(auto const& x : passetsCache->GetItemsMap()){
        CAsset checkasset = x.second->second.asset;
        
        if(checkasset == assetToCheck)
            return true;
        
        if(iequals(assetToCheck.getAssetName(), checkasset.getAssetName()) || iequals(assetToCheck.getAssetName(), checkasset.getShortName()))
            return true;
    }
	
	return false;
}

bool assetNameExists(std::string assetName){
	
    for(auto const& x : passetsCache->GetItemsMap()){
        CAsset checkasset = x.second->second.asset;
        
        if(iequals(assetName, checkasset.getAssetName()) || iequals(assetName, checkasset.getShortName()))
            return true;
    }
	
	return false;
}

bool isSubsidy(CAsset assetToCheck){
    if(assetToCheck == Params().GetConsensus().subsidy_asset)
	    return true;
	    
	return false;	
}

CAsset GetSubsidyAsset(){
	
	return Params().GetConsensus().subsidy_asset;
}
