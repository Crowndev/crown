// Copyright (c) 2014-2016 The ShadowCoin developers
// Copyright (c) 2017-2021 The Particl Core developers
// Copyright (c) 2009-2020 The Hemis Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <rpc/register.h>

#include <algorithm>
#include <string>

#include <smsg/smessage.h>
#include <smsg/db.h>
#include <wallet/ismine.h>
#include <util/strencodings.h>
#include <core_io.h>
#include <base58.h>
#include <rpc/util.h>
#include <validation.h>
#include <timedata.h>
#include <validationinterface.h>
#include <util/string.h>
#include <util/time.h>

#include <leveldb/db.h>

#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#include <wallet/rpcwallet.h>
#include <wallet/coincontrol.h>
#endif

#include <univalue.h>
#include <fstream>
#include <boost/xpressive/xpressive.hpp>

static std::string dequote(std::string s)
{
    std::string str(s.c_str());
    boost::xpressive::sregex nums = boost::xpressive::sregex::compile(":\\\"(-?\\d*(\\.\\d+))\\\"");
    std::string nm(":$1");
    str = regex_replace(str, nums, nm);
    boost::xpressive::sregex tru = boost::xpressive::sregex::compile("\\\"true\\\"");
    std::string tr("true");
    str = regex_replace(str, tru, tr);
    boost::xpressive::sregex fal = boost::xpressive::sregex::compile("\\\"false\\\"");
    std::string fl("false");
    str = regex_replace(str, fal, fl);
    return str;
}

static void EnsureSMSGIsEnabled()
{
    if (!smsg::fSecMsgEnabled)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Secure messaging is disabled.");
};

static RPCHelpMan smsgenable()
{
    return RPCHelpMan{"smsgenable",
                "Enable secure messaging with the specified wallet as the active wallet.\n"
                "Uses the first smsg-enabled wallet as the active wallet if none specified.\n",
                {
                    {"walletname", RPCArg::Type::STR, /* default */ "wallet.dat", "Active smsg wallet."},
                },
                RPCResults{},
                RPCExamples{
            HelpExampleCli("smsgenable", "\"wallet_name\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("smsgenable", "\"wallet_name\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (smsg::fSecMsgEnabled) {
        throw JSONRPCError(RPC_MISC_ERROR, "Secure messaging is already enabled.");
    }

    UniValue result(UniValue::VOBJ);

    std::shared_ptr<CWallet> pwallet;
    std::string sFindWallet, wallet_name = "Not set.";
#ifdef ENABLE_WALLET
    auto vpwallets = GetWallets();

    if (!request.params[0].isNull()) {
        sFindWallet = request.params[0].get_str();
    }
    for (const auto &pw : vpwallets) {
        CWallet *const ppartw = pw.get();
        if (!ppartw) {
            continue;
        }
        if (!request.params[0].isNull() && ppartw->GetName() == sFindWallet) {
            pwallet = pw;
            break;
        }
        if (ppartw->m_smsg_enabled) {
            pwallet = pw;
            break;
        }
    }
    if (!request.params[0].isNull() && !pwallet) {
        throw JSONRPCError(RPC_MISC_ERROR, "Wallet not found: \"" + sFindWallet + "\"");
    }
    if (pwallet) {
        wallet_name = pwallet->GetName();
    }
    result.pushKV("result", (smsgModule.Enable(pwallet, vpwallets) ? "Enabled secure messaging." : "Failed."));
#else
    std::vector<std::shared_ptr<CWallet>> empty;
    result.pushKV("result", (smsgModule.Enable(pwallet, empty) ? "Enabled secure messaging." : "Failed."));
 #endif

    result.pushKV("wallet", wallet_name);

    return result;
},
    };
}

static RPCHelpMan smsgdisable()
{
    return RPCHelpMan{"smsgdisable",
                "\nDisable secure messaging.\n",
                {
                },
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("smsgdisable", "")
                    + HelpExampleRpc("smsgdisable", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (!smsg::fSecMsgEnabled)
        throw JSONRPCError(RPC_MISC_ERROR, "Secure messaging is already disabled.");

    UniValue result(UniValue::VOBJ);

    result.pushKV("result", (smsgModule.Disable() ? "Disabled secure messaging." : "Failed."));

    return result;
},
    };
}

static RPCHelpMan smsgsetwallet()
{
    return RPCHelpMan{"smsgsetwallet",
                "Set secure messaging to use the specified wallet.\n"
                "SMSG can only be enabled on one wallet.\n"
                "Call with no parameters to unset the active wallet.\n",
                {
                    {"walletname", RPCArg::Type::STR, /* default */ "wallet.dat", "Enable smsg on a specific wallet."},
                },
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("smsgsetwallet", "\"wallet_name\"")
                    + HelpExampleRpc("smsgsetwallet", "\"wallet_name\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (!smsg::fSecMsgEnabled) {
        throw JSONRPCError(RPC_MISC_ERROR, "Secure messaging must be enabled.");
    }

    UniValue result(UniValue::VOBJ);

    std::shared_ptr<CWallet> pwallet;
    std::string wallet_name = "Not set.";
#ifndef ENABLE_WALLET
    throw JSONRPCError(RPC_MISC_ERROR, "Wallet is disabled.");
#else
    auto vpwallets = GetWallets();

    if (!request.params[0].isNull()) {
        std::string sFindWallet = request.params[0].get_str();

        for (const auto &pw : vpwallets) {
            if (pw->GetName() != sFindWallet) {
                continue;
            }
            pwallet = pw;
            break;
        }
        if (!pwallet) {
            throw JSONRPCError(RPC_MISC_ERROR, "Wallet not found: \"" + sFindWallet + "\"");
        }
    }
    if (pwallet) {
        wallet_name = pwallet->GetName();
    }
#endif

    result.pushKV("result", (smsgModule.SetActiveWallet(pwallet) ? "Set active wallet." : "Failed."));
    result.pushKV("wallet", wallet_name);

    return result;
},
    };
}

static RPCHelpMan smsgoptions()
{
    return RPCHelpMan{"smsgoptions",
                "\nList and manage options.\n",
                {
                    {"mode", RPCArg::Type::STR, /* default */ "list", "Mode: list or set, 2nd arg is with_description in list mode."},
                    {"optname", RPCArg::Type::STR, /* default */ "", "Option name."},
                    {"value", RPCArg::Type::STR, /* default */ "", "New option value."},
                },
                RPCResults{},
                RPCExamples{
            "\nList possible options with descriptions.\n"
            + HelpExampleCli("smsgoptions", "list 1")
            + HelpExampleRpc("smsgoptions", "\"list\", 1")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string mode = "list";
    if (request.params.size() > 0) {
        mode = request.params[0].get_str();
    }

    UniValue result(UniValue::VOBJ);

    if (mode == "list") {
        UniValue options(UniValue::VARR);

        bool fDescriptions = false;
        if (!request.params[1].isNull()) {
            fDescriptions = GetBool(request.params[1]);
        }

        UniValue option(UniValue::VOBJ);
        option.pushKV("name", "newAddressRecv");
        option.pushKV("value", smsgModule.options.fNewAddressRecv);
        if (fDescriptions) {
            option.pushKV("description", "Enable receiving messages for newly created addresses.");
        }
        options.push_back(option);

        option = UniValue(UniValue::VOBJ);
        option.pushKV("name", "newAddressAnon");
        option.pushKV("value", smsgModule.options.fNewAddressAnon);
        if (fDescriptions) {
            option.pushKV("description", "Enable receiving anonymous messages for newly created addresses.");
        }
        options.push_back(option);

        option = UniValue(UniValue::VOBJ);
        option.pushKV("name", "scanIncoming");
        option.pushKV("value", smsgModule.options.fScanIncoming);
        if (fDescriptions) {
            option.pushKV("description", "Scan incoming blocks for public keys, -smsgscanincoming must also be set");
        }
        options.push_back(option);

        result.pushKV("options", options);
        result.pushKV("result", "Success.");
    } else
    if (mode == "set") {
        if (request.params.size() < 3) {
            result.pushKV("result", "Too few parameters.");
            result.pushKV("expected", "set <optname> <value>");
            return result;
        }

        std::string optname = request.params[1].get_str();
        bool fValue = GetBool(request.params[2]);

        std::transform(optname.begin(), optname.end(), optname.begin(), ::tolower);
        if (optname == "newaddressrecv") {
            smsgModule.options.fNewAddressRecv = fValue;
            result.pushKV("set option", std::string("newAddressRecv = ") + (smsgModule.options.fNewAddressRecv ? "true" : "false"));
        } else
        if (optname == "newaddressanon") {
            smsgModule.options.fNewAddressAnon = fValue;
            result.pushKV("set option", std::string("newAddressAnon = ") + (smsgModule.options.fNewAddressAnon ? "true" : "false"));
        } else
        if (optname == "scanincoming") {
            smsgModule.options.fScanIncoming = fValue;
            result.pushKV("set option", std::string("scanIncoming = ") + (smsgModule.options.fScanIncoming ? "true" : "false"));
        } else {
            result.pushKV("result", "Option not found.");
            return result;
        }
    } else {
        result.pushKV("result", "Unknown Mode.");
        result.pushKV("expected", "smsgoptions [list|set <optname> <value>]");
    }

    return result;
},
    };
}

