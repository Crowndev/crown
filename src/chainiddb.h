// Copyright (c) 2017-2019 The Particl Core developers
// Copyright (c) 2009-2020 The Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_ID_DB_H
#define CROWN_ID_DB_H

#include <fs.h>
#include <lrucache.h>
#include <dbwrapper.h>
#include <primitives/transaction.h>

#include <sync.h>
#include <pubkey.h>

class CIDData
{
public:
    CChainID chainID;
    uint32_t nTime;
    uint256 reg_tx;

    CIDData(const CChainID& id, const uint256& tx, const uint32_t& nTime);
    CIDData();

    void SetNull()
    {
        chainID.SetEmpty();
        nTime=0;
        reg_tx = uint256();
    }

    SERIALIZE_METHODS(CIDData, obj) {READWRITE(obj.chainID, obj.nTime, obj.reg_tx);}
};


/** Access to the asset database */
class CIdDB : public CDBWrapper
{
public:
    explicit CIdDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    CIdDB(const CIdDB&) = delete;
    CIdDB& operator=(const CIdDB&) = delete;

    // Write to database functions
    bool WriteIDData(const CChainID &id, const CTransactionRef& tx, uint32_t& time);

    // Read from database functions
    bool ReadIDData(const std::string& alias, CChainID& id, uint256 &tx, uint32_t& time);

    // Erase from database functions
    bool EraseIDData(const std::string& alias);

    // Helper functions
    bool LoadIDs();
    bool IDDir(std::vector<CIDData>& ids, const std::string filter, const size_t count, const long start);
    bool IDDir(std::vector<CIDData>& ids);

};

//Check if Alias or Pubkey exist in database
bool ExistsID(const std::string& alias, const CPubKey& pubkey);
CChainID GetID(const std::string& alias);
CChainID GetAddressID(const CPubKey &pubKey);
/** Global variable that point to the id database (protected by cs_main) */
extern std::unique_ptr<CIdDB> piddb;

/** Global variable that point to the id Cache (protected by cs_main) */
extern CLRUCache<std::string, CIDData> *pIdCache;

void DumpIDs();

#endif // CROWN_ID_DB_H
