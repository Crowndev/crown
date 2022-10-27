// Copyright (c) 2018-2021 The Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <rpc/util.h>
#include <rpc/blockchain.h>

#include <util/strencodings.h>
#include <insight/insight.h>
#include <index/txindex.h>
#include <validation.h>
#include <txmempool.h>
#include <key_io.h>
#include <miner.h>

#include <core_io.h>
#include <node/context.h>
#include <script/standard.h>
#include <shutdown.h>

#include <univalue.h>

// Avoid initialization-order-fiasco
#define _UNIX_EPOCH_TIME "UNIX epoch time"

bool getAddressesFromParams(const UniValue& params, std::vector<std::pair<uint160, int> > &addresses)
{
    if (params[0].isStr()) {
        CTxDestination dest = DecodeDestination(params[0].get_str());
        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Invalid Crown address 1 %s", params[0].get_str()));
        }

        uint160 hashBytes;
        int type = 0;
        if (!GetIndexKey(dest, hashBytes, type)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Invalid address 1 type = %d", type ));
        }
        addresses.push_back(std::make_pair(hashBytes, type));
    } else if (params[0].isObject()) {

        UniValue addressValues = find_value(params[0].get_obj(), "addresses");
        if (!addressValues.isArray()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Addresses is expected to be an array");
        }

        std::vector<UniValue> values = addressValues.getValues();
        for (std::vector<UniValue>::iterator it = values.begin(); it != values.end(); ++it) {

            CTxDestination dest = DecodeDestination(it->get_str());
            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Invalid Crown address 2 %s", params[0].get_str()));
            }

            uint160 hashBytes;
            int type = 0;
            if (!GetIndexKey(dest, hashBytes, type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address 2");
            }
            addresses.push_back(std::make_pair(hashBytes, type));
        }
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address 3");
    }

    return true;
}

static RPCHelpMan getaddressmempool()
{
    return RPCHelpMan{"getaddressmempool",
                "\nReturns all mempool deltas for an address (requires addressindex to be enabled).\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The base58check encoded address."},
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "", {
                        {RPCResult::Type::OBJ, "", "", {
                            {RPCResult::Type::STR, "address", "The base58check encoded address"},
                            {RPCResult::Type::STR_HEX, "txid", "The related txids"},
                            {RPCResult::Type::STR_HEX, "index", "The related input or output index"},
                            {RPCResult::Type::NUM, "satoshis", "The difference of satoshis"},
                            {RPCResult::Type::NUM_TIME, "timestamp", "The time the transaction entered the mempool (seconds)"},
                            {RPCResult::Type::STR_HEX, "prevtxid", "The previous txid (if spending)"},
                            {RPCResult::Type::NUM, "prevout", "The previous transaction output index (if spending)"},
                        }}
                    }
                },
                RPCExamples{
            HelpExampleCli("getaddressmempool", "'{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}'") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("getaddressmempool", "{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const CTxMemPool& mempool = EnsureMemPool(request.context);

    if (!fAddressIndex) {
        throw JSONRPCError(RPC_MISC_ERROR, "Address index is not enabled.");
    }

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address 4");
    }

    std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> > indexes;
    if (!mempool.getAddressIndex(addresses, indexes)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
    }

    std::sort(indexes.begin(), indexes.end(), timestampSort);

    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> >::iterator it = indexes.begin();
         it != indexes.end(); it++) {

        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.addressBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        UniValue delta(UniValue::VOBJ);
        delta.pushKV("address", address);
        delta.pushKV("txid", it->first.txhash.GetHex());
        delta.pushKV("index", (int)it->first.index);
        delta.pushKV("satoshis", it->second.amount);
        delta.pushKV("timestamp", (int64_t)it->second.time.count());
        if (it->second.amount < 0) {
            delta.pushKV("prevtxid", it->second.prevhash.GetHex());
            delta.pushKV("prevout", (int)it->second.prevout);
        }
        result.push_back(delta);
    }

    return result;
},
    };
}

