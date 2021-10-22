// Copyright (c) 2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_SPECIALTX_H
#define CROWN_SPECIALTX_H

#include <streams.h>
#include <version.h>

#include <wallet/coincontrol.h>
#include <init.h>
#include <crown/legacysigner.h>
#include <pubkey.h>
#include <rpc/protocol.h>
#include <util/system.h>

class CTransaction;
class CBlock;
class CBlockIndex;
class TxValidationState;

bool CheckNftTx(const CTransaction& tx, const CBlockIndex* pindex, TxValidationState& state);
bool ProcessNftTxsInBlock(const CBlock& block, const CBlockIndex* pindex, TxValidationState& state);
bool UndoNftTxsInBlock(const CBlock& block, const CBlockIndex* pindex);
void UpdateNftTxsBlockTip(const CBlockIndex* pindex);
uint256 CalcNftTxInputsHash(const CTransaction& tx);

template <typename T>
inline bool GetNftTxPayload(const std::vector<unsigned char>& payload, T& obj)
{
    CDataStream ds(payload, SER_NETWORK, PROTOCOL_VERSION);

    try {
        ds >> obj;
    } catch (std::exception& e) {
        return false;
    }

    if (!ds.empty())
        return false;

    return true;
}

template <typename T>
inline bool GetNftTxPayload(const CMutableTransaction& tx, T& obj)
{
    return GetNftTxPayload(tx.extraPayload, obj);
}

template <typename T>
inline bool GetNftTxPayload(const CTransaction& tx, T& obj)
{
    return GetNftTxPayload(tx.extraPayload, obj);
}

template <typename T>
void SetNftTxPayload(CMutableTransaction& tx, const T& payload)
{
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    tx.extraPayload.assign(ds.begin(), ds.end());
}

uint256 CalcNftTxInputsHash(const CTransaction& tx);

template <typename SpecialTxPayload>
static void FundSpecialTx(CMutableTransaction& tx, SpecialTxPayload payload)
{
    // resize so that fee calculation is correct
    payload.signature.resize(65);

    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    tx.extraPayload.assign(ds.begin(), ds.end());

    static CTxOut dummyTxOut(0, CScript() << OP_RETURN);
    bool dummyTxOutAdded = false;
    if (tx.vout.empty()) {
        // add dummy txout as FundTransaction requires at least one output
        tx.vout.push_back(dummyTxOut);
        dummyTxOutAdded = true;
    }

    CAmount nFee;
    CFeeRate feeRate = CFeeRate(0);
    int nChangePos = -1;
    std::string strFailReason;
    std::set<int> setSubtractFeeFromOutputs;
    // TODO fix
    //if (!GetWallets()[0]->FundTransaction(tx, nFee, false, feeRate, nChangePos, strFailReason, false, false, setSubtractFeeFromOutputs, true, CNoDestination()))
    //    throw JSONRPCError(RPC_INTERNAL_ERROR, strFailReason);

    if (dummyTxOutAdded && tx.vout.size() > 1) {
        // FundTransaction added a change output, so we don't need the dummy txout anymore
        // Removing it results in slight overpayment of fees, but we ignore this for now (as it's a very low amount)
        std::vector<CTxOut>::iterator it = std::find(tx.vout.begin(), tx.vout.end(), dummyTxOut);
        assert(it != tx.vout.end());
        tx.vout.erase(it);
    }
}

template <typename SpecialTxPayload>
static void SignSpecialTxPayload(const CMutableTransaction& tx, SpecialTxPayload& payload, const CKey& key)
{
    // payload.inputsHash = CalcNftTxInputsHash(tx);
    // TODO: consider adding inputs to payload

    payload.signature.clear();

    uint256 hash = ::SerializeHash(payload);
    if (!CHashSigner::SignHash(hash, key, payload.signature)) {
        //throw JSONRPCError(RPC_INTERNAL_ERROR, "failed to sign special tx");
    }
}

template <typename SpecialTxPayload>
static bool CheckInputsHashAndSig(const CTransaction& tx, const SpecialTxPayload& payload, const CKeyID& keyId, TxValidationState& state)
{
    uint256 inputsHash = CalcNftTxInputsHash(tx);
    // if (inputsHash != proTx.inputsHash)
    //    return state.DoS(100, false, REJECT_INVALID, "bad-protx-inputs-hash");
    // TODO: consider adding inputs to payload

    // TODO fix
    //std::string strError;
    //if (!CHashSigner::VerifyHash(::SerializeHash(payload), keyId, payload.signature, strError))
    //    return state.DoS(100, false, REJECT_INVALID, "bad-special-tx-sig", false, strError);

    return true;
}

#endif //CROWN_SPECIALTX_H
