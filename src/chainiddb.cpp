// Copyright (c) 2017-2021 The Particl Core developers
// Copyright (c) 2009-2020 The Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainiddb.h>
#include <fs.h>
#include <auxpow.h>

#include <chainparams.h>
#include <consensus/params.h>
#include <consensus/validation.h>

#include <primitives/transaction.h>
#include <util/system.h>
#include <validation.h>

#include <leveldb/db.h>
#include <string.h>
static const char ID_FLAG = 'I';

std::unique_ptr<CIdDB> piddb;
CLRUCache<std::string, CIDData> *pIdCache = nullptr;

CIdDB::CIdDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "chainid", nCacheSize, fMemory, fWipe) {
}

CIDData::CIDData(const CChainID& id, const uint256& tx, const uint32_t& _nTime)
{
    SetNull();
    chainID = id;
    reg_tx=tx;
    nTime = _nTime;
}

CIDData::CIDData()
{
    this->SetNull();
}

bool CIdDB::WriteIDData(const CChainID &id, const CTransactionRef& tx, uint32_t& time )
{
    CIDData data(id, tx->GetHash(), time);
    return Write(std::make_pair(ID_FLAG, id.sAlias), data);
}

bool CIdDB::ReadIDData(const std::string& alias, CChainID& id, uint256 &tx, uint32_t& time)
{
    CIDData data;
    bool ret =  Read(std::make_pair(ID_FLAG, alias), data);

    if (ret) {
        id = data.chainID;
        tx = data.reg_tx;
        time = data.nTime;
    }

    return ret;
}

bool CIdDB::EraseIDData(const std::string& alias)
{
    return Erase(std::make_pair(ID_FLAG, alias));
}

bool CIdDB::LoadIDs()
{
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(ID_FLAG, std::string()));

    while (pcursor->Valid()) {
        std::pair<char, std::string> key;
        if (pcursor->GetKey(key) && key.first == ID_FLAG) {
            CIDData data;
            if (pcursor->GetValue(data)) {
                pIdCache->Put(data.chainID.sAlias, data);
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

bool CIdDB::IDDir(std::vector<CIDData>& ids, const std::string filter, const size_t count, const long start)
{
    //FlushStateToDisk();
    BlockValidationState state_dummy;
    const CChainParams& chainparams = Params();
    ::ChainstateActive().FlushStateToDisk(chainparams, state_dummy, FlushStateMode::PERIODIC);

    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(ID_FLAG, std::string()));

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
            if (pcursor->GetKey(key) && key.first == ID_FLAG) {
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
        if (pcursor->GetKey(key) && key.first == ID_FLAG) {
            if (prefix == "" ||
                    (wildcard && key.second.find(prefix) == 0) ||
                    (!wildcard && key.second == prefix)) {
                if (offset < skip) {
                    offset += 1;
                }
                else {
                    CIDData data;
                    if (pcursor->GetValue(data)) {
                        ids.push_back(data);
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

bool CIdDB::IDDir(std::vector<CIDData>& ids)
{
    return CIdDB::IDDir(ids, "*", MAX_SIZE, 0);
}

CChainID GetID(const std::string& alias)
{
	CChainID idCheck;
    for(auto const& x : pIdCache->GetItemsMap()){
		if(iequals(x.first , alias)){
		    idCheck = x.second->second.chainID;
		}
    }	
	return idCheck;
}

CChainID GetAddressID(const CPubKey &pubKey)
{
	CChainID idCheck;
    for(auto const& x : pIdCache->GetItemsMap()){
		if(x.second->second.chainID.pubKey == pubKey){
		    idCheck = x.second->second.chainID;
		}
    }	
	return idCheck;
}

void DumpIDs()
{
    int64_t n_start = GetTimeMillis();

    for(auto const& x : pIdCache->GetItemsMap()){
        CIDData data = x.second->second;
        piddb->Write(std::make_pair(ID_FLAG, x.first), data);
    }
    LogPrint(BCLog::NET, "Finished IDs dump  %dms\n", GetTimeMillis() - n_start);

}

bool ExistsID(const std::string& alias, const CPubKey& pubkey){

    for(auto const& x : pIdCache->GetItemsMap()){
		if(iequals(x.first , alias))
		    return true;

		CChainID idCheck = x.second->second.chainID;
		if(pubkey == idCheck.pubKey)
		    return true;
    }	
	return false;
}
