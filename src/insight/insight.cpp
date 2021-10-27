// Copyright (c) 2017-2021 The Particl Core developers
// Copyright (c) 2017-2021 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <insight/insight.h>
#include <insight/addressindex.h>
#include <insight/spentindex.h>
#include <insight/timestampindex.h>
#include <validation.h>
#include <txdb.h>
#include <txmempool.h>
#include <uint256.h>
#include <script/script.h>
#include <key_io.h>

#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <util/system.h>

bool fAddressIndex = false;
bool fTimestampIndex = false;
bool fSpentIndex = false;
bool fBalancesIndex = false;

bool ExtractIndexInfo(const CScript *pScript, int &scriptType, std::vector<uint8_t> &hashBytes)
{
    CScript tmpScript;

    int witnessversion = 0;
    std::vector<unsigned char> witnessprogram;
    scriptType = ADDR_INDT_UNKNOWN;
    if (pScript->IsPayToPubkeyHash()) {
        hashBytes.assign(pScript->begin()+3, pScript->begin()+23);
        scriptType = ADDR_INDT_PUBKEY_ADDRESS;
    } else
    if (pScript->IsPayToScriptHash()) {
        hashBytes.assign(pScript->begin()+2, pScript->begin()+22);
        scriptType = ADDR_INDT_SCRIPT_ADDRESS;
    } else
    if (pScript->IsPayToWitnessScriptHash()) {
        hashBytes.assign(pScript->begin()+2, pScript->begin()+34);
        scriptType = ADDR_INDT_WITNESS_V0_SCRIPTHASH;
    } else
    if (pScript->IsWitnessProgram(witnessversion, witnessprogram)) {
        hashBytes.assign(witnessprogram.begin(), witnessprogram.begin() + witnessprogram.size());
        scriptType = ADDR_INDT_WITNESS_V0_KEYHASH;
    }

    return true;
};

bool ExtractIndexInfo(const CTxOut *out, int &scriptType, std::vector<uint8_t> &hashBytes, CAmount &nValue, CAsset &nAsset, const CScript *&pScript)
{
    if (!(pScript = &out->scriptPubKey)) {
        return error("%s: Expected script pointer.", __func__);
    }

    nValue = out->nValue;
    nAsset = out->nAsset;

    ExtractIndexInfo(pScript, scriptType, hashBytes);

    return true;
};

bool GetTimestampIndex(const unsigned int &high, const unsigned int &low, const bool fActiveOnly, std::vector<std::pair<uint256, unsigned int> > &hashes)
{
    if (!fTimestampIndex) {
        return error("Timestamp index not enabled");
    }
    if (!pblocktree->ReadTimestampIndex(high, low, fActiveOnly, hashes)) {
        return error("Unable to get hashes for timestamps");
    }

    return true;
};

bool GetSpentIndex(CSpentIndexKey &key, CSpentIndexValue &value, const CTxMemPool *pmempool)
{
    if (!fSpentIndex) {
        return false;
    }
    if (pmempool && pmempool->getSpentIndex(key, value)) {
        return true;
    }
    if (!pblocktree->ReadSpentIndex(key, value)) {
        return false;
    }

    return true;
};

bool HashOnchainActive(const uint256 &hash)
{
    CBlockIndex* pblockindex = g_chainman.BlockIndex()[hash];

    if (!::ChainActive().Contains(pblockindex)) {
        return false;
    }

    return true;
};

bool GetAddressIndex(uint160 addressHash, int type, std::string sAssetName,
                     std::vector<std::pair<CAddressIndexKey, CAmount> > &addressIndex, int start, int end)
{
    if (!fAddressIndex) {
        return error("Address index not enabled");
    }
    if (!pblocktree->ReadAddressIndex(addressHash, type, sAssetName, addressIndex, start, end)) {
        return error("Unable to get txids for address");
    }

    return true;
};

bool GetAddressUnspent(uint160 addressHash, int type, std::string sAssetName,
                       std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > &unspentOutputs)
{
    if (!fAddressIndex) {
        return error("Address index not enabled");
    }
    if (!pblocktree->ReadAddressUnspentIndex(addressHash, type, sAssetName, unspentOutputs)) {
        return error("Unable to get txids for address");
    }

    return true;
};

bool GetBlockBalances(const uint256 &block_hash, BlockBalances &balances)
{
    if (!fBalancesIndex) {
        return error("Balances index not enabled");
    }
    if (!pblocktree->ReadBlockBalancesIndex(block_hash, balances)) {
        return error("Unable to get balances for block %s", block_hash.ToString());
    }

    return true;
};

bool getAddressFromIndex(const int &type, const uint160 &hash, std::string &address)
{
    if (type == ADDR_INDT_SCRIPT_ADDRESS) {
        address = EncodeDestination(ScriptHash(uint160(hash.begin(), 20)));
    } else if (type == ADDR_INDT_PUBKEY_ADDRESS) {
        address = EncodeDestination(PKHash(uint160(hash.begin(), 20)));
    } else if (type == ADDR_INDT_WITNESS_V0_KEYHASH) {
        address = EncodeDestination(WitnessV0KeyHash(uint160(hash.begin(), 20)));
    } else if (type == ADDR_INDT_WITNESS_V0_SCRIPTHASH) {
        std::vector<unsigned char> addrdata(ParseHex(hash.ToString()));
        const CScript script(addrdata.begin(), addrdata.end());
        address = EncodeDestination(WitnessV0ScriptHash(script));
    } else {
        return false;
    }
    return true;
}

bool GetIndexKey(const CTxDestination &dest, uint160 &hashBytes, int &type) {
    if (dest.index() == DI::_PKHash) {
        const PKHash &id = std::get<PKHash>(dest);
        memcpy(hashBytes.begin(), id.begin(), 20);
        type = ADDR_INDT_PUBKEY_ADDRESS;
        return true;
    }
    if (dest.index() == DI::_ScriptHash) {
        const ScriptHash& id = std::get<ScriptHash>(dest);
        memcpy(hashBytes.begin(), id.begin(), 20);
        type = ADDR_INDT_SCRIPT_ADDRESS;
        return true;
    }
    if (dest.index() == DI::_WitnessV0KeyHash) {
        const WitnessV0KeyHash& id = std::get<WitnessV0KeyHash>(dest);
        memcpy(hashBytes.begin(), id.begin(), 20);
        type = ADDR_INDT_WITNESS_V0_KEYHASH;
        return true;
    }
    if (dest.index() == DI::_WitnessV0ScriptHash) {
        const WitnessV0ScriptHash& id = std::get<WitnessV0ScriptHash>(dest);
        memcpy(hashBytes.begin(), id.begin(), 32);
        type = ADDR_INDT_WITNESS_V0_SCRIPTHASH;
        return true;
    }
    type = ADDR_INDT_UNKNOWN;
    return false;
}

bool heightSort(std::pair<CAddressUnspentKey, CAddressUnspentValue> a,
                std::pair<CAddressUnspentKey, CAddressUnspentValue> b)
{
    return a.second.blockHeight < b.second.blockHeight;
}

bool timestampSort(std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> a,
                   std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> b)
{
    return a.second.time < b.second.time;
}
