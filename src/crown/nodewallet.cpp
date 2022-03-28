// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Crown Core developers
// Copyright (c) 2020 The Crown developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crown/nodewallet.h>

class NodeWallet;
NodeWallet currentNode;

bool NodeWallet::GetMasternodeVinAndKeys(CTxIn& txinRet, CPubKey& pubKeyRet, CKey& keyRet, std::shared_ptr<CWallet> pwallet)
{
    if (!pwallet) return false;

    std::vector<COutput> vPossibleCoins;
    pwallet->AvailableCoins(vPossibleCoins, true, nullptr, Params().MasternodeCollateral(), Params().MasternodeCollateral());
    if (vPossibleCoins.empty()) {
        LogPrintf("NodeWallet::GetMasternodeVinAndKeys -- Could not locate any valid masternode vin\n");
        return false;
    }

    return GetVinAndKeysFromOutput(vPossibleCoins[0], txinRet, pubKeyRet, keyRet);
}

bool NodeWallet::GetSystemnodeVinAndKeys(CTxIn& txinRet, CPubKey& pubKeyRet, CKey& keyRet, std::shared_ptr<CWallet> pwallet)
{
    if (!pwallet) return false;

    std::vector<COutput> vPossibleCoins;
    pwallet->AvailableCoins(vPossibleCoins, true, nullptr, Params().SystemnodeCollateral(), Params().SystemnodeCollateral());
    if (vPossibleCoins.empty()) {
        LogPrintf("NodeWallet::GetSystemnodeVinAndKeys -- Could not locate any valid systemnode vin\n");
        return false;
    }

    return GetVinAndKeysFromOutput(vPossibleCoins[0], txinRet, pubKeyRet, keyRet);
}

bool NodeWallet::GetVinAndKeysFromOutput(COutput out, CTxIn& txinRet, CPubKey& pubkeyRet, CKey& keyRet, std::shared_ptr<CWallet> pwallet)
{
    if (!pwallet) return false;

    CScript pubScript;
    CKeyID keyID;

    txinRet = CTxIn(out.tx->tx->GetHash(), out.i);
    if(out.tx->tx->nVersion >= TX_ELE_VERSION)
        pubScript = out.tx->tx->vpout[out.i].scriptPubKey;
    else
        pubScript = out.tx->tx->vout[out.i].scriptPubKey;
    

    CTxDestination address;
    ExtractDestination(pubScript, address);
    auto key_id = std::get<PKHash>(address);
    keyID = ToKeyID(key_id);
    if (keyID.IsNull()) {
        LogPrintf("GetVinFromOutput -- Address does not refer to a key\n");
        return false;
    }

    LegacyScriptPubKeyMan* spk_man = pwallet->GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        LogPrintf("GetVinFromOutput -- This type of wallet does not support this command\n");
        return false;
    }

    if (!spk_man->GetKey(keyID, keyRet)) {
        LogPrintf("GetVinFromOutput -- Private key for address is not known\n");
        return false;
    }

    pubkeyRet = keyRet.GetPubKey();
    return true;
}

bool NodeWallet::GetBudgetSystemCollateralTX(CTransactionRef& tx, uint256 hash, std::shared_ptr<CWallet> pwallet)
{
    if (!pwallet) return false;

    const CAmount BUDGET_FEE_TX = (25 * COIN);

    CScript scriptChange;
    scriptChange << OP_RETURN << ToByteVector(hash);

    std::vector<CRecipient> vecSend;
    vecSend.push_back((CRecipient) { scriptChange, BUDGET_FEE_TX, 0, Params().GetConsensus().subsidy_asset, CAsset(), false, false });

    CCoinControl coinControl;
    int nChangePosRet = -1;
    CAmount nFeeRequired = 0;
    bilingual_str error;
    FeeCalculation fee_calc_out;

    return pwallet->CreateTransaction(vecSend, tx, nFeeRequired, nChangePosRet, error, coinControl, fee_calc_out);
}

bool NodeWallet::GetActiveMasternode(CMasternode*& activeStakingNode)
{
    activeStakingNode = nullptr;
    if (activeMasternode.status == ACTIVE_MASTERNODE_STARTED)
        activeStakingNode = mnodeman.Find(activeMasternode.vin);
    return activeStakingNode != nullptr;
}

