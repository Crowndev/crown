// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Crown Core developers
// Copyright (c) 2020 The Crown developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crown/nodewallet.h>
#include <node/context.h>
#include <rpc/blockchain.h>
#include <smsg/smessage.h>

class NodeWallet;
NodeWallet currentNode;

bool NodeWallet::GetMasternodeVinAndKeys(CTxIn& txinRet, CPubKey& pubKeyRet, CKey& keyRet, std::shared_ptr<CWallet> pwallet)
{
    if (!pwallet) return false;

    std::vector<COutput> vPossibleCoins;
    pwallet->AvailableCoins(vPossibleCoins, Params().GetConsensus().subsidy_asset, true, nullptr, ALL_COINS, Params().MasternodeCollateral(), Params().MasternodeCollateral());
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
    pwallet->AvailableCoins(vPossibleCoins, Params().GetConsensus().subsidy_asset, true, nullptr, ALL_COINS, Params().SystemnodeCollateral(), Params().SystemnodeCollateral());
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
    CAmountMap nFeeRequired;
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

void NodeMinter(const CChainParams& chainparams, CConnman& connman)
{
    util::ThreadRename("crown-minter");

    if (!fMasterNode && !fSystemNode)
        return;
    if (fReindex || fImporting)
        return;
    if (!gArgs.GetBoolArg("-jumpstart", false)) {
        if (connman.GetNodeCount(CConnman::CONNECTIONS_ALL) == 0){
            LogPrintf("%s: No connections..\n", __func__);
            return;
        }
        if (::ChainActive().Tip()->nHeight+1 < Params().PoSStartHeight()){
            LogPrintf("%s: PoS Pre start Height..\n", __func__);
            return;
        }
        if (::ChainstateActive().IsInitialBlockDownload()){
            LogPrintf("%s: Initial Download..\n", __func__);
            return;
        }
        if (!masternodeSync.IsSynced() || !systemnodeSync.IsSynced()){
            LogPrintf("%s: Masternode/Systemnode Sync..\n", __func__);
            return;
        }
    }

    LogPrintf("%s: Attempting to stake..\n", __func__);

    //
    // Create new block
    //
    CScript dummyscript;

    std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler(*g_rpc_node->mempool, chainparams).CreateNewBlock(dummyscript, true));

    if (!pblocktemplate.get()) {
        LogPrintf("%s: Stake not found..\n", __func__);
        return;
    }
    CBlock *pblock = &pblocktemplate->block;
    //IncrementExtraNonce(pblock, ::ChainActive().Tip(), nExtraNonce);

    // if proof-of-stake block found then process block
    // Process this block the same as if we had received it from another node
    std::shared_ptr<CBlock> shared_pblock = std::make_shared<CBlock>(*pblock);

	if (!g_chainman.ProcessNewBlock(chainparams, shared_pblock, true, nullptr)) {
		LogPrintf("%s - ProcessNewBlock() failed, block not accepted\n", __func__);
		return;
	}

    return;
}