static RPCHelpMan smsglocalkeys()
{
    return RPCHelpMan{"smsglocalkeys",
                "\nList and manage keys messages can be received with.\n",
                {
                    {"mode", RPCArg::Type::STR, /* default */ "whitelist", "whitelist|all|wallet|recv +/- \"address\"|anon +/- \"address\""},
                    {"optype", RPCArg::Type::STR, /* default */ "", "Add or remove +/-."},
                    {"address", RPCArg::Type::STR, /* default */ "", "Address to affect."},
                },
                RPCResults{},
                RPCExamples{
                    "\nList local keys.\n"
                    + HelpExampleCli("smsglocalkeys", "")
                    + HelpExampleRpc("smsglocalkeys", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    UniValue result(UniValue::VOBJ);

    std::string mode = "whitelist";
    if (request.params.size() > 0) {
        mode = request.params[0].get_str();
    }

    if (mode == "whitelist"
        || mode == "all") {
        LOCK(smsgModule.cs_smsg);
        uint32_t nKeys = 0;

        UniValue keys(UniValue::VARR);
#ifdef ENABLE_WALLET
        int all = mode == "all" ? 1 : 0;
        for (auto it = smsgModule.addresses.begin(); it != smsgModule.addresses.end(); ++it) {
            if (!all
                && !it->fReceiveEnabled) {
                continue;
            }

            CKeyID &keyID = it->address;
            std::string sPublicKey;
            CPubKey pubKey;

            if (0 == smsgModule.GetLocalKey(keyID, pubKey)) {
                sPublicKey = EncodeBase58(pubKey);
            }

            UniValue objM(UniValue::VOBJ);
            std::string sInfo, sLabel;
            PKHash wpkh = PKHash(keyID);
            sLabel = smsgModule.LookupLabel(wpkh);
            if (all) {
                sInfo = std::string("Receive ") + (it->fReceiveEnabled ? "on,  " : "off, ");
            }
            sInfo += std::string("Anon ") + (it->fReceiveAnon ? "on" : "off");
            //result.pushKV("key", it->sAddress + " - " + sPublicKey + " " + sInfo + " - " + sLabel);
            objM.pushKV("address", EncodeDestination(PKHash(keyID)));
            objM.pushKV("public_key", sPublicKey);
            objM.pushKV("receive", (it->fReceiveEnabled ? "1" : "0"));
            objM.pushKV("anon", (it->fReceiveAnon ? "1" : "0"));
            objM.pushKV("label", sLabel);
            keys.push_back(objM);

            nKeys++;
        }
        result.pushKV("wallet_keys", keys);
#endif

        keys = UniValue(UniValue::VARR);
        for (auto &p : smsgModule.keyStore.mapKeys) {
            auto &key = p.second;
            UniValue objM(UniValue::VOBJ);
            CPubKey pk = key.key.GetPubKey();
            objM.pushKV("address", EncodeDestination(PKHash(p.first)));
            objM.pushKV("public_key", EncodeBase58(pk));
            objM.pushKV("receive", (key.nFlags & smsg::SMK_RECEIVE_ON ? "1" : "0"));
            objM.pushKV("anon", (key.nFlags & smsg::SMK_RECEIVE_ANON ? "1" : "0"));
            objM.pushKV("label", key.sLabel);
            keys.push_back(objM);

            nKeys++;
        }
        result.pushKV("smsg_keys", keys);

        result.pushKV("result", strprintf("%u", nKeys));
    } else
    if (mode == "recv") {
        if (request.params.size() < 3) {
            result.pushKV("result", "Too few parameters.");
            result.pushKV("expected", "recv <+/-> <address>");
            return result;
        }

        bool fValue = GetBool(request.params[1]);
        std::string addr = request.params[2].get_str();

        CTxDestination dest = DecodeDestination(addr);
        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address.");
        }
        CKeyID keyID = ToKeyID(std::get<PKHash>(dest));

        if (!smsgModule.SetWalletAddressOption(keyID, "receive", fValue)
            && !smsgModule.SetSmsgAddressOption(keyID, "receive", fValue)) {
            result.pushKV("result", "Address not found.");
            return result;
        }

        std::string sInfo;
        sInfo = std::string("Receive ") + (fValue ? "on" : "off");
        result.pushKV("result", "Success.");
        result.pushKV("key", EncodeDestination(dest) + " " + sInfo);
        return result;
    } else
    if (mode == "anon") {
        if (request.params.size() < 3) {
            result.pushKV("result", "Too few parameters.");
            result.pushKV("expected", "anon <+/-> <address>");
            return result;
        }

        bool fValue = GetBool(request.params[1]);
        std::string addr = request.params[2].get_str();

        CTxDestination dest = DecodeDestination(addr);
        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address.");
        }
        CKeyID keyID = ToKeyID(std::get<PKHash>(dest));

        if (!smsgModule.SetWalletAddressOption(keyID, "anon", fValue)
            && !smsgModule.SetSmsgAddressOption(keyID, "anon", fValue)) {
            result.pushKV("result", "Address not found.");
            return result;
        }

        std::string sInfo;
        sInfo += std::string("Anon ") + (fValue ? "on" : "off");
        result.pushKV("result", "Success.");
        result.pushKV("key", EncodeDestination(dest) + " " + sInfo);

        return result;
    } else
    if (mode == "wallet") {
#ifdef ENABLE_WALLET
        uint32_t nKeys = 0;
        UniValue keys(UniValue::VOBJ);
        for (const auto &pw : smsgModule.m_vpwallets) {
            LOCK(pw->cs_wallet);

            for (const auto &entry : pw->m_address_book) {
                if (!pw->IsMine(entry.first)) {
                    continue;
                }

                //CTxDestination dest = DecodeDestination(entry.first);
                if (!IsValidDestination(entry.first)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address.");
                }

                std::string address = EncodeDestination(entry.first);
                std::string sPublicKey;

                CKeyID keyID = ToKeyID(std::get<PKHash>(entry.first));

                CPubKey pubKey;
                LegacyScriptPubKeyMan* spk_man = pw->GetLegacyScriptPubKeyMan();

                if (!spk_man->GetPubKey(keyID, pubKey)) {
                    continue;
                }
                if (!pubKey.IsValid() || !pubKey.IsCompressed()) {
                    continue;
                }

                sPublicKey = EncodeBase58(pubKey);
                UniValue objM(UniValue::VOBJ);

                objM.pushKV("key", address);
                objM.pushKV("publickey", sPublicKey);
                objM.pushKV("label", entry.second.GetLabel());

                keys.push_back(objM);
                nKeys++;
            }
        }
        result.pushKV("keys", keys);
        result.pushKV("result", strprintf("%u", nKeys));
#else
        throw JSONRPCError(RPC_MISC_ERROR, "No wallet.");
#endif
    } else {
        result.pushKV("result", "Unknown Mode.");
        result.pushKV("expected", "smsglocalkeys [whitelist|all|wallet|recv <+/-> <address>|anon <+/-> <address>]");
    }

    return result;
},
    };
};

static RPCHelpMan smsgscanchain()
{
    return RPCHelpMan{"smsgscanchain",
                "\nLook for public keys in the block chain.\n",
                {},
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("smsgscanchain", "")
                    + HelpExampleRpc("smsgscanchain", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    UniValue result(UniValue::VOBJ);
    if (!smsgModule.ScanBlockChain()) {
        result.pushKV("result", "Scan Chain Failed.");
    } else {
        result.pushKV("result", "Scan Chain Completed.");
    }

    return result;
},
    };
}

static RPCHelpMan smsgscanbuckets()
{
    return RPCHelpMan{"smsgscanbuckets",
                "\nForce rescan of all messages in the bucket store.\n"
                "Wallet must be unlocked if any receiving keys are stored in the wallet.\n",
                {
                    {"options", RPCArg::Type::OBJ, /* default */ "", "",
                        {
                            {"scanexpired", RPCArg::Type::BOOL, /* default */ "false", "Scan all messages."},
                        },
                        "options"},
                },
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("smsgscanbuckets", "")
                    + HelpExampleRpc("smsgscanbuckets", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    bool scan_all = false;
    if (request.params[0].isObject()) {
        UniValue options = request.params[0].get_obj();
        RPCTypeCheckObj(options,
        {
            {"scanexpired",          UniValueType(UniValue::VBOOL)},
        }, true, true);
        if (options["scanexpired"].isBool()) {
            scan_all = options["scanexpired"].get_bool();
        }
    }

    UniValue result(UniValue::VOBJ);
    if (!smsgModule.ScanBuckets(scan_all)) {
        result.pushKV("result", "Scan Buckets Failed.");
    } else {
        result.pushKV("result", "Scan Buckets Completed.");
    }

    return result;
},
    };
}

static RPCHelpMan smsgaddaddress()
{
    return RPCHelpMan{"smsgaddaddress",
                "\nAdd address and matching public key to database.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address to add."},
                    {"pubkey", RPCArg::Type::STR, RPCArg::Optional::NO, "Public key for \"address\"."},
                },
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("smsgaddaddress", "\"address\" \"public_key\"")
                    + HelpExampleRpc("smsgaddaddress", "\"address\", \"public_key\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    std::string addr = request.params[0].get_str();
    std::string pubk = request.params[1].get_str();

    UniValue result(UniValue::VOBJ);
    int rv = smsgModule.AddAddress(addr, pubk);
    if (rv != 0) {
        result.pushKV("result", "Public key not added to db.");
        result.pushKV("reason", smsg::GetString(rv));
    } else {
        result.pushKV("result", "Public key added to db.");
    }

    return result;
},
    };
}

static RPCHelpMan smsgaddlocaladdress()
{
    return RPCHelpMan{"smsgaddlocaladdress",
                "\nEnable receiving messages on <address>.\n"
                "Key for \"address\" must exist in the wallet.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Address to add."},
                },
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("smsgaddlocaladdress", "\"address\"")
                    + HelpExampleRpc("smsgaddlocaladdress", "\"address\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    std::string addr = request.params[0].get_str();

    UniValue result(UniValue::VOBJ);
    int rv = smsgModule.AddLocalAddress(addr);
    if (rv != 0) {
        result.pushKV("result", "Address not added.");
        result.pushKV("reason", smsg::GetString(rv));
    } else {
        result.pushKV("result", "Receiving messages enabled for address.");
    }

    return result;
},
    };
}

static RPCHelpMan smsgimportprivkey()
{
    return RPCHelpMan{"smsgimportprivkey",
                "\nAdds a private key (as returned by dumpprivkey) to the SMSG database.\n"
                "Keys imported into SMSG will be stored unencrypted and can receive messages even if the wallet is locked.\n",
                {
                    {"privkey", RPCArg::Type::STR, RPCArg::Optional::NO, "The private key to import (see dumpprivkey)."},
                    {"label", RPCArg::Type::STR, /* default */ "", "An optional label."},
                },
                RPCResults{},
                RPCExamples{
            "\nDump a private key\n"
            + HelpExampleCli("dumpprivkey", "\"myaddress\"") +
            "\nImport the private key\n"
            + HelpExampleCli("smsgimportprivkey", "\"mykey\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("smsgimportprivkey", "\"mykey\", \"testing\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    std::string strLabel = "";
    if (!request.params[1].isNull()) {
        strLabel = request.params[1].get_str();
    }

    int rv = smsgModule.ImportPrivkey(request.params[0].get_str(), strLabel);
    if (0 != rv) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Import failed.");
    }

    return NullUniValue;
},
    };
}

