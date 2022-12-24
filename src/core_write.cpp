// Copyright (c) 2009-2019 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <assetdb.h>

#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <key_io.h>
#include <script/script.h>
#include <script/standard.h>
#include <serialize.h>
#include <streams.h>
#include <univalue.h>
#include <util/system.h>
#include <util/strencodings.h>

UniValue ValueFromAmount(const CAmount& amount)
{
    bool sign = amount < 0;
    int64_t n_abs = (sign ? -amount : amount);
    int64_t quotient = n_abs / COIN;
    int64_t remainder = n_abs % COIN;
    return UniValue(UniValue::VNUM,
            strprintf("%s%d.%08d", sign ? "-" : "", quotient, remainder));
}

std::string FormatScript(const CScript& script)
{
    std::string ret;
    CScript::const_iterator it = script.begin();
    opcodetype op;
    while (it != script.end()) {
        CScript::const_iterator it2 = it;
        std::vector<unsigned char> vch;
        if (script.GetOp(it, op, vch)) {
            if (op == OP_0) {
                ret += "0 ";
                continue;
            } else if ((op >= OP_1 && op <= OP_16) || op == OP_1NEGATE) {
                ret += strprintf("%i ", op - OP_1NEGATE - 1);
                continue;
            } else if (op >= OP_NOP && op <= OP_NOP10) {
                std::string str(GetOpName(op));
                if (str.substr(0, 3) == std::string("OP_")) {
                    ret += str.substr(3, std::string::npos) + " ";
                    continue;
                }
            }
            if (vch.size() > 0) {
                ret += strprintf("0x%x 0x%x ", HexStr(std::vector<uint8_t>(it2, it - vch.size())),
                                               HexStr(std::vector<uint8_t>(it - vch.size(), it)));
            } else {
                ret += strprintf("0x%x ", HexStr(std::vector<uint8_t>(it2, it)));
            }
            continue;
        }
        ret += strprintf("0x%x ", HexStr(std::vector<uint8_t>(it2, script.end())));
        break;
    }
    return ret.substr(0, ret.size() - 1);
}

const std::map<unsigned char, std::string> mapSigHashTypes = {
    {static_cast<unsigned char>(SIGHASH_ALL), std::string("ALL")},
    {static_cast<unsigned char>(SIGHASH_ALL|SIGHASH_ANYONECANPAY), std::string("ALL|ANYONECANPAY")},
    {static_cast<unsigned char>(SIGHASH_NONE), std::string("NONE")},
    {static_cast<unsigned char>(SIGHASH_NONE|SIGHASH_ANYONECANPAY), std::string("NONE|ANYONECANPAY")},
    {static_cast<unsigned char>(SIGHASH_SINGLE), std::string("SINGLE")},
    {static_cast<unsigned char>(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY), std::string("SINGLE|ANYONECANPAY")},
};

std::string SighashToStr(unsigned char sighash_type)
{
    const auto& it = mapSigHashTypes.find(sighash_type);
    if (it == mapSigHashTypes.end()) return "";
    return it->second;
}

/**
 * Create the assembly string representation of a CScript object.
 * @param[in] script    CScript object to convert into the asm string representation.
 * @param[in] fAttemptSighashDecode    Whether to attempt to decode sighash types on data within the script that matches the format
 *                                     of a signature. Only pass true for scripts you believe could contain signatures. For example,
 *                                     pass false, or omit the this argument (defaults to false), for scriptPubKeys.
 */
std::string ScriptToAsmStr(const CScript& script, const bool fAttemptSighashDecode)
{
    std::string str;
    opcodetype opcode;
    std::vector<unsigned char> vch;
    CScript::const_iterator pc = script.begin();
    while (pc < script.end()) {
        if (!str.empty()) {
            str += " ";
        }
        if (!script.GetOp(pc, opcode, vch)) {
            str += "[error]";
            return str;
        }
        if (0 <= opcode && opcode <= OP_PUSHDATA4) {
            if (vch.size() <= static_cast<std::vector<unsigned char>::size_type>(4)) {
                str += strprintf("%d", CScriptNum(vch, false).getint());
            } else {
                // the IsUnspendable check makes sure not to try to decode OP_RETURN data that may match the format of a signature
                if (fAttemptSighashDecode && !script.IsUnspendable()) {
                    std::string strSigHashDecode;
                    // goal: only attempt to decode a defined sighash type from data that looks like a signature within a scriptSig.
                    // this won't decode correctly formatted public keys in Pubkey or Multisig scripts due to
                    // the restrictions on the pubkey formats (see IsCompressedOrUncompressedPubKey) being incongruous with the
                    // checks in CheckSignatureEncoding.
                    if (CheckSignatureEncoding(vch, SCRIPT_VERIFY_STRICTENC, nullptr)) {
                        const unsigned char chSigHashType = vch.back();
                        const auto it = mapSigHashTypes.find(chSigHashType);
                        if (it != mapSigHashTypes.end()) {
                            strSigHashDecode = "[" + it->second + "]";
                            vch.pop_back(); // remove the sighash type byte. it will be replaced by the decode.
                        }
                    }
                    str += HexStr(vch) + strSigHashDecode;
                } else {
                    str += HexStr(vch);
                }
            }
        } else {
            str += GetOpName(opcode);
        }
    }
    return str;
}