static RPCHelpMan getaddressutxos()
{
return RPCHelpMan{"getaddressutxos",
                "\nReturns all unspent outputs for an address (requires addressindex to be enabled).\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The base58check encoded address."},
                    {"chainInfo", RPCArg::Type::BOOL, /* default */ "false", "Include chain info in results, only applies if start and end specified."},
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "", {
                        {RPCResult::Type::OBJ, "", "", {
                            {RPCResult::Type::STR, "address", "The base58check encoded address"},
                            {RPCResult::Type::STR_HEX, "txid", "The output txid"},
                            {RPCResult::Type::NUM, "height", "The block height"},
                            {RPCResult::Type::NUM, "outputIndex", "The output index"},
                            {RPCResult::Type::STR_HEX, "script", "The script hex encoded"},
                            {RPCResult::Type::NUM, "satoshis", "The number of satoshis of the output"},
                        }}
                    }
                },
                RPCExamples{
            HelpExampleCli("getaddressutxos", "'{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}'") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("getaddressutxos", "{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (!fAddressIndex) {
        throw JSONRPCError(RPC_MISC_ERROR, "Address index is not enabled.");
    }

    bool includeChainInfo = false;
    if (request.params[0].isObject()) {
        UniValue chainInfo = find_value(request.params[0].get_obj(), "chainInfo");
        if (chainInfo.isBool()) {
            includeChainInfo = chainInfo.get_bool();
        }
    }

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address 5");
    }

    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
    CAsset asset;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (!GetAddressUnspent((*it).first, (*it).second, asset, unspentOutputs)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
        }
    }

    std::sort(unspentOutputs.begin(), unspentOutputs.end(), heightSort);

    UniValue utxos(UniValue::VARR);

    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++) {
        UniValue output(UniValue::VOBJ);
        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.hashBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        output.pushKV("address", address);
        output.pushKV("txid", it->first.txhash.GetHex());
        output.pushKV("outputIndex", (int)it->first.index);
        output.pushKV("script", HexStr(it->second.script));
        output.pushKV("satoshis", it->second.satoshis);
        UniValue p(UniValue::VOBJ);
        AssetToUniv(it->second.asset, p);
        output.pushKV("asset", p);
                    
        output.pushKV("height", it->second.blockHeight);
        utxos.push_back(output);
    }

    if (includeChainInfo) {
        UniValue result(UniValue::VOBJ);
        result.pushKV("utxos", utxos);

        LOCK(cs_main);
        result.pushKV("hash", ::ChainActive().Tip()->GetBlockHash().GetHex());
        result.pushKV("height", (int)::ChainActive().Height());
        return result;
    } else {
        return utxos;
    }
},
    };
}

static RPCHelpMan getaddressdeltas()
{
    return RPCHelpMan{"getaddressdeltas",
                "\nReturns all changes for an address (requires addressindex to be enabled).\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The base58check encoded address."},
                    {"start", RPCArg::Type::NUM, /* default */ "0", "The start block height."},
                    {"end", RPCArg::Type::NUM, /* default */ "0", "The end block height."},
                    {"chainInfo", RPCArg::Type::BOOL, /* default */ "false", "Include chain info in results, only applies if start and end specified."},
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "", {
                        {RPCResult::Type::OBJ, "", "", {
                            {RPCResult::Type::NUM, "satoshis", "The difference of satoshis"},
                            {RPCResult::Type::STR_HEX, "txid", "The related txid"},
                            {RPCResult::Type::NUM, "index", "The block height"},
                            {RPCResult::Type::STR, "address", "The base58check encoded address"},
                        }}
                    }
                },
                RPCExamples{
            HelpExampleCli("getaddressdeltas", "'{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}'") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("getaddressdeltas", "{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (!fAddressIndex) {
        throw JSONRPCError(RPC_MISC_ERROR, "Address index is not enabled.");
    }

    UniValue startValue = find_value(request.params[0].get_obj(), "start");
    UniValue endValue = find_value(request.params[0].get_obj(), "end");

    UniValue chainInfo = find_value(request.params[0].get_obj(), "chainInfo");
    bool includeChainInfo = false;
    if (chainInfo.isBool()) {
        includeChainInfo = chainInfo.get_bool();
    }

    int start = 0;
    int end = 0;

    if (startValue.isNum() && endValue.isNum()) {
        start = startValue.get_int();
        end = endValue.get_int();
        if (start <= 0 || end <= 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Start and end is expected to be greater than zero");
        }
        if (end < start) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "End value is expected to be greater than start");
        }
    }

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address 6");
    }

    std::vector<std::pair<CAddressIndexKey, CAmountMap> > addressIndex;
    CAsset asset;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (start > 0 && end > 0) {
            if (!GetAddressIndex((*it).first, (*it).second, asset, addressIndex, start, end)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        } else {
            if (!GetAddressIndex((*it).first, (*it).second, asset, addressIndex)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        }
    }

    UniValue deltas(UniValue::VARR);

    for (std::vector<std::pair<CAddressIndexKey, CAmountMap> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.hashBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        UniValue delta(UniValue::VOBJ);
        UniValue p(UniValue::VOBJ);
        AmountMapToUniv(it->second, p);
        delta.pushKV("satoshis", p);
        delta.pushKV("txid", it->first.txhash.GetHex());
        delta.pushKV("index", (int)it->first.index);
        delta.pushKV("blockindex", (int)it->first.txindex);
        delta.pushKV("height", it->first.blockHeight);
        delta.pushKV("address", address);
        deltas.push_back(delta);
    }

    UniValue result(UniValue::VOBJ);

    if (includeChainInfo && start > 0 && end > 0) {
        LOCK(cs_main);

        if (start > ::ChainActive().Height() || end > ::ChainActive().Height()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Start or end is outside chain range");
        }

        CBlockIndex* startIndex = ::ChainActive()[start];
        CBlockIndex* endIndex = ::ChainActive()[end];

        UniValue startInfo(UniValue::VOBJ);
        UniValue endInfo(UniValue::VOBJ);

        startInfo.pushKV("hash", startIndex->GetBlockHash().GetHex());
        startInfo.pushKV("height", start);

        endInfo.pushKV("hash", endIndex->GetBlockHash().GetHex());
        endInfo.pushKV("height", end);

        result.pushKV("deltas", deltas);
        result.pushKV("start", startInfo);
        result.pushKV("end", endInfo);

        return result;
    } else {
        return deltas;
    }
},
    };
}