static RPCHelpMan smsgdumpprivkey()
{
    return RPCHelpMan{"smsgdumpprivkey",
        "\nReveals the private key corresponding to 'address'.\n",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The crown address for the private key"},
        },
        RPCResult{
            RPCResult::Type::STR, "key", "The private key"
        },
        RPCExamples{
            HelpExampleCli("dumpprivkey", "\"myaddress\"")
    + HelpExampleCli("smsgimportprivkey", "\"mykey\"")
    + HelpExampleRpc("smsgdumpprivkey", "\"myaddress\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string strAddress = request.params[0].get_str();
    CTxDestination dest = DecodeDestination(strAddress);
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Hemis address");
    }

    std::vector<std::vector<unsigned char> > vSolutions;
    TxoutType whichType = Solver(GetScriptForDestination(dest), vSolutions);

    if (whichType != TxoutType::WITNESS_V0_KEYHASH  && whichType != TxoutType::PUBKEYHASH) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Address not a key id");
    }

    const CKeyID &idk = ToKeyID(std::get<PKHash>(dest));

    CKey key_out;
    int rv = smsgModule.DumpPrivkey(idk, key_out);
    if (0 != rv) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Private key for address " + strAddress + " is not known");
    }

    return EncodeSecret(key_out);
},
    };
}

static RPCHelpMan smsggetpubkey()
{
    return RPCHelpMan{"smsggetpubkey",
                "\nReturn the base58 encoded compressed public key for an address.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Return the pubkey matching \"address\"."},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "", {
                        {RPCResult::Type::STR, "address", "address of public key"},
                        {RPCResult::Type::STR_HEX, "publickey", "public key of address"},
                    },
                },
                RPCExamples{
            HelpExampleCli("smsggetpubkey", "\"myaddress\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("smsggetpubkey", "\"myaddress\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    std::string address = request.params[0].get_str();
    std::string publicKey;

    UniValue result(UniValue::VOBJ);
    int rv = smsgModule.GetLocalPublicKey(address, publicKey);
    switch (rv) {
        case smsg::SMSG_NO_ERROR:
            result.pushKV("address", address);
            result.pushKV("publickey", publicKey);
            return result; // success, don't check db
        case smsg::SMSG_WALLET_NO_PUBKEY:
            break; // check db
        //case 1:
        default:
            throw JSONRPCError(RPC_INTERNAL_ERROR, smsg::GetString(rv));
    }

    CTxDestination dest = DecodeDestination(address);
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address.");
    }
    CKeyID keyID = ToKeyID(std::get<PKHash>(dest));

    CPubKey cpkFromDB;
    rv = smsgModule.GetStoredKey(keyID, cpkFromDB);

    switch (rv) {
        case smsg::SMSG_NO_ERROR:
            if (!cpkFromDB.IsValid()
                || !cpkFromDB.IsCompressed()) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Invalid public key.");
            } else {
                publicKey = EncodeBase58(cpkFromDB);

                result.pushKV("address", address);
                result.pushKV("publickey", publicKey);
            }
            break;
        case smsg::SMSG_PUBKEY_NOT_EXISTS:
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Address not found in wallet or db.");
        default:
            throw JSONRPCError(RPC_INTERNAL_ERROR, smsg::GetString(rv));
    }

    return result;
},
    };
}

static RPCHelpMan smsgsend()
{
    return RPCHelpMan{"smsgsend",
                "\nSend an encrypted message from \"address_from\" to \"address_to\".\n",
                {
                    {"address_from", RPCArg::Type::STR, RPCArg::Optional::NO, "The address of the sender."},
                    {"address_to", RPCArg::Type::STR, RPCArg::Optional::NO, "The address of the recipient."},
                    {"message", RPCArg::Type::STR, RPCArg::Optional::NO, "The message to send."},
                    {"paid_msg", RPCArg::Type::BOOL, /* default */ "false", "Send as paid message."},
                    {"days_retention", RPCArg::Type::NUM, /* default */ "1", "No. of days for which the message will be retained by network."},
                    {"testfee", RPCArg::Type::BOOL, /* default */ "false", "Don't send the message, only estimate the fee."},
                    {"options", RPCArg::Type::OBJ, /* default */ "", "",
                        {
                            {"fromfile", RPCArg::Type::BOOL, /* default */ "false", "Send file as message, path specified in \"message\"."},
                            {"decodehex", RPCArg::Type::BOOL, /* default */ "false", "Decode \"message\" from hex before sending."},
                            {"submitmsg", RPCArg::Type::BOOL, /* default */ "true", "Submit smsg to network, if false POW is not set and hex encoded smsg returned."},
                            {"savemsg", RPCArg::Type::BOOL, /* default */ "true", "Save smsg to outbox."},
                            {"ttl_is_seconds", RPCArg::Type::BOOL, /* default */ "false", "If true days_retention parameter is interpreted as seconds to live."},
                        },
                        "options"},
                    {"coin_control", RPCArg::Type::OBJ, /* default */ "", "",
                        {
                            {"changeaddress", RPCArg::Type::STR, /* default */ "", "The crown address to receive the change"},
                            {"inputs", RPCArg::Type::ARR, /* default */ "", "A json array of json objects",
                                {
                                    {"", RPCArg::Type::OBJ, /* default */ "", "",
                                        {
                                            {"tx", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "txn id"},
                                            {"n", RPCArg::Type::NUM, RPCArg::Optional::NO, "txn vout"},
                                        },
                                    },
                                },
                            },
                            {"replaceable", RPCArg::Type::BOOL, /* default */ "", "Marks this transaction as BIP125 replaceable.\n"
                            "                              Allows this transaction to be replaced by a transaction with higher fees"},
                            {"conf_target", RPCArg::Type::NUM, /* default */ "", "Confirmation target (in blocks)"},
                            {"estimate_mode", RPCArg::Type::STR, /* default */ "UNSET", "The fee estimate mode, must be one of:\n"
                            "         \"UNSET\"\n"
                            "         \"ECONOMICAL\"\n"
                            "         \"CONSERVATIVE\""},
                            {"avoid_reuse", RPCArg::Type::BOOL, /* default */ "true", "(only available if avoid_reuse wallet flag is set) Avoid spending from dirty addresses; addresses are considered\n"
                            "                             dirty if they have previously been used in a transaction."},
                            {"feeRate", RPCArg::Type::AMOUNT, /* default */ "not set: makes wallet determine the fee", "Set a specific fee rate in " + CURRENCY_UNIT + "/kB"},
                        },
                    },
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "", {
                        {RPCResult::Type::STR, "result", "\"Sent\"/\"Not Sent\""},
                        {RPCResult::Type::STR_HEX, "msgid", "Message id, if sent"},
                        {RPCResult::Type::STR_HEX, "txid", "txnid of the funding txn, if paid msg"},
                        {RPCResult::Type::STR_AMOUNT, "fee", "fee paid, if paid msg"},
                }},
                RPCExamples{
             HelpExampleCli("smsgsend", "\"myaddress\" \"toaddress\" \"message\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("smsgsend", "\"myaddress\", \"toaddress\", \"message\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    RPCTypeCheck(request.params,
        {UniValue::VSTR, UniValue::VSTR, UniValue::VSTR,
         UniValue::VBOOL, UniValue::VNUM, UniValue::VBOOL, UniValue::VOBJ}, true);

    std::string idFrom  = request.params[0].get_str();
    std::string idTo    = request.params[1].get_str();

    std::string addrFrom  = request.params[0].get_str();
    std::string addrTo    = request.params[1].get_str();
    std::string msg       = request.params[2].get_str();

    bool fPaid = request.params[3].isNull() ? false : request.params[3].get_bool();
    int nRetention = request.params[4].isNull() ? 1 : request.params[4].get_int();
    bool fTestFee = request.params[5].isNull() ? false : request.params[5].get_bool();

    bool fFromFile = false;
    bool fDecodeHex = false;
    bool submit_msg = true;
    bool save_msg = true;
    bool ttl_in_seconds = false;

    UniValue options = request.params[6];
    if (options.isObject()) {
        RPCTypeCheckObj(options,
        {
            {"fromfile",          UniValueType(UniValue::VBOOL)},
            {"decodehex",         UniValueType(UniValue::VBOOL)},
            {"submitmsg",         UniValueType(UniValue::VBOOL)},
            {"savemsg",           UniValueType(UniValue::VBOOL)},
            {"ttl_is_seconds",    UniValueType(UniValue::VBOOL)},
        }, true, false);
        if (!options["fromfile"].isNull()) {
            fFromFile = options["fromfile"].get_bool();
        }
        if (!options["decodehex"].isNull()) {
            fDecodeHex = options["decodehex"].get_bool();
        }
        if (!options["submitmsg"].isNull()) {
            submit_msg = options["submitmsg"].get_bool();
        }
        if (!options["savemsg"].isNull()) {
            save_msg = options["savemsg"].get_bool();
        }
        if (!options["ttl_is_seconds"].isNull()) {
            ttl_in_seconds = options["ttl_is_seconds"].get_bool();
        }
    }

    if (fFromFile && fDecodeHex) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Can't use decodehex with fromfile.");
    }

    if (fDecodeHex) {
        if (!IsHex(msg)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Expect hex encoded message with decodehex.");
        }
        std::vector<uint8_t> vData = ParseHex(msg);
        msg = std::string(vData.begin(), vData.end());
    }

    CAmount nFee = 0;
    size_t nTxBytes = 0;

    if (fPaid && Params().GetConsensus().nPaidSmsgTime > GetTime()) {
        throw std::runtime_error("Paid SMSG not yet active on mainnet.");
    }

    CTxDestination destFrom = DecodeDestination(addrFrom);
    if (!IsValidDestination(destFrom)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address.");
    }
    CKeyID kiFrom = ToKeyID(std::get<PKHash>(destFrom));

    CTxDestination destTo = DecodeDestination(addrTo);
    if (!IsValidDestination(destTo)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address.");
    }
    CKeyID kiTo = ToKeyID(std::get<PKHash>(destTo));

    if (!ttl_in_seconds) {
        nRetention *= smsg::SMSG_SECONDS_IN_DAY;
    }

    UniValue result(UniValue::VOBJ);
    std::string sError;
    smsg::SecureMessage smsgOut;

#ifdef ENABLE_WALLET
    CCoinControl cctl;
    if (fPaid) {
        if (!smsgModule.pactive_wallet) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Active wallet must be set to send a paid smsg.");
        }
        CWallet *const pw = smsgModule.pactive_wallet.get();
        if (!fTestFee) {
            EnsureWalletIsUnlocked(pw);
        }
        UniValue uv_cctl = request.params[7];
        if (uv_cctl.isObject()) {
            ParseCoinControlOptions(uv_cctl, pw, cctl);
        }
    }
    if (smsgModule.Send(kiFrom, kiTo, msg, smsgOut, sError, fPaid, nRetention, fTestFee, &nFee, &nTxBytes,
                        fFromFile, submit_msg, save_msg, &cctl) != 0) {
#else
    if (smsgModule.Send(kiFrom, kiTo, msg, smsgOut, sError, fPaid, nRetention, fTestFee, &nFee, &nTxBytes,
                        fFromFile, submit_msg, save_msg) != 0) {
#endif
        result.pushKV("result", "Send failed.");
        result.pushKV("error", sError);
    } else {
        result.pushKV("result", (!submit_msg || fTestFee) ? "Not Sent." : "Sent.");

        if (!fTestFee) {
            result.pushKV("msgid", HexStr(smsgModule.GetMsgID(smsgOut)));
        }

        if (!submit_msg) {
            unsigned char header_buffer[smsg::SMSG_HDR_LEN];
            smsgOut.WriteHeader(header_buffer);
            result.pushKV("msg", HexStr(Span<const unsigned char>(header_buffer, smsg::SMSG_HDR_LEN)) +
                                 HexStr(Span<const unsigned char>(smsgOut.pPayload, smsgOut.nPayload)));
        }

        if (fPaid) {
            if (!fTestFee) {
                uint256 txid;
                smsgOut.GetFundingTxid(txid);
                result.pushKV("txid", txid.ToString());
            }
            result.pushKV("fee", ValueFromAmount(nFee));
            result.pushKV("tx_bytes", (int)nTxBytes);
        }
    }

    return result;
},
    };
}