std::string EncodeHexTx(const CTransaction& tx, const int serializeFlags)
{
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION | serializeFlags);
    ssTx << tx;
    return HexStr(ssTx);
}

void ScriptToUniv(const CScript& script, UniValue& out, bool include_address)
{
    out.pushKV("asm", ScriptToAsmStr(script));
    out.pushKV("hex", HexStr(script));

    std::vector<std::vector<unsigned char>> solns;
    TxoutType type = Solver(script, solns);
    out.pushKV("type", GetTxnOutputType(type));

    CTxDestination address;
    if (include_address && ExtractDestination(script, address) && type != TxoutType::PUBKEY) {
        out.pushKV("address", EncodeDestination(address));
    }
}

void ScriptPubKeyToUniv(const CScript& scriptPubKey,
                        UniValue& out, bool fIncludeHex)
{
    TxoutType type;
    std::vector<CTxDestination> addresses;
    int nRequired;

    out.pushKV("asm", ScriptToAsmStr(scriptPubKey));
    if (fIncludeHex)
        out.pushKV("hex", HexStr(scriptPubKey));

    if (!ExtractDestinations(scriptPubKey, type, addresses, nRequired) || type == TxoutType::PUBKEY) {
        out.pushKV("type", GetTxnOutputType(type));
        return;
    }

    out.pushKV("reqSigs", nRequired);
    out.pushKV("type", GetTxnOutputType(type));

    UniValue a(UniValue::VARR);
    for (const CTxDestination& addr : addresses) {
        a.push_back(EncodeDestination(addr));
    }
    out.pushKV("addresses", a);
}

void DataToJSON(const CTxDataBase *baseOut, UniValue &entry)
{
    switch (baseOut->GetVersion()) {
        case OUTPUT_DATA:{
            CTxData *s = (CTxData*) baseOut;
            entry.pushKV("type", "data");
            entry.pushKV("data_hex", HexStr(s->vData));
            break;
        }
        default:
            entry.pushKV("type", "unknown");
            break;
    }
};

void AssetToUniv(const CAssetData& assetdata, UniValue &entry){
	
	CAsset asset = assetdata.asset;
    entry.pushKV("version", (int)asset.nVersion);
    uint32_t type = asset.GetType();
    entry.pushKV("type", AssetTypeToString(type));
    entry.pushKV("name", asset.getAssetName());
    entry.pushKV("symbol", asset.getShortName());
    entry.pushKV("id", asset.GetHex());
    entry.pushKV("collateral", assetdata.inputAmount);
    entry.pushKV("Issued", assetdata.issuedAmount);
    entry.pushKV("Transaction", assetdata.txhash.GetHex());
    entry.pushKV("Time", (int64_t)assetdata.nTime);

    entry.pushKV("expiry", (int64_t)asset.GetExpiry());
    entry.pushKV("transferable", asset.isTransferable() ? "yes" : "no");
    entry.pushKV("convertable", asset.isConvertable() ? "yes" : "no");
    entry.pushKV("limited", asset.isLimited() ? "yes" : "no");
    entry.pushKV("restricted", asset.isRestricted() ? "yes" : "no");
    entry.pushKV("stakeable", asset.isStakeable() ? "yes" : "no");
    entry.pushKV("inflation", asset.isInflatable() ? "yes" : "no");
    entry.pushKV("divisible", asset.isDivisible() ? "yes" : "no");

}

void AssetToUniv(const CAsset& asset, UniValue &entry){

    entry.pushKV("version", (int)asset.nVersion);
    uint32_t type = asset.GetType();

    entry.pushKV("type", AssetTypeToString(type));
    entry.pushKV("name", asset.getAssetName());
    entry.pushKV("symbol", asset.getShortName());
    entry.pushKV("id", asset.GetHex());
//    entry.pushKV("contract_hash", asset.contract_hash.GetHex());
    entry.pushKV("expiry", (int64_t)asset.GetExpiry());
    entry.pushKV("transferable", asset.isTransferable() ? "yes" : "no");
    entry.pushKV("convertable", asset.isConvertable() ? "yes" : "no");
    entry.pushKV("limited", asset.isLimited() ? "yes" : "no");
    entry.pushKV("restricted", asset.isRestricted() ? "yes" : "no");
    entry.pushKV("stakeable", asset.isStakeable() ? "yes" : "no");
    entry.pushKV("inflation", asset.isInflatable() ? "yes" : "no");
    entry.pushKV("divisible", asset.isDivisible() ? "yes" : "no");

}