static RPCHelpMan getaddressbalance()
{
    return RPCHelpMan{"getaddressbalance",
                "\nReturns the balance for an address(es) (requires addressindex to be enabled).\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The crown address "},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "", {
                        {RPCResult::Type::STR_AMOUNT, "balance", "The current balance in satoshis"},
                        {RPCResult::Type::STR_AMOUNT, "received", "The total number of satoshis received (including change)"},
                    }
                },
                RPCExamples{
            HelpExampleCli("getaddressbalance", "'{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}'") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("getaddressbalance", "{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (!fAddressIndex) {
        throw JSONRPCError(RPC_MISC_ERROR, "Address index is not enabled.");
    }

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address 7");
    }

    std::vector<std::pair<CAddressIndexKey, CAmountMap> > addressIndex;
    std::string address= request.params[0].get_str();
    CAsset asset;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (!GetAddressIndex((*it).first, (*it).second, asset, addressIndex)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
        }
    }

    CAmountMap balance;
    CAmountMap received;

    for (std::vector<std::pair<CAddressIndexKey, CAmountMap> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        if (it->second > CAmountMap()) {
            received += it->second;
        }
        balance += it->second;
    }

    UniValue result(UniValue::VOBJ);
    UniValue bal(UniValue::VOBJ);
    UniValue rec(UniValue::VOBJ);
    AmountMapToUniv(balance, bal);
    AmountMapToUniv(received, rec);
    result.pushKV("balance", bal);
    result.pushKV("received", rec);

    return result;
},
    };
}