static RPCHelpMan smsginbox()
{
    return RPCHelpMan{"smsginbox",
                "\nDecrypt and display received messages.\n"
                "Warning: clear will delete all messages.\n",
                {
                    {"mode", RPCArg::Type::STR, /* default */ "unread", "\"all|unread|clear\" List all messages, unread messages or clear all messages."},
                    {"filter", RPCArg::Type::STR, /* default */ "", "Filter messages when in list mode. Applied to from, to and text fields."},
                    {"options", RPCArg::Type::OBJ, /* default */ "", "",
                        {
                            {"updatestatus", RPCArg::Type::BOOL, /* default */ "true", "Update read status if true."},
                            {"encoding", RPCArg::Type::STR, /* default */ "text", "Display message data in encoding, values: \"text\", \"hex\", \"none\"."},
                        },
                        "options"},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "", {
                        {RPCResult::Type::STR_HEX, "msgid", "Message id"},
                        {RPCResult::Type::STR, "version", "The message version"},
                        {RPCResult::Type::STR, "received", "Time the message was received"},
                        {RPCResult::Type::STR, "sent", "Time the message was sent"},
                        {RPCResult::Type::NUM, "daysretention", "DEPRECATED Number of days message will stay in the network for"},
                        {RPCResult::Type::NUM, "ttl", "Seconds message will stay in the network for"},
                        {RPCResult::Type::STR, "from", "Address the message was sent from"},
                        {RPCResult::Type::STR, "to", "Address the message was sent to"},
                        {RPCResult::Type::STR, "text", "Message text"},
                }},
                RPCExamples{
                    "Display unread received messages:"
                    + HelpExampleCli("smsginbox", "") +
                    "Display all received messages that match \"address\":"
                    + HelpExampleCli("smsginbox", "\"all\" \"address\"")
                    + HelpExampleRpc("smsginbox", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VSTR, UniValue::VOBJ}, true);

    std::string mode = request.params[0].isStr() ? request.params[0].get_str() : "unread";
    std::string filter = request.params[1].isStr() ? request.params[1].get_str() : "";

    std::string sEnc = "text";
    bool update_status = true;
    if (request.params[2].isObject()) {
        UniValue options = request.params[2].get_obj();
        if (options["updatestatus"].isBool()) {
            update_status = options["updatestatus"].get_bool();
        }
        if (options["encoding"].isStr()) {
            sEnc = options["encoding"].get_str();
        }
    }

    UniValue result(UniValue::VOBJ);

    {

        smsg::SecMsgDB dbInbox;
        if (!dbInbox.Open("cr+")) {
            throw std::runtime_error("Could not open DB.");
        }

        uint32_t nMessages = 0;
        uint8_t chKey[30];

        if (mode == "clear") {
            LOCK(smsg::cs_smsgDB);
            dbInbox.TxnBegin();

            leveldb::Iterator *it = dbInbox.pdb->NewIterator(leveldb::ReadOptions());
            while (dbInbox.NextSmesgKey(it, smsg::DBK_INBOX, chKey)) {
                dbInbox.EraseSmesg(chKey);
                nMessages++;
            }
            delete it;
            dbInbox.TxnCommit();

            result.pushKV("result", strprintf("Deleted %u messages.", nMessages));
        } else
        if (mode == "all" || mode == "unread") {
            int fCheckReadStatus = mode == "unread" ? 1 : 0;

            smsg::SecMsgStored smsgStored;
            smsg::MessageData msg;

            dbInbox.TxnBegin();

            leveldb::Iterator *it = dbInbox.pdb->NewIterator(leveldb::ReadOptions());
            UniValue messageList(UniValue::VARR);

            while (dbInbox.NextSmesg(it, smsg::DBK_INBOX, chKey, smsgStored)) {
                if (fCheckReadStatus
                    && !(smsgStored.status & SMSG_MASK_UNREAD)) {
                    continue;
                }
                const unsigned char *pHeader = smsgStored.vchMessage.data();
                smsg::SecureMessage smsg(pHeader);
                const smsg::SecureMessage *psmsg = &smsg;

                UniValue objM(UniValue::VOBJ);
                objM.pushKV("msgid", HexStr(Span<const unsigned char>(&chKey[2], 28))); // timestamp+hash
                objM.pushKV("version", strprintf("%02x%02x", psmsg->version[0], psmsg->version[1]));

                uint32_t nPayload = smsgStored.vchMessage.size() - smsg::SMSG_HDR_LEN;
                int rv = smsgModule.Decrypt(false, smsgStored.addrTo, pHeader, pHeader + smsg::SMSG_HDR_LEN, nPayload, msg);
                if (rv == 0) {
                    std::string sAddrTo = EncodeDestination(PKHash(smsgStored.addrTo));
                    std::string sText = std::string((char*)msg.vchMessage.data());
                    if (filter.size() > 0
                        && !(part::stringsMatchI(msg.sFromAddress, filter, 3) ||
                            part::stringsMatchI(sAddrTo, filter, 3) ||
                            part::stringsMatchI(sText, filter, 3))) {
                        continue;
                    }

                    PushTime(objM, "received", smsgStored.timeReceived);
                    PushTime(objM, "sent", msg.timestamp);
                    objM.pushKV("paid", UniValue(psmsg->IsPaidVersion()));

                    int64_t ttl = psmsg->m_ttl;
                    objM.pushKV("ttl", ttl);
                    int nDaysRetention = ttl / smsg::SMSG_SECONDS_IN_DAY;
                    objM.pushKV("daysretention", nDaysRetention);
                    PushTime(objM, "expiration", psmsg->timestamp + ttl);

                    uint32_t nPayload = smsgStored.vchMessage.size() - smsg::SMSG_HDR_LEN;
                    objM.pushKV("payloadsize", (int)nPayload);

                    objM.pushKV("from", msg.sFromAddress);
                    objM.pushKV("to", sAddrTo);
                    if (sEnc == "none") {
                    } else
                    if (sEnc == "text") {
                        
                        UniValue valRequest;
						if (valRequest.read(dequote(sText)) && valRequest.isObject()){
                            UniValue message = valRequest.get_obj();
                            objM.pushKV("json", message);
					    }
					    else
					        objM.pushKV("text", sText);
					    
                    } else
                    if (sEnc == "hex") {
                        objM.pushKV("hex", HexStr(msg.vchMessage));
                    } else
                    if (sEnc == "json") {
						UniValue message (UniValue::VType::VARR, std::string((char*)msg.vchMessage.data()));
                        objM.pushKV("json", message);
                    } else {
                        objM.pushKV("unknown_encoding", sEnc);
                    }
                } else {
                    if (filter.size() > 0) {
                        continue;
                    }

                    objM.pushKV("status", "Decrypt failed");
                    objM.pushKV("error", smsg::GetString(rv));
                }

                messageList.push_back(objM);

                // Only set 'read' status if the message decrypted successfully and update_status is set
                if (fCheckReadStatus && rv == 0 && update_status) {
                    smsgStored.status &= ~SMSG_MASK_UNREAD;
                    dbInbox.WriteSmesg(chKey, smsgStored);
                }
                nMessages++;
            }
            delete it;
            dbInbox.TxnCommit();

            result.pushKV("messages", messageList);
            result.pushKV("result", strprintf("%u", nMessages));
        } else {
            result.pushKV("result", "Unknown Mode.");
            result.pushKV("expected", "all|unread|clear.");
        }
    } // cs_smsgDB

    return result;
},
    };
};