void TxToUniv(const CTransaction& tx, const uint256& hashBlock, UniValue& entry, bool include_hex, int serialize_flags)
{
    entry.pushKV("txid", tx.GetHash().GetHex());
    entry.pushKV("hash", tx.GetWitnessHash().GetHex());
    // Transaction version is actually unsigned in consensus checks, just signed in memory,
    // so cast to unsigned before giving it to the user.
    entry.pushKV("version", static_cast<int64_t>(static_cast<uint32_t>(tx.nVersion)));
    entry.pushKV("time", (int64_t)tx.nTime);
    entry.pushKV("size", (int)::GetSerializeSize(tx, PROTOCOL_VERSION));
    entry.pushKV("vsize", (GetTransactionWeight(tx) + WITNESS_SCALE_FACTOR - 1) / WITNESS_SCALE_FACTOR);
    entry.pushKV("weight", GetTransactionWeight(tx));
    entry.pushKV("locktime", (int64_t)tx.nLockTime);

    UniValue vin(UniValue::VARR);
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        const CTxIn& txin = tx.vin[i];
        UniValue in(UniValue::VOBJ);
        if (tx.IsCoinBase())
            in.pushKV("coinbase", HexStr(txin.scriptSig));
        else {
            in.pushKV("txid", txin.prevout.hash.GetHex());
            in.pushKV("vout", (int64_t)txin.prevout.n);
            UniValue o(UniValue::VOBJ);
            o.pushKV("asm", ScriptToAsmStr(txin.scriptSig, true));
            o.pushKV("hex", HexStr(txin.scriptSig));
            in.pushKV("scriptSig", o);
        }

        in.pushKV("sequence", (int64_t)txin.nSequence);

        if (tx.witness.vtxinwit.size() > i) {
            const CScriptWitness &scriptWitness = tx.witness.vtxinwit[i].scriptWitness;
            if (!scriptWitness.IsNull()) {
                UniValue txinwitness(UniValue::VARR);
                for (const auto &item : scriptWitness.stack) {
                    txinwitness.push_back(HexStr(item));
                }
                in.pushKV("txinwitness", txinwitness);
            }
        }
        vin.push_back(in);
    }
    entry.pushKV("vin", vin);

    UniValue vdata(UniValue::VARR);
    for (unsigned int i = 0; i < tx.vdata.size(); i++)
    {
        UniValue out(UniValue::VOBJ);
        out.pushKV("n", (int64_t)i);
        DataToJSON(tx.vdata[i].get(), out);
        vdata.push_back(out);
    }
    entry.pushKV("data", vdata);

    UniValue vout(UniValue::VARR);

    if(tx.nVersion >= TX_ELE_VERSION){
        for (unsigned int k = 0; k < tx.vpout.size() ; k++) {
            UniValue out(UniValue::VOBJ);
            out.pushKV("value", ValueFromAmount(tx.vpout[k].nValue));
            UniValue p(UniValue::VOBJ);
            AssetToUniv(tx.vpout[k].nAsset, p);
            out.pushKV("asset", p);
            out.pushKV("n", (int64_t)k);
            UniValue o(UniValue::VOBJ);
            ScriptPubKeyToUniv(tx.vpout[k].scriptPubKey, o, true);
            out.pushKV("scriptPubKey", o);
            vout.push_back(out);
        }
    }
    else
    {
        for (unsigned int k = 0; k < tx.vout.size() ; k++) {
            UniValue out(UniValue::VOBJ);
            out.pushKV("value", ValueFromAmount(tx.vout[k].nValue));
            out.pushKV("n", (int64_t)k);
            UniValue o(UniValue::VOBJ);
            ScriptPubKeyToUniv(tx.vout[k].scriptPubKey, o, true);
            out.pushKV("scriptPubKey", o);
            vout.push_back(out);
        }
    }

    entry.pushKV((tx.nVersion >= TX_ELE_VERSION ? "vpout" : "vout"), vout);

    if (!tx.extraPayload.empty()) {
        entry.pushKV("extraPayloadSize", (int)tx.extraPayload.size());
        entry.pushKV("extraPayload", HexStr(tx.extraPayload));
    }

    if (!hashBlock.IsNull())
        entry.pushKV("blockhash", hashBlock.GetHex());

    if (include_hex) {
        entry.pushKV("hex", EncodeHexTx(tx, serialize_flags)); // The hex-encoded transaction. Used the name "hex" to be consistent with the verbose output of "getrawtransaction".
    }
}

void AmountMapToUniv(const CAmountMap& balanceOrig, UniValue &entry)
{
    // Make sure the policyAsset is always present in the balance map.
    for(auto it = balanceOrig.begin(); it != balanceOrig.end(); ++it) 
        entry.pushKV(it->first.getAssetName(), ValueFromAmount(it->second));

}