static RPCHelpMan getaddresstxids()
{
    return RPCHelpMan{"getaddresstxids",
                "\nReturns the txids for an address(es) (requires addressindex to be enabled).\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The base58check encoded address."},
                    {"start", RPCArg::Type::NUM, /* default */ "0", "The start block height."},
                    {"end", RPCArg::Type::NUM, /* default */ "0", "The end block height."},
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "", {
                        {RPCResult::Type::STR_HEX, "transactionid", "The transaction txid"},
                    }
                },
                RPCExamples{
            HelpExampleCli("getaddresstxids", "'{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}'") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("getaddresstxids", "{\"addresses\": [\"Pb7FLL3DyaAVP2eGfRiEkj4U8ZJ3RHLY9g\"]}")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (!fAddressIndex) {
        throw JSONRPCError(RPC_MISC_ERROR, "Address index is not enabled.");
    }

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address 8");
    }

    int start = 0;
    int end = 0;
    if (request.params[0].isObject()) {
        UniValue startValue = find_value(request.params[0].get_obj(), "start");
        UniValue endValue = find_value(request.params[0].get_obj(), "end");
        if (startValue.isNum() && endValue.isNum()) {
            start = startValue.get_int();
            end = endValue.get_int();
        }
    }

    std::vector<std::pair<CAddressIndexKey, CAmountMap> > addressIndex;
    CAsset asset;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (start > 0 && end > 0) {
            if (!GetAddressIndex((*it).first, (*it).second, asset,  addressIndex, start, end)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        } else {
            if (!GetAddressIndex((*it).first, (*it).second, asset, addressIndex)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        }
    }

    std::set<std::pair<int, std::string> > txids;
    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CAddressIndexKey, CAmountMap> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        int height = it->first.blockHeight;
        std::string txid = it->first.txhash.GetHex();

        if (addresses.size() > 1) {
            txids.insert(std::make_pair(height, txid));
        } else {
            if (txids.insert(std::make_pair(height, txid)).second) {
                result.push_back(txid);
            }
        }
    }

    if (addresses.size() > 1) {
        for (std::set<std::pair<int, std::string> >::const_iterator it=txids.begin(); it!=txids.end(); it++) {
            result.push_back(it->second);
        }
    }

    return result;
},
    };
}