static RPCHelpMan smsgoutbox()
{
    return RPCHelpMan{"smsgoutbox",
                "\nDecrypt and display all sent messages.\n"
                "Warning: \"mode\"=\"clear\" will delete all sent messages.\n",
                {
                    {"mode", RPCArg::Type::STR, /* default */ "all", "all|clear, List or clear messages."},
                    {"filter", RPCArg::Type::STR, /* default */ "", "Filter messages when in list mode. Applied to from, to and text fields."},
                    {"options", RPCArg::Type::OBJ, /* default */ "", "",
                        {
                            {"encoding", RPCArg::Type::STR, /* default */ "text", "Display message data in encoding, values: \"text\", \"hex\", \"none\"."},
                            {"sending", RPCArg::Type::BOOL, /* default */ "false", "Display messages in sending queue."},
                        },
                        "options"},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "", {
                        {RPCResult::Type::STR_HEX, "msgid", "Message id"},
                        {RPCResult::Type::STR, "version", "The message version"},
                        {RPCResult::Type::STR, "sent", "Time the message was sent"},
                        {RPCResult::Type::STR, "from", "Address the message was sent from"},
                        {RPCResult::Type::STR, "to", "Address the message was sent to"},
                        {RPCResult::Type::STR, "text", "Message text"},
                }},
                RPCExamples{
                    HelpExampleCli("smsgoutbox", "")
                    + HelpExampleRpc("smsgoutbox", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VSTR}, true);

    std::string mode = request.params[0].isStr() ? request.params[0].get_str() : "all";
    std::string filter = request.params[1].isStr() ? request.params[1].get_str() : "";

    bool show_sending = false;
    std::string sEnc = "text";
    if (request.params[2].isObject()) {
        UniValue options = request.params[2].get_obj();
        if (options["encoding"].isStr()) {
            sEnc = options["encoding"].get_str();
        }
        if (options["sending"].isBool()) {
            show_sending = options["sending"].get_bool();
        }
    }

    UniValue result(UniValue::VOBJ);

    uint8_t chKey[30];
    memset(&chKey[0], 0, sizeof(chKey));

    {
        LOCK(smsg::cs_smsgDB);

        smsg::SecMsgDB dbOutbox;
        if (!dbOutbox.Open("cr+")) {
            throw std::runtime_error("Could not open DB.");
        }

        uint32_t nMessages = 0;

        std::string db_prefix = show_sending ? smsg::DBK_QUEUED : smsg::DBK_OUTBOX;
        if (mode == "clear") {
            dbOutbox.TxnBegin();

            leveldb::Iterator *it = dbOutbox.pdb->NewIterator(leveldb::ReadOptions());
            while (dbOutbox.NextSmesgKey(it, db_prefix, chKey)) {
                dbOutbox.EraseSmesg(chKey);
                nMessages++;
            }
            delete it;
            dbOutbox.TxnCommit();

            result.pushKV("result", strprintf("Deleted %u messages.", nMessages));
        } else
        if (mode == "all") {
            smsg::SecMsgStored smsgStored;
            smsg::MessageData msg;
            leveldb::Iterator *it = dbOutbox.pdb->NewIterator(leveldb::ReadOptions());

            UniValue messageList(UniValue::VARR);

            while (dbOutbox.NextSmesg(it, db_prefix, chKey, smsgStored)) {
                const unsigned char *pHeader = smsgStored.vchMessage.data();
                smsg::SecureMessage smsg(pHeader);
                const smsg::SecureMessage *psmsg = &smsg;

                UniValue objM(UniValue::VOBJ);
                objM.pushKV("msgid", HexStr(Span<const unsigned char>(&chKey[2], 28))); // timestamp+hash
                objM.pushKV("version", strprintf("%02x%02x", psmsg->version[0], psmsg->version[1]));

                uint32_t nPayload = smsgStored.vchMessage.size() - smsg::SMSG_HDR_LEN;
                int rv = smsgModule.Decrypt(false, smsgStored.addrOutbox, pHeader, pHeader + smsg::SMSG_HDR_LEN, nPayload, msg);
                if (rv == 0) {
                    std::string sAddrTo = EncodeDestination(PKHash(smsgStored.addrTo));
                    std::string sText = std::string((char*)msg.vchMessage.data());
                    if (filter.size() > 0
                        && !(part::stringsMatchI(msg.sFromAddress, filter, 3) ||
                            part::stringsMatchI(sAddrTo, filter, 3) ||
                            part::stringsMatchI(sText, filter, 3))) {
                        continue;
                    }

                    PushTime(objM, "sent", msg.timestamp);
                    objM.pushKV("paid", UniValue(psmsg->IsPaidVersion()));

                    int64_t ttl = psmsg->m_ttl;
                    objM.pushKV("ttl", ttl);
                    int nDaysRetention = ttl / smsg::SMSG_SECONDS_IN_DAY;
                    objM.pushKV("daysretention", nDaysRetention);
                    PushTime(objM, "expiration", psmsg->timestamp + ttl);

                    uint32_t nPayload = smsgStored.vchMessage.size() - smsg::SMSG_HDR_LEN;
                    objM.pushKV("payloadsize", (int)nPayload);

                    objM.pushKV("from", msg.sFromAddress);
                    objM.pushKV("to", sAddrTo);
                    if (sEnc == "none") {
                    } else
                    if (sEnc == "text") {
                        
                        UniValue valRequest;
						if (valRequest.read(dequote(sText)) && valRequest.isObject()){
                            UniValue message = valRequest.get_obj();
                            objM.pushKV("json", message);
					    }
					    else
					        objM.pushKV("text", sText);
                    } else
                    if (sEnc == "hex") {
                        objM.pushKV("hex", HexStr(msg.vchMessage));
                    } else {
                        objM.pushKV("unknown_encoding", sEnc);
                    }
                } else {
                    if (filter.size() > 0) {
                        continue;
                    }

                    objM.pushKV("status", "Decrypt failed");
                    objM.pushKV("error", smsg::GetString(rv));
                }
                messageList.push_back(objM);
                nMessages++;
            }
            delete it;

            result.pushKV("messages" ,messageList);
            result.pushKV("result", strprintf("%u", nMessages));
        } else {
            result.pushKV("result", "Unknown Mode.");
            result.pushKV("expected", "all|clear.");
        }
    }

    return result;
},
    };
};

static RPCHelpMan smsgbuckets()
{
    return RPCHelpMan{"smsgbuckets",
                "\nDisplay message bucket information.\n",
                {
                    {"mode", RPCArg::Type::STR, /* default */ "stats", "stats|total|dump. \"dump\" will remove all buckets."},
                },
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("smsgbuckets", "")
                    + HelpExampleRpc("smsgbuckets", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    std::string mode = "stats";
    if (request.params.size() > 0) {
        mode = request.params[0].get_str();
    }

    UniValue result(UniValue::VOBJ);
    UniValue arrBuckets(UniValue::VARR);

    char cbuf[256];
    if (mode == "stats" || mode == "total") {
        bool show_buckets = mode != "total" ? true : false;
        uint32_t nBuckets = 0;
        uint32_t nMessages = 0;
        uint64_t nBytes = 0;
        {
            LOCK(smsgModule.cs_smsg);
            std::map<int64_t, smsg::SecMsgBucket>::const_iterator it;
            for (it = smsgModule.buckets.begin(); it != smsgModule.buckets.end(); ++it) {
                const std::set<smsg::SecMsgToken> &tokenSet = it->second.setTokens;

                std::string sBucket = ToString(it->first);
                std::string sFile = sBucket + "_01.dat";
                std::string sHash = ToString((int64_t)it->second.hash);

                size_t nActiveMessages = it->second.CountActive();

                nBuckets++;
                nMessages += nActiveMessages;

                UniValue objM(UniValue::VOBJ);
                if (show_buckets) {
                    objM.pushKV("bucket", sBucket);
                    PushTime(objM, "time", it->first);
                    objM.pushKV("no. messages", strprintf("%u", tokenSet.size()));
                    objM.pushKV("active messages", strprintf("%u", nActiveMessages));
                    objM.pushKV("hash", sHash);
                    objM.pushKV("last changed", part::GetTimeString(it->second.timeChanged, cbuf, sizeof(cbuf)));
                }

                fs::path fullPath = gArgs.GetDataDirNet() / fs::PathFromString(smsg::STORE_DIR) / fs::PathFromString(sFile);
                if (!fs::exists(fullPath)) {
                    if (tokenSet.size() == 0) {
                        objM.pushKV("file size", "Empty bucket.");
                    } else {
                        objM.pushKV("file size, error", "File not found.");
                    }
                } else {
                    try {
                        uint64_t nFBytes = 0;
                        nFBytes = fs::file_size(fullPath);
                        nBytes += nFBytes;
                        if (show_buckets) {
                            objM.pushKV("file size", part::BytesReadable(nFBytes));
                        }
                    } catch (const fs::filesystem_error& ex) {
                        objM.pushKV("file size, error", ex.what());
                    }
                }
                if (objM.size() > 0) {
                    arrBuckets.push_back(objM);
                }
            }
        } // cs_smsg

        UniValue objM(UniValue::VOBJ);
        objM.pushKV("numbuckets", (int)nBuckets);
        objM.pushKV("numpurged", (int)smsgModule.setPurged.size());
        objM.pushKV("messages", (int)nMessages);
        objM.pushKV("size", part::BytesReadable(nBytes));
        if (arrBuckets.size() > 0) {
            result.pushKV("buckets", arrBuckets);
        }
        result.pushKV("total", objM);
    } else
    if (mode == "dump") {
        {
            LOCK(smsgModule.cs_smsg);
            std::map<int64_t, smsg::SecMsgBucket>::iterator it;
            for (it = smsgModule.buckets.begin(); it != smsgModule.buckets.end(); ++it) {
                std::string sFile = ToString(it->first) + "_01.dat";

                try {
                    fs::path fullPath = gArgs.GetDataDirNet() / fs::PathFromString(smsg::STORE_DIR) / fs::PathFromString(sFile);
                    fs::remove(fullPath);
                } catch (const fs::filesystem_error& ex) {
                    //objM.push_back(Pair("file size, error", ex.what()));
                    LogPrintf("Error removing bucket file %s.\n", ex.what());
                }
            }
            smsgModule.buckets.clear();
            smsgModule.start_time = GetAdjustedTime();
        } // cs_smsg

        result.pushKV("result", "Removed all buckets.");
    } else {
        result.pushKV("result", "Unknown Mode.");
        result.pushKV("expected", "stats|total|dump.");
    }

    return result;
},
    };
};

static bool sortMsgAsc(const std::pair<int64_t, UniValue> &a, const std::pair<int64_t, UniValue> &b)
{
    return a.first < b.first;
};

static bool sortMsgDesc(const std::pair<int64_t, UniValue> &a, const std::pair<int64_t, UniValue> &b)
{
    return a.first > b.first;
};

