// Copyright (c) 2018-2019 The Dash Core developers
// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <clientversion.h>
#include <hash.h>
#include <platform/governance.h>
#include <platform/governance-vote.h>
#include <platform/nf-token/nf-token-protocol-reg-tx.h>
#include <platform/nf-token/nf-token-reg-tx.h>
#include <platform/nf-token/nf-tokens-manager.h>
#include <platform/nf-token/nft-protocols-manager.h>
#include <platform/specialtx.h>
#include <primitives/block.h>
#include <primitives/transaction.h>

bool CheckNftTx(const CTransaction& tx, const CBlockIndex* pindexLast, TxValidationState& state)
{
    if (tx.nVersion != TX_NFT_VERSION || tx.nType == TRANSACTION_NORMAL) {
        return true;
    }

    try {
        switch (tx.nType) {
            case TRANSACTION_GOVERNANCE_VOTE:
                return Platform::CheckVoteTx(tx, pindexLast, state);
            case TRANSACTION_NF_TOKEN_REGISTER:
                return Platform::NfTokenRegTx::CheckTx(tx, pindexLast, state);
            case TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER:
                return Platform::NfTokenProtocolRegTx::CheckTx(tx, pindexLast, state);
        }
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
    }

    return true;
}

bool ProcessNftTx(const CTransaction& tx, const CBlockIndex* pindex, TxValidationState& state)
{
    if (tx.nVersion != TX_NFT_VERSION || tx.nType == TRANSACTION_NORMAL) {
        return true;
    }

    switch (tx.nType) {
        case TRANSACTION_GOVERNANCE_VOTE:
            return Platform::ProcessVoteTx(tx, pindex, state);
        case TRANSACTION_NF_TOKEN_REGISTER:
            return Platform::NfTokenRegTx::ProcessTx(tx, pindex, state);
        case TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER:
            return Platform::NfTokenProtocolRegTx::ProcessTx(tx, pindex, state);
    }

    return true;
}

bool UndoNftTx(const CTransaction& tx, const CBlockIndex* pindex)
{
    if (tx.nVersion != TX_NFT_VERSION || tx.nType == TRANSACTION_NORMAL) {
        return true;
    }

    switch (tx.nType) {
        case TRANSACTION_GOVERNANCE_VOTE:
            return true;
        case TRANSACTION_NF_TOKEN_REGISTER:
            return Platform::NfTokenRegTx::UndoTx(tx, pindex);
        case TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER:
            return Platform::NfTokenProtocolRegTx::UndoTx(tx, pindex);
    }

    return false;
}

bool ProcessNftTxsInBlock(const CBlock& block, const CBlockIndex* pindex, TxValidationState& state)
{
    int64_t nTimeLoop = 0;
    try {
        int64_t nTime1 = GetTimeMicros();
        for (int i = 0; i < (int)block.vtx.size(); i++) {
            const CTransaction& tx = *block.vtx[i];
            if (!CheckNftTx(tx, pindex->pprev, state)) {
                return false;
            }
            if (!ProcessNftTx(tx, pindex, state)) {
                return false;
            }
        }
        int64_t nTime2 = GetTimeMicros(); nTimeLoop += nTime2 - nTime1;
        LogPrint(BCLog::BENCH, "        - ProcessNftTxsInBlock: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1), nTimeLoop * 0.000001);
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
    }

    return true;
}

bool UndoNftTxsInBlock(const CBlock& block, const CBlockIndex* pindex)
{
    int64_t nTimeLoop = 0;
    try {
        int64_t nTime1 = GetTimeMicros();
        for (int i = (int)block.vtx.size() - 1; i >= 0; --i) {
            const CTransaction& tx = *block.vtx[i];
            if (!UndoNftTx(tx, pindex)) {
                return false;
            }
        }
        int64_t nTime2 = GetTimeMicros(); nTimeLoop += nTime2 - nTime1;
        LogPrint(BCLog::BENCH, "        - UndoNftTxsInBlock: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1), nTimeLoop * 0.000001);
    } catch (const std::exception& e) {
        return error(strprintf("%s -- failed: %s\n", __func__, e.what()).c_str());
    }

    return true;
}

void UpdateNftTxsBlockTip(const CBlockIndex* pindex)
{
    Platform::NfTokensManager::Instance().UpdateBlockTip(pindex);
    Platform::NftProtocolsManager::Instance().UpdateBlockTip(pindex);
}

uint256 CalcNftTxInputsHash(const CTransaction& tx)
{
    CHashWriter hw(CLIENT_VERSION, SER_GETHASH);
    for (const auto& in : tx.vin) {
        hw << in.prevout;
    }
    return hw.GetHash();
}
