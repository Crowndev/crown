// Copyright (c) 2017-2019 The Particl Core developers
// Copyright (c) 2009-2020 The Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_CONTRACT_DB_H
#define CROWN_CONTRACT_DB_H

#include <fs.h>
#include <lrucache.h>
#include <dbwrapper.h>
#include <primitives/transaction.h>

#include <sync.h>

class CContractData
{
public:
    CContract contract;
    uint32_t nTime;
    uint256 reg_tx;

    CContractData(const CContract& contract, const uint256& tx, const uint32_t& nTime);
    CContractData();

    void SetNull()
    {
        contract.SetEmpty();
        nTime=0;
        reg_tx = uint256();
    }

    SERIALIZE_METHODS(CContractData, obj) {READWRITE(obj.contract, obj.nTime, obj.reg_tx);}
};

/** Access to the asset database */
class CContractDB : public CDBWrapper
{
public:
    explicit CContractDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    CContractDB(const CContractDB&) = delete;
    CContractDB& operator=(const CContractDB&) = delete;

    // Write to database functions
    bool WriteContractData(const CContract& contract, const CTransactionRef& tx, uint32_t& time);

    // Read from database functions
    bool ReadContractData(const std::string& name, CContract& contract, uint256 &reg_tx, uint32_t& time);

    // Erase from database functions
    bool EraseContractData(const std::string& name);

    // Helper functions
    bool LoadContracts();
    bool ContractDir(std::vector<CContractData>& contracts, const std::string filter, const size_t count, const long start);
    bool ContractDir(std::vector<CContractData>& contracts);

};

CContract GetContract(const std::string& name);
CContract GetContractByHash(uint256 & hash);
/** Global variable that point to the active assets database (protected by cs_main) */
extern std::unique_ptr<CContractDB> pcontractdb;

/** Global variable that point to the assets metadata LRU Cache (protected by cs_main) */
extern CLRUCache<std::string, CContractData> *pcontractCache;
bool ExistsContract(const std::string& name);
void DumpContracts();
std::vector<CContract> GetAllContracts();
#endif // CROWN_ID_DB_H