static RPCHelpMan smsgview()
{
    return RPCHelpMan{"smsgview",
                "\nView messages by address.\n"
                "Setting address to '*' will match all addresses\n"
                "'abc*' will match addresses with labels beginning 'abc'\n"
                "'*abc' will match addresses with labels ending 'abc'\n"
                "Full date/time format for from and to is yyyy-mm-ddThh:mm:ss\n"
                "From and to will accept incomplete inputs like: -from 2016\n",
                {
                    {"arg1", RPCArg::Type::STR, /* default */ "*", "address/label"},
                    {"arg2", RPCArg::Type::STR, /* default */ "asc", "asc/desc"},
                    {"arg3", RPCArg::Type::STR, /* default */ "", "-from yyyy-mm-dd"},
                    {"arg4", RPCArg::Type::STR, /* default */ "", "-to yyyy-mm-dd"},
                },
                RPCResults{},
                RPCExamples{
                    HelpExampleCli("smsgview", "")
                    + HelpExampleRpc("smsgview", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

#ifdef ENABLE_WALLET

    char cbuf[256];
    bool fMatchAll = false;
    bool fDesc = false;
    int64_t tFrom = 0, tTo = 0;
    std::vector<CKeyID> vMatchAddress;
    std::string sTemp;

    if (request.params.size() > 0) {
        sTemp = request.params[0].get_str();

        // Blank address or "*" will match all
        if (sTemp.length() < 1) { // Error instead?
            fMatchAll = true;
        } else
        if (sTemp.length() == 1 && sTemp[0] == '*') {
            fMatchAll = true;
        }

        if (!fMatchAll) {
            CTxDestination dest = DecodeDestination(sTemp);

            if (IsValidDestination(dest)) {
                CKeyID ki = ToKeyID(std::get<PKHash>(dest));
                vMatchAddress.push_back(ki);
            } else {
                // Lookup address by label, can match multiple addresses

                // TODO: Use Boost.Regex?
                int matchType = 0; // 0 full match, 1 startswith, 2 endswith
                if (sTemp[0] == '*') {
                    matchType = 1;
                    sTemp.erase(0, 1);
                } else
                if (sTemp[sTemp.length()-1] == '*') {
                    matchType = 2;
                    sTemp.erase(sTemp.length()-1, 1);
                }

                std::map<CTxDestination, CAddressBookData>::iterator itl;

                for (const auto &pw : smsgModule.m_vpwallets) {
                    LOCK(pw->cs_wallet);
                    for (itl = pw->m_address_book.begin(); itl != pw->m_address_book.end(); ++itl) {
                        if (part::stringsMatchI(itl->second.GetLabel(), sTemp, matchType)) {
                            if (IsValidDestination(itl->first)) {
                                CKeyID ki = ToKeyID(std::get<PKHash>(dest));
                                vMatchAddress.push_back(ki);
                            } else {
                                LogPrintf("Warning: matched invalid address: %s\n", EncodeDestination(itl->first));
                            }
                        }
                    }
                }
            }
        }
    } else {
        fMatchAll = true;
    }

    size_t i = 1;
    while (i < request.params.size()) {
        sTemp = request.params[i].get_str();
        if (sTemp == "-from") {
            if (i >= request.params.size()-1) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Argument required for: " + sTemp);
            }
            i++;
            sTemp = request.params[i].get_str();
            tFrom = part::strToEpoch(sTemp.c_str());
            if (tFrom < 0) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "from format error: " + std::string(strerror(errno)));
            }
        } else
        if (sTemp == "-to") {
            if (i >= request.params.size()-1) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Argument required for: " + sTemp);
            }
            i++;
            sTemp = request.params[i].get_str();
            tTo = part::strToEpoch(sTemp.c_str());
            if (tTo < 0) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "to format error: " + std::string(strerror(errno)));
            }
        } else
        if (sTemp == "asc") {
            fDesc = false;
        } else
        if (sTemp == "desc") {
            fDesc = true;
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown parameter: " + sTemp);
        }

        i++;
    }

    if (!fMatchAll && vMatchAddress.size() < 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "No address found.");
    }

    UniValue result(UniValue::VOBJ);

    std::map<CKeyID, std::string> mLabelCache;
    std::vector<std::pair<int64_t, UniValue> > vMessages;

    std::vector<std::string> vPrefixes;
    vPrefixes.push_back(smsg::DBK_INBOX);
    vPrefixes.push_back(smsg::DBK_OUTBOX);

    uint8_t chKey[30];
    size_t nMessages = 0;
    UniValue messageList(UniValue::VARR);

    size_t debugEmptySent = 0;

    {
        LOCK(smsg::cs_smsgDB);
        smsg::SecMsgDB dbMsg;
        if (!dbMsg.Open("cr")) {
            throw std::runtime_error("Could not open DB.");
        }

        std::vector<std::string>::iterator itp;
        std::vector<CKeyID>::iterator its;
        for (itp = vPrefixes.begin(); itp < vPrefixes.end(); ++itp) {
            bool fInbox = *itp == smsg::DBK_INBOX;

            dbMsg.TxnBegin();

            leveldb::Iterator *it = dbMsg.pdb->NewIterator(leveldb::ReadOptions());
            smsg::SecMsgStored smsgStored;
            smsg::MessageData msg;

            while (dbMsg.NextSmesg(it, *itp, chKey, smsgStored)) {
                if (!fInbox && smsgStored.addrOutbox.IsNull()) {
                    debugEmptySent++;
                    continue;
                }

                uint32_t nPayload = smsgStored.vchMessage.size() - smsg::SMSG_HDR_LEN;
                int rv;
                if ((rv = smsgModule.Decrypt(false, fInbox ? smsgStored.addrTo : smsgStored.addrOutbox,
                    smsgStored.vchMessage.data(), &smsgStored.vchMessage[smsg::SMSG_HDR_LEN], nPayload, msg)) == 0) {
                    if ((tFrom > 0 && msg.timestamp < tFrom)
                        || (tTo > 0 && msg.timestamp > tTo)) {
                        continue;
                    }

                    CTxDestination dest = DecodeDestination(msg.sFromAddress);
                    if (!IsValidDestination(dest)) {
                        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address.");
                    }
                    CKeyID kiFrom = ToKeyID(std::get<PKHash>(dest));

                    if (!fMatchAll) {
                        bool fSkip = true;

                        for (its = vMatchAddress.begin(); its < vMatchAddress.end(); ++its) {
                            if (*its == kiFrom
                                || *its == smsgStored.addrTo) {
                                fSkip = false;
                                break;
                            }
                        }

                        if (fSkip) {
                            continue;
                        }
                    }

                    // Get labels for addresses, cache found labels.
                    std::string lblFrom, lblTo;
                    std::map<CKeyID, std::string>::iterator itl;

                    if ((itl = mLabelCache.find(kiFrom)) != mLabelCache.end()) {
                        lblFrom = itl->second;
                    } else {
                        PKHash wpkh = PKHash(kiFrom);
                        lblFrom = smsgModule.LookupLabel(wpkh);
                        mLabelCache[kiFrom] = lblFrom;
                    }

                    if ((itl = mLabelCache.find(smsgStored.addrTo)) != mLabelCache.end()) {
                        lblTo = itl->second;
                    } else {
                        PKHash wpkh = PKHash(smsgStored.addrTo);
                        lblTo = smsgModule.LookupLabel(wpkh);
                        mLabelCache[smsgStored.addrTo] = lblTo;
                    }

                    std::string sFrom = kiFrom.IsNull() ? "anon" : EncodeDestination(PKHash(kiFrom));
                    std::string sTo = EncodeDestination(PKHash(smsgStored.addrTo));
                    if (lblFrom.length() != 0) {
                        sFrom += " (" + lblFrom + ")";
                    }
                    if (lblTo.length() != 0) {
                        sTo += " (" + lblTo + ")";
                    }

                    UniValue objM(UniValue::VOBJ);
                    PushTime(objM, "sent", msg.timestamp);
                    objM.pushKV("from", sFrom);
                    objM.pushKV("to", sTo);
                    objM.pushKV("text", std::string((char*)&msg.vchMessage[0]));

                    vMessages.push_back(std::make_pair(msg.timestamp, objM));
                } else {
                    LogPrintf("%s: SecureMsgDecrypt failed, %s.\n", __func__, HexStr(Span<const unsigned char>(chKey, 18)));
                }
            }
            delete it;

            dbMsg.TxnCommit();
        }
    } // cs_smsgDB


    std::sort(vMessages.begin(), vMessages.end(), fDesc ? sortMsgDesc : sortMsgAsc);

    std::vector<std::pair<int64_t, UniValue> >::iterator itm;
    for (itm = vMessages.begin(); itm < vMessages.end(); ++itm) {
        messageList.push_back(itm->second);
        nMessages++;
    }

    result.pushKV("messages", messageList);

    if (LogAcceptCategory(BCLog::SMSG)) {
        result.pushKV("debug empty sent", (int)debugEmptySent);
    }

    result.pushKV("result", strprintf("Displayed %u messages.", nMessages));
    if (tFrom > 0) {
        result.pushKV("from", part::GetTimeString(tFrom, cbuf, sizeof(cbuf)));
    }
    if (tTo > 0) {
        result.pushKV("to", part::GetTimeString(tTo, cbuf, sizeof(cbuf)));
    }
#else
    UniValue result(UniValue::VOBJ);
    throw JSONRPCError(RPC_MISC_ERROR, "No wallet.");
#endif
    return result;
},
    };
}

