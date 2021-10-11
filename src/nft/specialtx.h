// Copyright (c) 2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_SPECIALTX_H
#define CROWN_SPECIALTX_H

#include <primitives/transaction.h>
#include <streams.h>
#include <validation.h>
#include <version.h>

class CBlock;
class CBlockIndex;
class CTransaction;
class TxValidationState;

bool CheckNftTx(const CTransaction& tx, const CBlockIndex* pindexLast, TxValidationState& state);
bool ProcessNftTxsInBlock(const CBlock& block, const CBlockIndex* pindexLast, TxValidationState& state, bool fJustCheck);
bool UndoNftTxsInBlock(const CBlock& block, const CBlockIndex* pindex);

template <typename T>
inline bool GetNftTxPayload(const std::vector<unsigned char>& payload, T& obj)
{
    CDataStream ds(payload, SER_NETWORK, PROTOCOL_VERSION);

    try {
        ds >> obj;
    } catch (std::exception& e) {
        return false;
    }
    return ds.empty();
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

#endif //CROWN_SPECIALTX_H
