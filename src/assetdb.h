// Copyright (c) 2017-2020 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_ASSETDB_H
#define CROWN_ASSETDB_H

#include <fs.h>
//#include <primitives/confidential.h>
#include <primitives/block.h>
#include <lrucache.h>
#include <dbwrapper.h>

class CAssetData
{
public:
    CAsset asset;
    CScript issuingAddress;
    CAmount inputAmount;
    CAmount issuedAmount;
    uint256 txhash;
    uint32_t nTime;

    explicit CAssetData(const CAsset& asset, const CTransactionRef& assetTx, const int& nOut, uint32_t nTime);
    CAssetData();

    void SetNull()
    {
        asset.SetNull();
        issuingAddress.clear();
        inputAmount =0;
        issuedAmount =0;
        nTime=0;
    }

    SERIALIZE_METHODS(CAssetData, obj) {READWRITE(obj.asset, obj.issuingAddress, obj.inputAmount, obj.issuedAmount, obj.nTime, obj.txhash);}
};


/** Access to the asset database */
class CAssetsDB : public CDBWrapper
{
public:
    explicit CAssetsDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    CAssetsDB(const CAssetsDB&) = delete;
    CAssetsDB& operator=(const CAssetsDB&) = delete;

    // Write to database functions
    bool WriteAssetData(const CAsset& asset, const CTransactionRef& assetTx, const int& nOut, uint32_t _nTime);

    // Read from database functions
    bool ReadAssetData(const std::string& strName, CAssetData &data);

    // Erase from database functions
    bool EraseAssetData(const std::string& sAssetName);

    // Helper functions
    bool LoadAssets();
    bool AssetDir(std::vector<CAssetData>& assets, const std::string filter, const size_t count, const long start);
    bool AssetDir(std::vector<CAssetData>& assets);

};

/** Global variable that point to the active assets database (protected by cs_main) */
extern std::unique_ptr<CAssetsDB> passetsdb;

/** Global variable that point to the assets metadata LRU Cache (protected by cs_main) */
extern CLRUCache<std::string, CAssetData> *passetsCache;

void DumpAssets();

CAsset GetAsset(const std::string& name);
CAssetData GetAssetData(const std::string& name);
std::vector<CAsset> GetAllAssets();
bool assetExists(CAsset assetToCheck, uint256 &txhash);
bool assetNameExists(std::string assetName);
bool isSubsidy(CAsset assetToCheck);
CAsset GetSubsidyAsset();
CAsset GetDevAsset();
#endif //CROWN_ASSETDB_H
