// Copyright (c) 2017-2021 The Particl Core developers
// Copyright (c) 2009-2020 The Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <streams.h>
#include <contractdb.h>

#include <fs.h>
#include <clientversion.h>
#include <compat/endian.h>
#include <primitives/transaction.h>
#include <logging.h>

#include <util/system.h>

#include <leveldb/db.h>
#include <string.h>
static const char CONTRACT_FLAG = 'G';

std::unique_ptr<CContractDB> pcontractdb;
CLRUCache<std::string, CContractData> *pcontractCache = nullptr;

CContractData::CContractData(const CContract& _contract, const uint256& tx, const uint32_t& _nTime)
{
    SetNull();
    contract = _contract;
    reg_tx=tx;
    nTime = _nTime;
}

CContractData::CContractData()
{
    this->SetNull();
}

CContractDB::CContractDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "contracts", nCacheSize, fMemory, fWipe) {
}

bool CContractDB::WriteContractData(const CContract& contract, const CTransactionRef& tx, uint32_t& time)
{
    CContractData data(contract, tx->GetHash(), time);
    return Write(std::make_pair(CONTRACT_FLAG, contract.asset_name), data);
}

bool CContractDB::ReadContractData(const std::string& name, CContract& contract, uint256 &reg_tx, uint32_t& time)
{
    CContractData data;
    bool ret =  Read(std::make_pair(CONTRACT_FLAG, name), data);

    if (ret) {
        contract = data.contract;
        reg_tx = data.reg_tx;
        time = data.nTime;
    }

    return ret;
}

bool CContractDB::EraseContractData(const std::string& name)
{
    return Erase(std::make_pair(CONTRACT_FLAG, name));
}

bool CContractDB::LoadContracts()
{
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(CONTRACT_FLAG, std::string()));

    while (pcursor->Valid()) {
        std::pair<char, std::string> key;
        if (pcursor->GetKey(key) && key.first == CONTRACT_FLAG) {
            CContractData data;
            if (pcursor->GetValue(data)) {
                pcontractCache->Put(data.contract.asset_name, data);
                pcursor->Next();
            } else {
                return error("%s: failed to read ID", __func__);
            }
        } else {
            break;
        }
    }

    return true;
}

CContract GetContract(const std::string& name)
{
	CContract cCheck;
    for(auto const& x : pcontractCache->GetItemsMap()){
		if(iequals(x.first , name)){
		    cCheck = x.second->second.contract;
		}
    }	
	return cCheck;
}

CContract GetContractByHash(const uint256 & hash)
{
	CContract cCheck;
    for(auto const& x : pcontractCache->GetItemsMap()){
		if(x.second->second.contract.GetHash() == hash)
		    cCheck = x.second->second.contract;
    }	
	return cCheck;
}

void DumpContracts()
{
    int64_t n_start = GetTimeMillis();

    for(auto const& x : pcontractCache->GetItemsMap()){
        CContractData data = x.second->second;
        pcontractdb->Write(std::make_pair(CONTRACT_FLAG, x.first), data);
    }
    LogPrint(BCLog::NET, "Finished IDs dump  %dms\n", GetTimeMillis() - n_start);

}

bool ExistsContract(const std::string& name){

    for(auto const& x : pcontractCache->GetItemsMap()){
		if(iequals(x.first , name)){
		    return true;
		}
    }	
	return false;
}

std::vector<CContract> GetAllContracts(){
    std::vector<CContract> tmp;

    for(auto const& x : pcontractCache->GetItemsMap())
       tmp.push_back(x.second->second.contract);

    return tmp;
}