static RPCHelpMan getspentinfo()
{
    return RPCHelpMan{"getspentinfo",
                "\nReturns the txid and index where an output is spent.\n",
                {
                    {"inputs", RPCArg::Type::OBJ, RPCArg::Optional::NO, "",
                        {
                            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex string of the txid."},
                            {"index", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number."},
                        },
                    },
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "", {
                        {RPCResult::Type::STR_HEX, "txid", "The transaction id"},
                        {RPCResult::Type::NUM, "index", "The spending input index"},
                        {RPCResult::Type::NUM, "height", "The height of the block containing the spending tx"},
                    }
                },
                RPCExamples{
            HelpExampleCli("getspentinfo", "'{\"txid\": \"0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9\", \"index\": 0}'") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("getspentinfo", "{\"txid\": \"0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9\", \"index\": 0}")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const CTxMemPool& mempool = EnsureMemPool(request.context);

    UniValue txidValue = find_value(request.params[0].get_obj(), "txid");
    UniValue indexValue = find_value(request.params[0].get_obj(), "index");

    if (!txidValue.isStr() || !indexValue.isNum()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid txid or index");
    }

    uint256 txid = ParseHashV(txidValue, "txid");
    int outputIndex = indexValue.get_int();

    CSpentIndexKey key(txid, outputIndex);
    CSpentIndexValue value;

    if (!GetSpentIndex(key, value, &mempool)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unable to get spent info");
    }

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("txid", value.txid.GetHex());
    obj.pushKV("index", (int)value.inputIndex);
    obj.pushKV("height", value.blockHeight);

    return obj;
},
    };
}

static void AddAddress(CScript *script, UniValue &uv)
{
    if (script->IsPayToScriptHash()) {
        std::vector<unsigned char> hashBytes(script->begin()+2, script->begin()+22);
        uv.pushKV("address", EncodeDestination(ScriptHash(uint160(hashBytes))));
    } else
    if (script->IsPayToPubkeyHash()) {
        std::vector<unsigned char> hashBytes(script->begin()+3, script->begin()+23);
        uv.pushKV("address", EncodeDestination(PKHash(uint160(hashBytes))));
    }
}

static UniValue blockToDeltasJSON(const CBlock& block, const CBlockIndex* blockindex, const CTxMemPool *pmempool)
{
    UniValue result(UniValue::VOBJ);
    result.pushKV("hash", block.GetHash().GetHex());
    int confirmations = -1;
    // Only report confirmations if the block is on the main chain
    if (::ChainActive().Contains(blockindex)) {
        confirmations = ::ChainActive().Height() - blockindex->nHeight + 1;
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block is an orphan");
    }
    result.pushKV("confirmations", confirmations);
    result.pushKV("size", (int)::GetSerializeSize(block, PROTOCOL_VERSION));
    result.pushKV("height", blockindex->nHeight);
    result.pushKV("version", block.nVersion.GetFullVersion());
    result.pushKV("merkleroot", block.hashMerkleRoot.GetHex());

    UniValue deltas(UniValue::VARR);

    for (unsigned int i = 0; i < block.vtx.size(); i++) {
        const CTransaction &tx = *(block.vtx[i]);
        const uint256 txhash = tx.GetHash();

        UniValue entry(UniValue::VOBJ);
        entry.pushKV("txid", txhash.GetHex());
        entry.pushKV("index", (int)i);

        UniValue inputs(UniValue::VARR);

        if (!tx.IsCoinBase()) {

            for (size_t j = 0; j < tx.vin.size(); j++) {
                const CTxIn input = tx.vin[j];

                UniValue delta(UniValue::VOBJ);

                CSpentIndexValue spentInfo;
                CSpentIndexKey spentKey(input.prevout.hash, input.prevout.n);

                if (GetSpentIndex(spentKey, spentInfo, pmempool)) {
                    std::string address;
                    if (!getAddressFromIndex(spentInfo.addressType, spentInfo.addressHash, address)) {
                        continue;
                    }
                    delta.pushKV("address", address);
                    delta.pushKV("satoshis", -1 * spentInfo.satoshis);
                    delta.pushKV("index", (int)j);
                    delta.pushKV("prevtxid", input.prevout.hash.GetHex());
                    delta.pushKV("prevout", (int)input.prevout.n);

                    inputs.push_back(delta);
                } else {
                    throw JSONRPCError(RPC_INTERNAL_ERROR, "Spent information not available");
                }
            }
        }

        entry.pushKV("inputs", inputs);

        UniValue outputs(UniValue::VARR);
        for(unsigned int k = 0; k < (tx.nVersion >= TX_ELE_VERSION ? tx.vpout.size() : tx.vout.size()) ; k++){
            CTxOutAsset out = (tx.nVersion >= TX_ELE_VERSION ? tx.vpout[k] : tx.vout[k]);

            UniValue delta(UniValue::VOBJ);

            delta.pushKV("index", (int)k);
            delta.pushKV("type", "standard");
            delta.pushKV("satoshis", out.nValue);
            AddAddress(&out.scriptPubKey, delta);
            outputs.push_back(delta);
        }

        entry.pushKV("outputs", outputs);
        deltas.push_back(entry);

    }
    result.pushKV("deltas", deltas);
    //PushTime(result, "time", block.GetBlockTime());
    //PushTime(result, "mediantime", blockindex->GetMedianTimePast());
    result.pushKV("nonce", (uint64_t)block.nNonce);
    result.pushKV("bits", strprintf("%08x", block.nBits));
    result.pushKV("difficulty", GetDifficulty(blockindex));
    result.pushKV("chainwork", blockindex->nChainWork.GetHex());

    if (blockindex->pprev)
        result.pushKV("previousblockhash", blockindex->pprev->GetBlockHash().GetHex());
    CBlockIndex *pnext = ::ChainActive().Next(blockindex);
    if (pnext)
        result.pushKV("nextblockhash", pnext->GetBlockHash().GetHex());
    return result;
}

static RPCHelpMan getblockdeltas()
{
return RPCHelpMan{"getblockdeltas",
        "\nReturns block deltas.\n",
        {
            {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The block hash"},
        },
        RPCResults{},
        RPCExamples{
        HelpExampleCli("getblockdeltas", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"") +
        "\nAs a JSON-RPC call\n"
        + HelpExampleRpc("getblockdeltas", "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    LOCK(cs_main);

    const CTxMemPool& mempool = EnsureMemPool(request.context);

    std::string strHash = request.params[0].get_str();
    uint256 hash(uint256S(strHash));

    if (g_chainman.BlockIndex().count(hash) == 0) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }

    CBlock block;
    CBlockIndex* pblockindex = g_chainman.BlockIndex()[hash];

    if (fHavePruned && !(pblockindex->nStatus & BLOCK_HAVE_DATA) && pblockindex->nTx > 0) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Block not available (pruned data)");
    }

    if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");
    }

    return blockToDeltasJSON(block, pblockindex, &mempool);
},
    };
}