static RPCHelpMan smsgone()
{
    return RPCHelpMan{"smsgone",
                "\nView smsg by msgid.\n",
                {
                    {"msgid", RPCArg::Type::STR, RPCArg::Optional::NO, "Id of the message to view."},
                    {"options", RPCArg::Type::OBJ, /* default */ "", "",
                        {
                            {"delete", RPCArg::Type::BOOL, /* default */ "false", "Delete msg if true."},
                            {"setread", RPCArg::Type::BOOL, /* default */ "false", "Set read status to value."},
                            {"encoding", RPCArg::Type::STR, /* default */ "text", "Display message data in encoding, values: \"text\", \"hex\", \"none\"."},
                            {"export", RPCArg::Type::BOOL, /* default */ "false", "Display the full smsg as a hex encoded string."},
                        },
                        "options"},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "", {
                        {RPCResult::Type::STR_HEX, "msgid", "Message id"},
                        {RPCResult::Type::STR, "version", "The message version"},
                        {RPCResult::Type::STR, "location", "inbox|outbox|sending"},
                        {RPCResult::Type::STR, "received", "Time the message was received"},
                        {RPCResult::Type::BOOL, "read", "Read status"},
                        {RPCResult::Type::STR, "sent", "Time the message was created"},
                        {RPCResult::Type::BOOL, "paid", "Paid or free message"},
                        {RPCResult::Type::NUM, "daysretention", "DEPRECATED Number of days message will stay in the network for"},
                        {RPCResult::Type::NUM, "ttl", "Seconds message will stay in the network for"},
                        {RPCResult::Type::NUM_TIME, "expiration", "Time the message will be dropped from the network"},
                        {RPCResult::Type::NUM, "payloadsize", "Size of user message"},
                        {RPCResult::Type::STR, "from", "Address the message was sent from"},
                }},
                RPCExamples{
            HelpExampleCli("smsgone", "\"msgid\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("smsgone", "\"msgid\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    RPCTypeCheckObj(request.params,
        {
            {"msgid",             UniValueType(UniValue::VSTR)},
            {"options",           UniValueType(UniValue::VOBJ)},
        }, true, false);

    std::string sMsgId = request.params[0].get_str();

    if (!IsHex(sMsgId) || sMsgId.size() != 56) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "msgid must be 28 bytes in hex string.");
    }
    std::vector<uint8_t> vMsgId = ParseHex(sMsgId.c_str());
    std::string sType;

    uint8_t chKey[30];
    chKey[1] = 'M';
    memcpy(chKey+2, vMsgId.data(), 28);
    smsg::SecMsgStored smsgStored;
    UniValue result(UniValue::VOBJ);

    UniValue options = request.params[1];
    {
        LOCK(smsg::cs_smsgDB);
        smsg::SecMsgDB dbMsg;
        if (!dbMsg.Open("cr+"))
            throw std::runtime_error("Could not open DB.");

        if ((chKey[0] = 'I') && dbMsg.ReadSmesg(chKey, smsgStored)) {
            sType = "inbox";
        } else
        if ((chKey[0] = 'S') && dbMsg.ReadSmesg(chKey, smsgStored)) {
            sType = "outbox";
        } else
        if ((chKey[0] = 'Q') && dbMsg.ReadSmesg(chKey, smsgStored)) {
            sType = "sending";
        } else {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Unknown message id.");
        };

        if (options.isObject()) {
            options = request.params[1].get_obj();
            if (options["delete"].isBool() && options["delete"].get_bool() == true) {
                if (!dbMsg.EraseSmesg(chKey)) {
                    throw JSONRPCError(RPC_INTERNAL_ERROR, "EraseSmesg failed.");
                }
                result.pushKV("operation", "Deleted");
            } else {
                // Can't mix delete and other operations
                if (options["setread"].isBool()) {
                    bool nv = options["setread"].get_bool();
                    if (nv) {
                        smsgStored.status &= ~SMSG_MASK_UNREAD;
                    } else {
                        smsgStored.status |= SMSG_MASK_UNREAD;
                    }

                    if (!dbMsg.WriteSmesg(chKey, smsgStored)) {
                        throw JSONRPCError(RPC_INTERNAL_ERROR, "WriteSmesg failed.");
                    }
                    result.pushKV("operation", strprintf("Set read status to: %s", nv ? "true" : "false"));
                }
            }
        }
    }

    smsg::SecureMessage smsg(smsgStored.vchMessage.data());
    const smsg::SecureMessage *psmsg = &smsg;

    result.pushKV("msgid", sMsgId);
    result.pushKV("version", strprintf("%02x%02x", psmsg->version[0], psmsg->version[1]));
    result.pushKV("location", sType);
    PushTime(result, "received", smsgStored.timeReceived);
    result.pushKV("to", EncodeDestination(PKHash(smsgStored.addrTo)));
    result.pushKV("read", UniValue(bool(!(smsgStored.status & SMSG_MASK_UNREAD))));

    PushTime(result, "sent", psmsg->timestamp);
    result.pushKV("paid", UniValue(psmsg->IsPaidVersion()));

    int64_t ttl = psmsg->m_ttl;
    result.pushKV("ttl", ttl);
    int nDaysRetention = ttl / smsg::SMSG_SECONDS_IN_DAY;
    result.pushKV("daysretention", nDaysRetention);
    PushTime(result, "expiration", psmsg->timestamp + ttl);


    smsg::MessageData msg;
    bool fInbox = sType == "inbox" ? true : false;
    uint32_t nPayload = smsgStored.vchMessage.size() - smsg::SMSG_HDR_LEN;
    result.pushKV("payloadsize", (int)nPayload);

    std::string sEnc = "text";
    if (options.isObject() && options["encoding"].isStr()) {
        sEnc = options["encoding"].get_str();
    }

    bool export_smsg = options.isObject() && options["export"].isBool() ? options["export"].get_bool() : false;
    if (export_smsg) {
        result.pushKV("raw", HexStr(smsgStored.vchMessage));
    }

    int rv;
    if ((rv = smsgModule.Decrypt(false, fInbox ? smsgStored.addrTo : smsgStored.addrOutbox,
        smsgStored.vchMessage.data(), &smsgStored.vchMessage[smsg::SMSG_HDR_LEN], nPayload, msg)) == 0) {
        result.pushKV("from", msg.sFromAddress);
        std::string sText = std::string((char*)msg.vchMessage.data());

        if (sEnc == "none") {
        } else
        if (sEnc == "") {
            // TODO: detect non ascii chars
            if (msg.vchMessage.size() < smsg::SMSG_MAX_MSG_BYTES) {
				UniValue valRequest;
				if (valRequest.read(dequote(sText)) && valRequest.isObject()){
					UniValue message = valRequest.get_obj();
					result.pushKV("json", message);
				}
				else
                    result.pushKV("text", std::string((char*)msg.vchMessage.data()));
            } else {
                result.pushKV("hex", HexStr(msg.vchMessage));
            }
        } else
        if (sEnc == "text") {
            
			UniValue valRequest;
			if (valRequest.read(dequote(sText)) && valRequest.isObject()){
				UniValue message = valRequest.get_obj();
				result.pushKV("json", message);
			}
			else
			    result.pushKV("text", sText);
        } else
        if (sEnc == "hex") {
            result.pushKV("hex", HexStr(msg.vchMessage));
        } else {
            result.pushKV("unknown_encoding", sEnc);
        }
    } else {
        result.pushKV("error", "decrypt failed");
    }

    return result;
},
    };
}

static RPCHelpMan smsgimport()
{
    return RPCHelpMan{"smsgimport",
                "\nImport smsg from hex string.\n",
                {
                    {"msg", RPCArg::Type::STR, RPCArg::Optional::NO, "Hex encoded smsg."},
                    {"options", RPCArg::Type::OBJ, /* default */ "", "",
                        {
                            {"submitmsg", RPCArg::Type::BOOL, /* default */ "false", "Submit msg to network if true."},
                            {"setread", RPCArg::Type::BOOL, /* default */ "false", "Set read status to value."},
                        },
                        "options"},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "", {
                        {RPCResult::Type::STR_HEX, "msgid", "The message identifier"},
                }},
                RPCExamples{
            HelpExampleCli("smsgimport", "\"msg\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("smsgimport", "\"msg\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    RPCTypeCheckObj(request.params,
        {
            {"msg",             UniValueType(UniValue::VSTR)},
            {"option",          UniValueType(UniValue::VOBJ)},
        }, true, false);

    std::string str_msg = request.params[0].get_str();

    if (!IsHex(str_msg)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "msg must be a hex string.");
    }

    std::vector<uint8_t> vsmsg = ParseHex(str_msg.c_str());
    smsg::SecureMessage smsg(vsmsg.data());
    smsg.pPayload = vsmsg.data() + smsg::SMSG_HDR_LEN;

    UniValue result(UniValue::VOBJ);
    std::string str_error;
    bool setread = false;
    bool submitmsg = false;
    UniValue options = request.params[1];
    if (options.isObject() && options["setread"].isBool()) {
        setread = options["setread"].get_bool();
    }
    if (options.isObject() && options["submitmsg"].isBool()) {
        submitmsg = options["submitmsg"].get_bool();
    }

    if (smsgModule.Import(&smsg, str_error, setread, submitmsg) != 0) {
        smsg.pPayload = nullptr;
        throw JSONRPCError(RPC_MISC_ERROR, "Import failed: " + str_error);
    }
    result.pushKV("msgid", HexStr(smsgModule.GetMsgID(smsg)));

    smsg.pPayload = nullptr;

    return result;
},
    };
}

static RPCHelpMan smsgpurge()
{
    return RPCHelpMan{"smsgpurge",
                "\nPurge smsg by msgid.\n",
                {
                    {"msgid", RPCArg::Type::STR_HEX, /* default */ "", "Id of the message to purge."},
                },
                RPCResults{},
                RPCExamples{
            HelpExampleCli("smsgpurge", "\"msgid\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("smsgpurge", "\"msgid\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    if (!request.params[0].isStr()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "msgid must be a string.");
    }

    std::string sMsgId = request.params[0].get_str();

    if (!IsHex(sMsgId) || sMsgId.size() != 56) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "msgid must be 28 bytes in hex string.");
    }
    std::vector<uint8_t> vMsgId = ParseHex(sMsgId.c_str());

    std::string sError;
    if (smsg::SMSG_NO_ERROR != smsgModule.Purge(vMsgId, sError)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Error: " + sError);
    }

    return NullUniValue;
},
    };
}

static RPCHelpMan smsggetfeerate()
{
    return RPCHelpMan{"smsggetfeerate",
                "\nReturn paid SMSG fee.\n",
                {
                    {"height", RPCArg::Type::NUM, /* default */ "", "Chain height to get fee rate for, pass a negative number for more detailed output."},
                },
                RPCResult{
                    RPCResult::Type::NUM, "fee_rate", "Fee rate in satoshis"
                },
                RPCExamples{
            HelpExampleCli("smsggetfeerate", "1000") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("smsggetfeerate", "1000")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    LOCK(cs_main);

    CBlockIndex *pblockindex = nullptr;
    if (!request.params[0].isNull()) {
        int nHeight = request.params[0].get_int();

        if (nHeight < 0) {
            UniValue result(UniValue::VOBJ);
            const CBlockIndex *pTip = ::ChainActive().Tip();
            const Consensus::Params &consensusParams = Params().GetConsensus();
            int chain_height = pTip->nHeight;

            result.pushKV("currentrate", GetSmsgFeeRate(nullptr));
            int fee_height = (chain_height / consensusParams.smsg_fee_period) * consensusParams.smsg_fee_period;
            result.pushKV("currentrateblockheight", fee_height);

            int64_t smsg_fee_rate_target =0;
            CBlock block;
            if (!ReadBlockFromDisk(block, pTip, Params().GetConsensus())) {
                throw JSONRPCError(RPC_MISC_ERROR, "Block not found on disk");
            }
            block.vtx[0]->GetSmsgFeeRate(smsg_fee_rate_target);
            result.pushKV("targetrate", smsg_fee_rate_target);
            result.pushKV("targetblockheight", chain_height);
            result.pushKV("nextratechangeheight", int(fee_height + consensusParams.smsg_fee_period));
            return result;
        }
        if (nHeight > ::ChainActive().Height()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");
        }
        pblockindex = ::ChainActive()[nHeight];
    }

    return GetSmsgFeeRate(pblockindex);
},
    };
}

static RPCHelpMan smsggetdifficulty()
{
    return RPCHelpMan{"smsggetdifficulty",
                "\nReturn free SMSG difficulty.\n",
                {
                    {"time", RPCArg::Type::NUM, /* default */ "", "Chain time to get smsg difficulty for."},
                },
                RPCResult{
                    RPCResult::Type::STR, "difficulty", "Current smsg difficulty"
                },
                RPCExamples{
            HelpExampleCli("smsggetdifficulty", "1552688834") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("smsggetdifficulty", "1552688834")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    LOCK(cs_main);

    int64_t chain_time = ::ChainActive().Tip()->nTime;
    if (!request.params[0].isNull()) {
        chain_time = request.params[0].get_int64();
        if (chain_time > ::ChainActive().Tip()->nTime) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Time out of range");
        }
    }

    uint32_t target_compact = GetSmsgDifficulty(chain_time);
    return smsg::GetDifficulty(target_compact);
},
    };
}

