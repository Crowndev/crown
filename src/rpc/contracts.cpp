
#include <pubkey.h>
#include <core_io.h>
#include <key_io.h>
#include <logging.h>
#include <assetdb.h>

#include <rpc/rawtransaction_util.h>
#include <rpc/util.h>
#include <rpc/server.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/standard.h>

#include <crypto/sha1.h>

#include <wallet/coincontrol.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>
#include <util/translation.h>
#include <univalue.h>
#include <validation.h>
#include <boost/algorithm/string.hpp>

typedef std::vector<unsigned char> valtype;

static RPCHelpMan hashmessage()
{
    return RPCHelpMan{"hashmessage",
                "\nSign a message with the private key of an address\n",
                {
                    {"hashtype", RPCArg::Type::STR, RPCArg::Optional::NO, "The type of hash required. sha256, hash256, hash160, ripemd160, sha1."},
                    {"message", RPCArg::Type::STR, RPCArg::Optional::NO, "The message to hash."},
                },
                RPCResult{
                    RPCResult::Type::STR, "signature", "The signature of the message encoded in base 64"
                },
                RPCExamples{
            "\nCreate the signature\n"
            + HelpExampleCli("hashmessage", "\"hash type\" \"my message\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("hashmessage", "\"hash type\", \"my message\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string hashtype = request.params[0].get_str();
    std::string svch = request.params[1].get_str();
    valtype vch(svch.begin(), svch.end());

    valtype vchHash((hashtype == "ripemd160" || hashtype == "sha1" || hashtype == "hash160") ? 20 : 32);
    if (hashtype == "ripemd160")
        CRIPEMD160().Write(vch.data(), vch.size()).Finalize(vchHash.data());
    else if (hashtype == "sha1")
        CSHA1().Write(vch.data(), vch.size()).Finalize(vchHash.data());
    else if (hashtype == "sha256")
        CSHA256().Write(vch.data(), vch.size()).Finalize(vchHash.data());
    else if (hashtype == "hash160")
        CHash160().Write(vch).Finalize(vchHash);
    else if (hashtype =="hash256")
        CHash256().Write(vch).Finalize(vchHash);

    return HexStr(vchHash);
},
    };
}

static RPCHelpMan createasset()
{
    return RPCHelpMan{"createasset",
                "\ncreate a new asset\n",
                {
                    {"name", RPCArg::Type::STR, RPCArg::Optional::NO, "Max 10 characters"},
                    {"short_name", RPCArg::Type::STR, RPCArg::Optional::NO, "Max 4 characters"},
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "address"},
                    {"contracturl", RPCArg::Type::STR, RPCArg::Optional::NO, "url"},
                    {"script", RPCArg::Type::STR, RPCArg::Optional::NO, "code"},
                    {"input_amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "In put amount in Crown. (minimum 1)"},
                    {"asset_amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "Amount of asset to generate. Note that the amount is Crown-like, with 8 decimal places."},
                    {"expiry", RPCArg::Type::NUM, RPCArg::Optional::NO, "Expiry date of asset"},
                    {"type", RPCArg::Type::NUM, RPCArg::Optional::NO, "asset type TOKEN = 1, UNIQUE = 2, EQUITY = 3, POINTS = 4, CREDITS = 5"},
                    {"transferable", RPCArg::Type::BOOL, RPCArg::Optional::NO, "asset can be transfered to other addresses after initial creation"},
                    {"convertable", RPCArg::Type::BOOL, RPCArg::Optional::NO, "asset can be converted to another asset (set false for NFTs)"},
                    {"restricted", RPCArg::Type::BOOL, RPCArg::Optional::NO, "asset can only be issued/reissued by creation address"},
                    {"limited", RPCArg::Type::BOOL, RPCArg::Optional::NO, "other assets cannot be converted to this one"},
                    {"divisible", RPCArg::Type::BOOL, RPCArg::Optional::NO, "asset is divisible (units smaller than 1.0)"},
                    {"data", RPCArg::Type::STR, RPCArg::Optional::NO, "Text form NFT data"},
                },
                RPCResult{
                    RPCResult::Type::STR, "assetID", "The id of the asset encoded in hex"
                },
                RPCExamples{
            "\nCreate an asset\n"
            + HelpExampleCli("createasset", "\"hash type\" \"my message\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("createasset", "\"hash type\", \"my message\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;
    CWallet* const pwallet = wallet.get();

    LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*wallet);

    LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);

    EnsureWalletIsUnlocked(pwallet);

    std::string name = request.params[0].get_str();
    std::string shortname = request.params[1].get_str();
    std::string address = request.params[2].get_str();
    std::string contracturl = request.params[3].get_str();
    std::string scriptcode = request.params[4].get_str();

    std::vector<unsigned char> scriptcodedata(ParseHex(scriptcode));
    CScript script(scriptcodedata.begin(), scriptcodedata.end());

    CAmount nAmount = AmountFromValue(request.params[5].get_int());
    CAmount nAssetAmount = AmountFromValue(request.params[6].get_int());
    int64_t expiry = request.params[7].get_int();
    int type = request.params[8].get_int();
    bool transferable = request.params[9].get_bool();
    bool convertable = request.params[10].get_bool();
    bool restricted = request.params[11].get_bool();
    bool limited = request.params[12].get_bool();
    bool divisible = request.params[13].get_bool();
    std::string datastring = request.params[14].get_str();

    CTxData rdata;
    rdata.vData.assign(datastring.begin(), datastring.end());
    
    CAsset asset;
    CTransactionRef tx;
    std::string strFailReason;

    if(!pwallet->CreateAsset(asset, tx, name, shortname, address, contracturl, script, nAmount, nAssetAmount, expiry, type, rdata, strFailReason, transferable, convertable, restricted, limited, divisible))
        throw JSONRPCError(RPC_MISC_ERROR, strFailReason);

    return tx->GetHash().GetHex();
},
    };
}

static RPCHelpMan convertasset()
{
    return RPCHelpMan{"convertasset",
                "\nconvert asset to CRW\n",
                {
                    {"name", RPCArg::Type::STR, RPCArg::Optional::NO, "Max 10 characters"},
                    {"input_amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "In put amount in Crown. (minimum 1)"},
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Destination address"},
                },
                RPCResult{
                    RPCResult::Type::STR, "TXID", "The txid of the asset conversion in hex"
                },
                RPCExamples{
            "\nconvert an asset\n"
            + HelpExampleCli("convertasset", "\"name\" \"input_amount\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("convertasset", "\"name\", \"input_amount\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;
    CWallet* const pwallet = wallet.get();

    LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*wallet);

    LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);

    EnsureWalletIsUnlocked(pwallet);

    CAsset asset = GetAsset(request.params[0].get_str());
    CAmount nAmount = AmountFromValue(request.params[1]);
    std::string address = request.params[2].get_str();

    CAmountMap assetAmount{{asset,nAmount}};
    CTransactionRef tx;
    std::string strFailReason;

    if(!pwallet->ConvertAsset(assetAmount, tx, address, strFailReason))
        throw JSONRPCError(RPC_MISC_ERROR, strFailReason);

    return tx->GetHash().GetHex();
},
    };
}

static RPCHelpMan getasset()
{
    return RPCHelpMan{"getasset",
                "\n  Get Details of an asset \n",
                {
                    {"asset", RPCArg::Type::STR, RPCArg::Optional::NO, "The asset to retrieve"},
                },
                RPCResult{
                    RPCResult::Type::STR, "details", "The asset "
                },
                RPCExamples{
            "\nRetrieve a asset\n"
            + HelpExampleCli("getasset", "\"Asset Name\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("getasset", "\"Asset Name\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string assetstring = request.params[0].get_str();
    CAssetData asset = GetAssetData(assetstring);
    UniValue result(UniValue::VOBJ);

    AssetToUniv(asset,result);

    return result;
},
    };
}

static RPCHelpMan getassets()
{
    return RPCHelpMan{"getassets",
                "\n  Get List of assets \n",
                {
                },
                RPCResult{
                    RPCResult::Type::STR, "details", "The assets "
                },
                RPCExamples{
            "\nRetrieve a asset\n"
            + HelpExampleCli("getassets", "\"Asset Name\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("getassets", "\"Asset Name\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::vector<CAsset> allassets = ::GetAllAssets();

    UniValue result(UniValue::VARR);
    for(auto asset : allassets){
        UniValue entry(UniValue::VOBJ);
        AssetToUniv(asset,entry);
        result.push_back(entry);
    }

    return result;
},
    };
}

static RPCHelpMan getnumassets()
{
    return RPCHelpMan{"getnumassets",
                "\n  Get List of assets \n",
                {
                },
                RPCResult{
                    RPCResult::Type::STR, "details", "The assets "
                },
                RPCExamples{
            "\nRetrieve a asset\n"
            + HelpExampleCli("getnumassets", "\"Asset Name\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("getnumassets", "\"Asset Name\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::vector<CAsset> allassets = ::GetAllAssets();
    UniValue deets(UniValue::VOBJ);
    deets.pushKV("Asset count", (int64_t)allassets.size());

    return deets;
},
    };
}

void RegisterContractRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              actor (function)
  //  --------------------- ------------------------
    { "contracts",  "hashmessage",         &hashmessage,            {}    },
    { "contracts",  "createasset",         &createasset,            {}    },
    { "contracts",  "convertasset",        &convertasset,           {}    },
    { "contracts",  "getasset",            &getasset,               {}    },
    { "contracts",  "getassets",           &getassets,              {}    },
    { "contracts",  "getnumassets",        &getnumassets,           {}    },

};

// clang-format on
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