bool NodeWallet::GetActiveSystemnode(CSystemnode*& activeStakingNode)
{
    activeStakingNode = nullptr;
    if (activeSystemnode.status == ACTIVE_SYSTEMNODE_STARTED)
        activeStakingNode = snodeman.Find(activeSystemnode.vin);
    return activeStakingNode != nullptr;
}

uint256 NodeWallet::GenerateStakeModifier(const CBlockIndex* prewardBlockIndex) const
{
    if (!prewardBlockIndex)
        return uint256();

    const CBlockIndex* pstakeModBlockIndex = prewardBlockIndex->GetAncestor(prewardBlockIndex->nHeight - Params().KernelModifierOffset());
    if (!pstakeModBlockIndex) {
        LogPrintf("GenerateStakeModifier -- Failed retrieving block index for stake modifier\n");
        return uint256();
    }

    return pstakeModBlockIndex->GetBlockHash();
}

#define STAKE_SEARCH_INTERVAL 30
bool NodeWallet::CreateCoinStake(const int nHeight, const uint32_t& nBits, const uint32_t& nTime, CMutableTransaction& txCoinStake, uint32_t& nTxNewTime, StakePointer& stakePointer, std::shared_ptr<CWallet> pwallet)
{
    if (!pwallet) return false;

    CTxIn* pvinActiveNode;
    CPubKey* ppubkeyActiveNode;
    int nActiveNodeInputHeight;
    std::vector<StakePointer> vStakePointers;
    CAmount nAmountMN;

    //! Add some spacing if blocks are coming in too fast
    if (nTime < ::ChainActive().Tip()->GetBlockTime())
        UninterruptibleSleep(std::chrono::milliseconds(5000));

    //! Maybe have a polymorphic base class for masternode and systemnode?
    if (fMasterNode) {
        CMasternode* activeStakingNode;
        if (!GetActiveMasternode(activeStakingNode)) {
            LogPrintf("CreateCoinStake -- Couldn't find CMasternode object for active masternode\n");
            return false;
        }
        if (!GetRecentStakePointers(vStakePointers)) {
            LogPrintf("CreateCoinStake -- Couldn't find recent payment blocks for MN\n");
            return false;
        }
        pvinActiveNode = &activeStakingNode->vin;
        ppubkeyActiveNode = &activeStakingNode->pubkey;
        nActiveNodeInputHeight = ::ChainActive().Height() - activeStakingNode->GetMasternodeInputAge();
        nAmountMN = static_cast<CAmount>(Params().MasternodeCollateral());

    } else if (fSystemNode) {
        CSystemnode* activeStakingNode;
        if (!GetActiveSystemnode(activeStakingNode)) {
            LogPrintf("CreateCoinStake -- Couldn't find CSystemnode object for active systemnode\n");
            return false;
        }
        if (!GetRecentStakePointers(vStakePointers)) {
            LogPrintf("CreateCoinStake -- Couldn't find recent payment blocks for SN\n");
            return false;
        }
        pvinActiveNode = &activeStakingNode->vin;
        ppubkeyActiveNode = &activeStakingNode->pubkey;
        nActiveNodeInputHeight = ::ChainActive().Height() - activeStakingNode->GetSystemnodeInputAge();
        nAmountMN = static_cast<CAmount>(Params().SystemnodeCollateral());

    } else {
        LogPrintf("CreateCoinStake -- Must be masternode or systemnode to create coin stake!\n");
        return false;
    }

    //Create kernels for each valid stake pointer and see if any create a successful proof
    for (auto pointer : vStakePointers) {
        if (!g_chainman.BlockIndex().count(pointer.hashBlock))
            continue;

        CBlockIndex* pindex = g_chainman.BlockIndex().at(pointer.hashBlock);

        // Make sure this pointer is not too deep
        if (nHeight - pindex->nHeight >= Params().ValidStakePointerDuration() + 1)
            continue;

        // check that collateral transaction happened long enough before this stake pointer
        if (pindex->nHeight - Params().KernelModifierOffset() <= nActiveNodeInputHeight)
            continue;

        // generate stake modifier based off block that happened before this stake pointer
        uint256 nStakeModifier = GenerateStakeModifier(pindex);
        if (nStakeModifier == uint256())
            continue;

        COutPoint pOutpoint(pointer.txid, pointer.nPos);
        Kernel kernel(pOutpoint, nAmountMN, nStakeModifier, pindex->GetBlockTime(), nTxNewTime);
        uint256 nTarget = ArithToUint256(arith_uint256().SetCompact(nBits));
        nLastStakeAttempt = GetTime();

        if (!SearchTimeSpan(kernel, nTime, nTime + STAKE_SEARCH_INTERVAL, nTarget))
            continue;

        LogPrintf("%s: Found valid kernel for mn/sn collateral %s\n", __func__, pvinActiveNode->prevout.ToString());
        LogPrintf("%s: %s\n", __func__, kernel.ToString());

        //Add stake payment to coinstake tx
        CAmount nBlockReward = GetBlockValue(nHeight, 0); //Do not add fees until after they are packaged into the block
        CScript scriptBlockReward = GetScriptForDestination(PKHash(ppubkeyActiveNode->GetID()));
        CTxOutAsset out(Params().GetConsensus().subsidy_asset, nBlockReward, scriptBlockReward);
        if(txCoinStake.nVersion >= TX_ELE_VERSION)
            txCoinStake.vpout.emplace_back(out);
        else
            txCoinStake.vout.emplace_back(out);

        nTxNewTime = kernel.GetTime();
        stakePointer = pointer;

        CTxIn txin;
        txin.scriptSig << OP_PROOFOFSTAKE;
        txCoinStake.vin.emplace_back(txin);

        return true;
    }

    return false;
}