static RPCHelpMan getblockhashes()
{
    return RPCHelpMan{"getblockhashes",
                "\nReturns array of hashes of blocks within the timestamp range provided.\n",
                {
                    {"high", RPCArg::Type::NUM, RPCArg::Optional::NO, "The newer block timestamp."},
                    {"low", RPCArg::Type::NUM, RPCArg::Optional::NO, "The older block timestamp."},
                    {"options", RPCArg::Type::OBJ, /* default */ "", "",
                        {
                            {"noOrphans", RPCArg::Type::BOOL, /* default */ "false", "Only include blocks on the main chain."},
                            {"logicalTimes", RPCArg::Type::BOOL, /* default */ "false", "Include logical timestamps with hashes."},
                        },
                        "options"},
                },
                RPCResults{
                    {RPCResult::Type::ARR, "", "", {
                        {RPCResult::Type::STR_HEX, "hash", "The block hash"},
                    }},
                    {RPCResult::Type::ARR, "", "", {
                        {RPCResult::Type::OBJ, "", "", {
                            {RPCResult::Type::STR_HEX, "blockhash", "The block hash"},
                            {RPCResult::Type::NUM, "logicalts", "The logical timestamp"},
                            {RPCResult::Type::NUM, "height", "The height of the block containing the spending tx"},
                        }}
                    }}
                },
                RPCExamples{
            HelpExampleCli("getblockhashes", "1231614698 1231024505 '{\"noOrphans\":false, \"logicalTimes\":true}'") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("getblockhashes", "1231614698, 1231024505")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    unsigned int high = request.params[0].get_int();
    unsigned int low = request.params[1].get_int();
    bool fActiveOnly = false;
    bool fLogicalTS = false;

    if (request.params.size() > 2) {
        if (request.params[2].isObject()) {
            UniValue noOrphans = find_value(request.params[2].get_obj(), "noOrphans");
            UniValue returnLogical = find_value(request.params[2].get_obj(), "logicalTimes");

            if (noOrphans.isBool()) {
                fActiveOnly = noOrphans.get_bool();
            }
            if (returnLogical.isBool()) {
                fLogicalTS = returnLogical.get_bool();
            }
        }
    }

    std::vector<std::pair<uint256, unsigned int> > blockHashes;

    {
        LOCK(cs_main);
        if (!GetTimestampIndex(high, low, fActiveOnly, blockHashes)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for block hashes");
        }
    }

    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<uint256, unsigned int> >::const_iterator it=blockHashes.begin(); it!=blockHashes.end(); it++) {
        if (fLogicalTS) {
            UniValue item(UniValue::VOBJ);
            item.pushKV("blockhash", it->first.GetHex());
            item.pushKV("logicalts", (int)it->second);
            result.push_back(item);
        } else {
            result.push_back(it->first.GetHex());
        }
    }

    return result;
},
    };
}

