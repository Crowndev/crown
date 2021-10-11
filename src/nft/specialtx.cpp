// Copyright (c) 2020-2021 Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <nft/specialtx.h>

#include <chainparams.h>
#include <clientversion.h>
#include <consensus/validation.h>
#include <hash.h>
#include <nft/token.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <util/system.h>

bool CheckNftTx(const CTransaction& tx, const CBlockIndex* pindexLast, TxValidationState& state)
{
    if (tx.nVersion != TX_NFT_VERSION || tx.nType == TRANSACTION_NORMAL) {
        return true;
    }

    LogPrintf("    - found NFT transaction %s (version: %s type: %s)\n", tx.GetHash().ToString(), GetVersionName(tx.nVersion), GetTypeName(tx.nType));

    try {
        switch (tx.nType) {
        case TRANSACTION_NF_TOKEN_REGISTER:
            return ContextualCheckTokenRegister(tx, pindexLast, state);
        case TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER:
            return ContextualCheckTokenProtocolRegister(tx, pindexLast, state);
        }
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
    }

    return true;
}

bool ProcessNftTx(const CTransaction& tx, const CBlockIndex* pindexLast, TxValidationState& state)
{
    if (tx.nVersion != TX_NFT_VERSION || tx.nType == TRANSACTION_NORMAL) {
        return true;
    }

    switch (tx.nType) {
    case TRANSACTION_NF_TOKEN_REGISTER:
        return CheckTokenRegister(tx, pindexLast, state);
    case TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER:
        return CheckTokenProtocolRegister(tx, pindexLast, state);
    }

    return true;
}

bool UndoNftTx(const CTransaction& tx, const CBlockIndex* pindex)
{
    if (tx.nVersion != TX_NFT_VERSION || tx.nType == TRANSACTION_NORMAL) {
        return true;
    }

    switch (tx.nType) {
    case TRANSACTION_NF_TOKEN_REGISTER:
    case TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER:
        return true;
    }

    return false;
}

bool ProcessNftTxsInBlock(const CBlock& block, const CBlockIndex* pindex, TxValidationState& state, bool fJustCheck)
{
    try {
        for (int i = 0; i < (int)block.vtx.size(); i++) {
            const CTransaction& tx = *block.vtx[i];
            if (!CheckNftTx(tx, pindex->pprev, state)) {
                return false;
            }
            if (!ProcessNftTx(tx, pindex, state)) {
                return false;
            }
        }
    } catch (const std::exception& e) {
        LogPrintf("%s -- failed: %s\n", __func__, e.what());
    }

    return true;
}

bool UndoNftTxsInBlock(const CBlock& block, const CBlockIndex* pindex)
{
    try {
        for (int i = (int)block.vtx.size() - 1; i >= 0; --i) {
            const CTransaction& tx = *block.vtx[i];
            if (!UndoNftTx(tx, pindex)) {
                return false;
            }
        }
    } catch (const std::exception& e) {
        return error(strprintf("%s -- failed: %s\n", __func__, e.what()).c_str());
    }

    return true;
}

uint256 CalcNftTxInputsHash(const CTransaction& tx)
{
    CHashWriter hw(CLIENT_VERSION, SER_GETHASH);
    for (const auto& in : tx.vin) {
        hw << in.prevout;
    }
    return hw.GetHash();
}