static RPCHelpMan smsggetinfo()
{
    return RPCHelpMan{"smsggetinfo",
                "\nReturns an object containing SMSG-related information.\n",
                {
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "", {
                        {RPCResult::Type::BOOL, "enabled", "True if SMSG is enabled"},
                        {RPCResult::Type::STR, "wallet", "name of the currently active wallet or \"None set\""},
                    },
                },
                RPCExamples{
            HelpExampleCli("smsggetinfo", "") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("smsggetinfo", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue obj(UniValue::VOBJ);

    obj.pushKV("enabled", smsg::fSecMsgEnabled);
    if (smsg::fSecMsgEnabled) {
        obj.pushKV("active_wallet", smsgModule.GetWalletName());
#ifdef ENABLE_WALLET
        UniValue wallet_names(UniValue::VARR);
        for (const auto &pw : smsgModule.m_vpwallets) {
            wallet_names.push_back(pw->GetName());
        }
        obj.pushKV("enabled_wallets", wallet_names);
#endif
    }

    return obj;
},
    };
}

static RPCHelpMan smsgpeers()
{
    return RPCHelpMan{"smsgpeers",
        "\nReturns data about each connected SMSG node as a json array of objects.\n",
        {
            {"index", RPCArg::Type::NUM, /* default */ "", "Peer index, omit for list."},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::NUM, "id", "Peer index"},
                    {RPCResult::Type::NUM, "version", "Peer version"},
                    {RPCResult::Type::NUM, "ignoreuntil", "Peer ignored until time"},
                    {RPCResult::Type::NUM, "misbehaving", "Misbehaviour counter"},
                    {RPCResult::Type::NUM, "numwantsent", "Number of smsges requested from peer"},
                    {RPCResult::Type::NUM, "receivecounter", "Messages received from peer in window"},
                    {RPCResult::Type::NUM, "ignoredcounter", "Number of times peer has been ignored"},
                }},
            }
        },
        RPCExamples{
    HelpExampleCli("smsgpeers", "") +
    "\nAs a JSON-RPC call\n"
    + HelpExampleRpc("smsgpeers", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    int index = request.params[0].isNull() ? -1 : request.params[0].get_int();

    UniValue result(UniValue::VARR);

    smsgModule.GetNodesStats(index, result);

    return result;
},
    };
}

static RPCHelpMan smsgzmqpush()
{
    return RPCHelpMan{"smsgzmqpush",
            "\nResend ZMQ notifications.\n",
            {
                {"options", RPCArg::Type::OBJ, /* default */ "", "",
                    {
                        {"timefrom", RPCArg::Type::NUM, /* default */ "0", "Skip messages received before timestamp."},
                        {"timeto", RPCArg::Type::NUM, /* default */ "max_int", "Skip messages received after timestamp."},
                        {"unreadonly", RPCArg::Type::BOOL, /* default */ "true", "Resend only unread messages."},
                    },
                    "options"},
            },
            RPCResult{
                RPCResult::Type::OBJ, "", "", {
                    {RPCResult::Type::NUM, "numsent", "Number of notifications sent"},
            }},
            RPCExamples{
        HelpExampleCli("smsgzmqpush", "'{ \"unreadonly\": false }'") +
        "\nAs a JSON-RPC call\n"
        + HelpExampleRpc("smsgzmqpush", "{ \"unreadonly\": false }")
            },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    RPCTypeCheck(request.params, {UniValue::VOBJ}, true);

    bool unreadonly = true;
    int64_t timefrom = 0;
    int64_t timeto = std::numeric_limits<int64_t>::max();
    int num_sent = 0;

    UniValue options = request.params[0];
    if (options.isObject()) {
        RPCTypeCheckObj(options,
        {
            {"timefrom",        UniValueType(UniValue::VNUM)},
            {"timeto",          UniValueType(UniValue::VNUM)},
            {"unreadonly",      UniValueType(UniValue::VBOOL)},
        }, true, true);
        if (options["timefrom"].isNum()) {
            timefrom = options["timefrom"].get_int64();
        }
        if (options["timeto"].isNum()) {
            timeto = options["timeto"].get_int64();
        }
        if (options["unreadonly"].isBool()) {
            unreadonly = options["unreadonly"].get_bool();
        }
    }

     {
        LOCK(smsg::cs_smsgDB);

        smsg::SecMsgDB dbInbox;
        if (!dbInbox.Open("cr+")) {
            throw std::runtime_error("Could not open DB.");
        }

        uint8_t chKey[30];
        smsg::SecMsgStored smsgStored;
        leveldb::Iterator *it = dbInbox.pdb->NewIterator(leveldb::ReadOptions());
        while (dbInbox.NextSmesg(it, smsg::DBK_INBOX, chKey, smsgStored)) {
            if (unreadonly
                && !(smsgStored.status & SMSG_MASK_UNREAD)) {
                continue;
            }
            if (smsgStored.timeReceived < timefrom ||
                smsgStored.timeReceived > timeto) {
                continue;
            }

            smsg::SecureMessage smsg(smsgStored.vchMessage.data());
            const smsg::SecureMessage *psmsg = &smsg;

            std::vector<uint8_t> vchUint160;
            vchUint160.resize(20);
            memcpy(vchUint160.data(), &chKey[10], 20);
            uint160 hash(vchUint160);

            GetMainSignals().NewSecureMessage(psmsg, hash);
            num_sent++;
        }
        delete it;
    } // cs_smsgDB

    UniValue result(UniValue::VOBJ);

    result.pushKV("numsent", num_sent);

    return result;
},
    };
};

static RPCHelpMan smsgdebug()
{
    return RPCHelpMan{"smsgdebug",
        "\nCommands useful for debugging.\n",
        {
            {"command", RPCArg::Type::STR, /* default */ "", "\"clearbanned\",\"dumpids\",\"dumpfundingtxids\"."},
            {"arg1", RPCArg::Type::STR, /* default */ "", ""},
        },
        RPCResults{
        },
        RPCExamples{
    HelpExampleCli("smsgdebug", "") +
    "\nAs a JSON-RPC call\n"
    + HelpExampleRpc("smsgdebug", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    EnsureSMSGIsEnabled();

    std::string mode = "none";
    if (request.params.size() > 0) {
        mode = request.params[0].get_str();
    }

    UniValue result(UniValue::VOBJ);

    if (mode == "clearbanned") {
        result.pushKV("command", mode);
        smsgModule.ClearBanned();
    } else
    if (mode == "dumpids") {
        fs::path filepath = gArgs.GetDataDirNet() / "smsg_ids.txt";
        if (fs::exists(filepath)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "smsg_ids.txt already exists in the datadir. Please move it out of the way first");
        }

        std::ofstream file;
        file.open(filepath);
        if (!file.is_open()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open dump file");
        }

        int num_messages = 0;
        LOCK(smsgModule.cs_smsg);
        std::map<int64_t, smsg::SecMsgBucket>::const_iterator it;
        std::vector<uint8_t> vch_msg;
        for (it = smsgModule.buckets.begin(); it != smsgModule.buckets.end(); ++it) {
            const std::set<smsg::SecMsgToken> &token_set = it->second.setTokens;
            for (auto token : token_set) {
                if (smsgModule.Retrieve(token, vch_msg) != smsg::SMSG_NO_ERROR) {
                    LogPrintf("SecureMsgRetrieve failed %d.\n", token.timestamp);
                    continue;
                }
                smsg::SecureMessage smsg(vch_msg.data());
                const smsg::SecureMessage *psmsg = &smsg;
                if (psmsg->version[0] == 0 && psmsg->version[1] == 0) {
                    continue; // Skip purged
                }
                file << strprintf("%d,%s\n", it->first, HexStr(smsgModule.GetMsgID(psmsg, vch_msg.data() + smsg::SMSG_HDR_LEN)));
                num_messages++;
            }
        }

        file.close();
        result.pushKV("messages", num_messages);
    } else
    if (mode == "dumpfundingtxids") {
        smsgModule.ShowFundingTxns(result);
    } else {
        result.pushKV("error", "Unknown command");
    }

    return result;
},
    };
}

static const CRPCCommand commands[] =
{ //  category              actor (function)
  //  --------------------- -----------------------
    { "smsg",               "smsgenable",               &smsgenable,      {}                 },
    { "smsg",               "smsgsetwallet",            &smsgsetwallet,      {}              },
    { "smsg",               "smsgdisable",               &smsgdisable,      {}                },
    { "smsg",               "smsgoptions",               &smsgoptions,      {}                },
    { "smsg",               "smsglocalkeys",               &smsglocalkeys,      {}              },
    { "smsg",               "smsgscanchain",               &smsgscanchain,      {}              },
    { "smsg",               "smsgscanbuckets",               &smsgscanbuckets,      {}            },
    { "smsg",               "smsgaddaddress",               &smsgaddaddress,      {}             },
    { "smsg",               "smsgaddlocaladdress",               &smsgaddlocaladdress,      {}        },
    { "smsg",               "smsgimportprivkey",               &smsgimportprivkey,      {}          },
    { "smsg",               "smsgdumpprivkey",               &smsgdumpprivkey,      {}            },
    { "smsg",               "smsggetpubkey",               &smsggetpubkey,      {}              },
    { "smsg",               "smsgsend",               &smsgsend,      {}                   },
    { "smsg",               "smsginbox",               &smsginbox,      {}                  },
    { "smsg",               "smsgoutbox",               &smsgoutbox,      {}                 },
    { "smsg",               "smsgbuckets",               &smsgbuckets,      {}                },
    { "smsg",               "smsgview",               &smsgview,      {}                   },
    { "smsg",               "smsgone",               &smsgone,      {}                    },
    { "smsg",               "smsgimport",               &smsgimport,      {}                 },
    { "smsg",               "smsgpurge",               &smsgpurge,      {}                  },
    { "smsg",               "smsggetfeerate",               &smsggetfeerate,      {}             },
    { "smsg",               "smsggetdifficulty",               &smsggetdifficulty,      {}          },
    { "smsg",               "smsggetinfo",               &smsggetinfo,      {}                },
    { "smsg",               "smsgpeers",               &smsgpeers,      {}                  },
    { "smsg",               "smsgzmqpush",               &smsgzmqpush,      {}                },
    { "smsg",               "smsgdebug",               &smsgdebug,      {}                  },
};

void RegisterSmsgRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