static RPCHelpMan gettxoutsetinfobyscript()
{
    return RPCHelpMan{"gettxoutsetinfobyscript",
                "\nReturns statistics about the unspent transaction output set per script type.\n"
                "This call may take some time.\n",
                {
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "", {
                        {RPCResult::Type::NUM, "height", "The current block height (index)"},
                        {RPCResult::Type::STR_HEX, "bestblock", "The best block hash hex"},
                    }
                },
                RPCExamples{
            HelpExampleCli("gettxoutsetinfobyscript", "") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("gettxoutsetinfobyscript", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue ret(UniValue::VOBJ);

    int nHeight;
    uint256 hashBlock;

    std::unique_ptr<CCoinsViewCursor> pcursor;
    {
        LOCK(cs_main);
        ::ChainstateActive().ForceFlushStateToDisk();
        pcursor = std::unique_ptr<CCoinsViewCursor>(::ChainstateActive().CoinsDB().Cursor());
        assert(pcursor);
        hashBlock = pcursor->GetBestBlock();
        nHeight = g_chainman.BlockIndex().find(hashBlock)->second->nHeight;
    }

    class PerScriptTypeStats {
    public:
        int64_t nPlain = 0;
        int64_t nBlinded = 0;
        int64_t nPlainValue = 0;

        UniValue ToUV()
        {
            UniValue ret(UniValue::VOBJ);
            ret.pushKV("num_plain", nPlain);
            ret.pushKV("num_blinded", nBlinded);
            ret.pushKV("total_amount", ValueFromAmount(nPlainValue));
            return ret;
        }
    };

    PerScriptTypeStats statsPKH;
    PerScriptTypeStats statsSH;
    PerScriptTypeStats statsOther;

    while (pcursor->Valid()) {
        if (ShutdownRequested()) return false;
        COutPoint key;
        Coin coin;
        if (pcursor->GetKey(key) && pcursor->GetValue(coin)) {
            PerScriptTypeStats *ps = &statsOther;
            if (coin.out.scriptPubKey.IsPayToPubkeyHash()) {
                ps = &statsPKH;
            } else if (coin.out.scriptPubKey.IsPayToScriptHash()) {
                ps = &statsSH;
            }
            ps->nPlainValue += coin.out.nValue;

        } else {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Unable to read UTXO set");
        }
        pcursor->Next();
    }

    ret.pushKV("height", (int64_t)nHeight);
    ret.pushKV("bestblock", hashBlock.GetHex());
    ret.pushKV("paytopubkeyhash", statsPKH.ToUV());
    ret.pushKV("paytoscripthash", statsSH.ToUV());
    ret.pushKV("other", statsOther.ToUV());

    return ret;
},
    };
}

static void pushScript(UniValue &uv, const std::string &name, const CScript *script)
{
    if (!script) {
        return;
    }

    UniValue uvs(UniValue::VOBJ);
    uvs.pushKV("hex", HexStr(*script));

    CTxDestination dest_stake, dest_spend;
    ExtractDestination(*script, dest_spend);
    
    if (!std::get_if<CNoDestination>(&dest_stake)) {
        uvs.pushKV("stakeaddr", EncodeDestination(dest_stake));
    }
    if (!std::get_if<CNoDestination>(&dest_spend)) {
        uvs.pushKV("spendaddr", EncodeDestination(dest_spend));
    }
    uv.pushKV(name, uvs);
}

static RPCHelpMan getblockreward()
{
    return RPCHelpMan{"getblockreward",
                "\nReturns the blockreward for block at height.\n",
                {
                    {"height", RPCArg::Type::NUM, RPCArg::Optional::NO, "The chain height of the block."},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "", {
                        {RPCResult::Type::STR_HEX, "blockhash", "The hash of the block"},
                        {RPCResult::Type::STR_HEX, "coinstake", "The hash of the coinstake transaction"},
                        {RPCResult::Type::NUM_TIME, "blocktime", "The block time expressed in " _UNIX_EPOCH_TIME},
                        {RPCResult::Type::STR_AMOUNT, "stakereward", "The stake reward portion, newly minted coin"},
                        {RPCResult::Type::STR_AMOUNT, "blockreward", "The block reward, value paid to staker, including fees"},
                        {RPCResult::Type::STR_AMOUNT, "foundationreward", "The accumulated foundation reward payout, if any"},
                        {RPCResult::Type::OBJ, "kernelscript", "", {
                            {RPCResult::Type::STR_HEX, "hex", "The script from the kernel output"},
                            {RPCResult::Type::STR, "stakeaddr", "The stake address, if output script is coldstake"},
                            {RPCResult::Type::STR, "spendaddr", "The spend address"},
                        }},
                        {RPCResult::Type::ARR, "", "", {
                            {RPCResult::Type::OBJ, "script", "", {
                                {RPCResult::Type::STR_HEX, "hex", "The script from the kernel output"},
                                {RPCResult::Type::STR, "stakeaddr", "The stake address, if output script is coldstake"},
                                {RPCResult::Type::STR, "spendaddr", "The spend address"},
                            }},
                            {RPCResult::Type::STR_AMOUNT, "value", "The value of the output"},
                        }}
                    }
                },
                RPCExamples{
            HelpExampleCli("getblockreward", "1000") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("getblockreward", "1000")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VNUM});

    NodeContext& node = EnsureNodeContext(request.context);
    if (!g_txindex) {
        throw JSONRPCError(RPC_MISC_ERROR, "Requires -txindex enabled");
    }

    int nHeight = request.params[0].get_int();
    if (nHeight < 0 || nHeight > ::ChainActive().Height()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
    }

    LOCK(cs_main);

    CBlockIndex *pblockindex = ::ChainActive()[nHeight];

    CAmount stake_reward = 0;
    //if (pblockindex->pprev) {
    //    stake_reward = Params().GetProofOfStakeReward(pblockindex->pprev, 0);
    //}

    CBlock block;
    if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
        throw JSONRPCError(RPC_MISC_ERROR, "Block not found on disk");
    }

    const auto &tx = block.vtx[0];

    UniValue outputs(UniValue::VARR);
    CAmount value_out = 0, value_in = 0, value_foundation = 0;
    for (const auto &txout : tx->vout) {
        UniValue output(UniValue::VOBJ);
        pushScript(output, "script", &txout.scriptPubKey);
        output.pushKV("value", ValueFromAmount(txout.nValue));
        outputs.push_back(output);

        value_out += txout.nValue;
    }

    CScript kernel_script;
    CAsset asset;
    int n = -1;
    for (const auto& txin : tx->vin) {
        n++;

        CBlockIndex *blockindex = nullptr;
        uint256 hash_block;
        const CTransactionRef tx_prev = GetTransaction(blockindex, node.mempool.get(), txin.prevout.hash, Params().GetConsensus(), hash_block);
        if (!tx_prev) {
            throw JSONRPCError(RPC_MISC_ERROR, "Transaction not found on disk");
        }
        if (txin.prevout.n > tx_prev->vout.size()) {
            throw JSONRPCError(RPC_MISC_ERROR, "prevout not found on disk");
        }
        value_in += tx_prev->vout[txin.prevout.n].nValue;
        if (n == 0) {
            kernel_script = tx_prev->vout[txin.prevout.n].scriptPubKey;
        }
    }

    CAmount block_reward = value_out - value_in;

    UniValue rv(UniValue::VOBJ);
    rv.pushKV("blockhash", pblockindex->GetBlockHash().ToString());
    if (tx->IsCoinStake()) {
        rv.pushKV("coinstake", tx->GetHash().ToString());
    }

    rv.pushKV("blocktime", pblockindex->GetBlockTime());
    rv.pushKV("stakereward", ValueFromAmount(stake_reward));
    rv.pushKV("blockreward", ValueFromAmount(block_reward));

    if (value_foundation > 0) {
        rv.pushKV("foundationreward", ValueFromAmount(value_foundation));
    }

    if (tx->IsCoinStake()) {
        pushScript(rv, "kernelscript", &kernel_script);
    }
    rv.pushKV("outputs", outputs);

    return rv;
},
    };
}