#define STAKE_SEARCH_INTERVAL 30
bool NodeWallet::CreateCoinStake(const int nHeight, const uint32_t& nBits, const uint32_t& nTime, CMutableTransaction& txCoinStake, uint32_t& nTxNewTime, StakePointer& stakePointer)
{
    CTxIn* pvinActiveNode;
    CPubKey* ppubkeyActiveNode;
    int nActiveNodeInputHeight;
    std::vector<StakePointer> vStakePointers;
    CAmount nAmountMN;


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

    OUTPUT_PTR<CTxData> out0 = MAKE_OUTPUT<CTxData>();
    out0->vData.resize(4);
    uint32_t tmp = htole32(nHeight+1);
    memcpy(&out0->vData[0], &tmp, 4);
    txCoinStake.vdata.push_back(out0);

    int64_t m_smsg_fee_rate_target = 0;
    uint32_t m_smsg_difficulty_target = 0; // 0 = auto
    CTransactionRef txPrevCoinstake = nullptr;

    // Place SMSG fee rate
    {
        CAmount smsg_fee_rate = Params().GetConsensus().smsg_fee_msg_per_day_per_k;

        if (!coinStakeCache.GetCoinStake(::ChainActive().Tip()->GetBlockHash(), txPrevCoinstake)) {
            return error("%s: Failed to get previous coinstake: %s.", __func__, ::ChainActive().Tip()->GetBlockHash().ToString());
        }
        txPrevCoinstake->GetSmsgFeeRate(smsg_fee_rate);

        if (m_smsg_fee_rate_target > 0) {
            int64_t diff = m_smsg_fee_rate_target - smsg_fee_rate;
            int64_t max_delta = Params().GetMaxSmsgFeeRateDelta(smsg_fee_rate);
            if (diff > max_delta) {
                diff = max_delta;
            }
            if (diff < -max_delta) {
                diff = -max_delta;
            }
            smsg_fee_rate += diff;
        }
        std::vector<uint8_t> vSmsgFeeRate(1), &vData = *txCoinStake.vdata[0]->GetPData();
        vSmsgFeeRate[0] = DO_SMSG_FEE;
        if (0 != part::PutVarInt(vSmsgFeeRate, smsg_fee_rate)) {
            return error("%s: PutVarInt failed: %d.", __func__, smsg_fee_rate);
        }
        vData.insert(vData.end(), vSmsgFeeRate.begin(), vSmsgFeeRate.end());
        CAmount test_fee = 0;
        assert(ExtractCoinStakeInt64(vData, DO_SMSG_FEE, test_fee));
        assert(test_fee == smsg_fee_rate);
    }

    // Place SMSG difficulty
    {
        uint32_t last_compact = Params().GetConsensus().smsg_min_difficulty, next_compact = m_smsg_difficulty_target;
        if (!txPrevCoinstake && !coinStakeCache.GetCoinStake(::ChainActive().Tip()->GetBlockHash(), txPrevCoinstake)) {
            return error("%s: Failed to get previous coinstake: %s.", __func__, ::ChainActive().Tip()->GetBlockHash().ToString());
        }
        txPrevCoinstake->GetSmsgDifficulty(last_compact);
        if (m_smsg_difficulty_target == 0) {
            next_compact = last_compact;
            int auto_adjust = smsgModule.AdjustDifficulty(txCoinStake.nTime);
            if (auto_adjust > 0 && last_compact != Params().GetConsensus().smsg_min_difficulty) {
                next_compact += auto_adjust;
            } else
            if (auto_adjust < 0) {
                next_compact += auto_adjust;
            }
        }

        uint32_t test_compact;
        if (last_compact != next_compact) {

            uint32_t delta;
            if (last_compact > next_compact) {
                delta = last_compact - next_compact;
                if (delta > Params().GetConsensus().smsg_difficulty_max_delta) {
                    next_compact = last_compact - Params().GetConsensus().smsg_difficulty_max_delta;
                }
            } else {
                delta = next_compact - last_compact;
                if (delta > Params().GetConsensus().smsg_difficulty_max_delta) {
                    next_compact = last_compact + Params().GetConsensus().smsg_difficulty_max_delta;
                }
            }

            if (next_compact > Params().GetConsensus().smsg_min_difficulty) {
                next_compact = Params().GetConsensus().smsg_min_difficulty;
            }
        }

        std::vector<uint8_t> vSmsgDifficulty(5), &vData = *txCoinStake.vdata[0]->GetPData();
        vSmsgDifficulty[0] = DO_SMSG_DIFFICULTY;
        uint32_t tmp = htole32(next_compact);
        memcpy(&vSmsgDifficulty[1], &tmp, 4);
        vData.insert(vData.end(), vSmsgDifficulty.begin(), vSmsgDifficulty.end());
        assert(ExtractCoinStakeUint32(vData, DO_SMSG_DIFFICULTY, test_compact));
        assert(test_compact == next_compact);
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

        if (!SearchTimeSpan(kernel, nTime, nTime + STAKE_SEARCH_INTERVAL, nTarget)){
			
            continue;
		}

        LogPrintf("%s: Found valid kernel for mn/sn collateral %s\n", __func__, pvinActiveNode->prevout.ToString());
        LogPrintf("%s: %s\n", __func__, kernel.ToString());

        //Add stake payment to coinstake tx
        CAmount nBlockReward = GetBlockValue(nHeight, 0); //Do not add fees until after they are packaged into the block
        CScript scriptBlockReward = GetScriptForDestination(PKHash(*ppubkeyActiveNode));
        CTxOutAsset out(Params().GetConsensus().subsidy_asset, nBlockReward, scriptBlockReward);

        if(txCoinStake.nVersion >= TX_ELE_VERSION)
            txCoinStake.vpout.emplace_back(out);
        else
            txCoinStake.vout.emplace_back(out);

        nTxNewTime = kernel.GetTime();
        stakePointer = pointer;
        CTxIn txin;
        txin.prevout.SetNull();// = pOutpoint; //HUH ??
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
        scriptMNPubKey = GetScriptForDestination(PKHash(pstaker->pubkey));
        for (auto& tx : blockLastPaid.vtx) {
            auto stakeSource = COutPoint(tx->GetHash(), nPaymentSlot);
            uint256 hashPointer = stakeSource.GetHash();
            if (mapUsedStakePointers.count(hashPointer))
                continue;

            if (tx->IsCoinBase()) {
                CTxOutAsset mout = (tx->nVersion >= TX_ELE_VERSION ? tx->vpout[nPaymentSlot] : tx->vout[nPaymentSlot]);
                if(mout.scriptPubKey == scriptMNPubKey){
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