template <typename stakingnode>
bool GetPointers(stakingnode* pstaker, std::vector<StakePointer>& vStakePointers, int nPaymentSlot)
{
    bool found = false;
    // get block index of last mn payment
    std::vector<const CBlockIndex*> vBlocksLastPaid;
    if (!pstaker->GetRecentPaymentBlocks(vBlocksLastPaid, false)) {
        LogPrintf("GetRecentStakePointer -- Couldn't find last paid block\n");
        return false;
    }

    int nBestHeight = ::ChainActive().Height();
    for (auto pindex : vBlocksLastPaid) {
        if (budget.IsBudgetPaymentBlock(pindex->nHeight))
            continue;

        // Pointer has to be at least deeper than the max reorg depth
        const int nMaxReorganizationDepth = 100;
        if (nBestHeight - pindex->nHeight < nMaxReorganizationDepth)
            continue;

        CBlock blockLastPaid;
        if (!ReadBlockFromDisk(blockLastPaid, pindex, Params().GetConsensus())) {
            LogPrintf("GetRecentStakePointer -- Failed reading block from disk\n");
            return false;
        }

        CScript scriptMNPubKey;
        scriptMNPubKey = GetScriptForDestination(PKHash(pstaker->pubkey.GetID()));
        for (auto& tx : blockLastPaid.vtx) {
            auto stakeSource = COutPoint(tx->GetHash(), nPaymentSlot);
            uint256 hashPointer = stakeSource.GetHash();
            if (mapUsedStakePointers.count(hashPointer))
                continue;
            
            CTxOutAsset mout = (tx->nVersion >= TX_ELE_VERSION ? tx->vpout[nPaymentSlot] : tx->vout[nPaymentSlot]);   
                
            if (tx->IsCoinBase() && mout.scriptPubKey == scriptMNPubKey) {
                StakePointer stakePointer;
                stakePointer.hashBlock = pindex->GetBlockHash();
                stakePointer.txid = tx->GetHash();
                stakePointer.nPos = nPaymentSlot;
                stakePointer.pubKeyProofOfStake = pstaker->pubkey;
                vStakePointers.emplace_back(stakePointer);
                found = true;
                continue;
            }
        }
    }

    return found;
}

bool NodeWallet::GetRecentStakePointers(std::vector<StakePointer>& vStakePointers)
{
    if (fMasterNode) {
        // find pointer to active CMasternode object
        CMasternode* pactiveMN;
        if (!GetActiveMasternode(pactiveMN))
            return error("GetRecentStakePointer -- Couldn't find CMasternode object for active masternode\n");

        return GetPointers(pactiveMN, vStakePointers, MN_PMT_SLOT);
    }

    CSystemnode* pactiveSN;
    if (!GetActiveSystemnode(pactiveSN))
        return error("GetRecentStakePointer -- Couldn't find CSystemnode object for active systemnode\n");

    return GetPointers(pactiveSN, vStakePointers, SN_PMT_SLOT);
}