static RPCHelpMan getinsightinfo()
{
    return RPCHelpMan{"getinsightinfo",
            "\nReturns an object of enabled indices.\n",
            {
            },
            RPCResult{
                RPCResult::Type::OBJ, "", "", {
                    {RPCResult::Type::BOOL, "txindex", "True if txindex is enabled"},
                    {RPCResult::Type::BOOL, "addressindex", "True if addressindex is enabled"},
                    {RPCResult::Type::BOOL, "spentindex", "True if spentindex is enabled"},
                    {RPCResult::Type::BOOL, "timestampindex", "True if timestampindex is enabled"},
                }
            },
            RPCExamples{
        HelpExampleCli("getindexinfo", "") +
        "\nAs a JSON-RPC call\n"
        + HelpExampleRpc("getindexinfo", "")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue ret(UniValue::VOBJ);

    ret.pushKV("txindex", (g_txindex ? true : false));
    ret.pushKV("addressindex", fAddressIndex);
    ret.pushKV("spentindex", fSpentIndex);
    ret.pushKV("timestampindex", fTimestampIndex);

    return ret;
},
    };
}

static const CRPCCommand commands[] =
{ //  category              actor (function)
  //  --------------------- -----------------------
    { "addressindex",       "getaddressmempool", &getaddressmempool      ,{} },
    { "addressindex",       "getaddressutxos", &getaddressutxos        ,{} },
    { "addressindex",       "getaddressdeltas", &getaddressdeltas       ,{} },
    { "addressindex",       "getaddresstxids", &getaddresstxids        ,{} },
    { "addressindex",       "getaddressbalance", &getaddressbalance      ,{} },

    { "blockchain",         "getspentinfo", &getspentinfo           ,{} },
    { "blockchain",         "getblockdeltas", &getblockdeltas         ,{} },
    { "blockchain",         "getblockhashes", &getblockhashes         ,{} },
    { "blockchain",         "gettxoutsetinfobyscript", &gettxoutsetinfobyscript,{} },
    { "blockchain",         "getblockreward", &getblockreward         ,{} },
    { "blockchain",         "getinsightinfo", &getinsightinfo         ,{} },
};

void RegisterInsightRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
