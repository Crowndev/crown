// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Crown Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <auxpow.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <chainparams.h>
#include <validation.h>

arith_uint256 powLimit = UintToArith256(uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
arith_uint256 powLimitTestnet = UintToArith256(uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));

arith_uint256 GetProofOfWorkLimit(bool testnet = false)
{
    return testnet ? powLimitTestnet : powLimit;
}

void SetProofOfWorkLimit(uint256 newPowLimit)
{
    powLimit = UintToArith256(newPowLimit);
}

unsigned int static KimotoGravityWell(const CBlockIndex* pindexLast, const Consensus::Params& params, bool testnet = false)
{
    const CBlockIndex* BlockLastSolved = pindexLast;
    const CBlockIndex* BlockReading = pindexLast;
    uint64_t PastBlocksMass = 0;
    int64_t PastRateActualSeconds = 0;
    int64_t PastRateTargetSeconds = 0;
    double PastRateAdjustmentRatio = double(1);
    arith_uint256 PastDifficultyAverage;
    arith_uint256 PastDifficultyAveragePrev;
    double EventHorizonDeviation;
    double EventHorizonDeviationFast;
    double EventHorizonDeviationSlow;

    uint64_t pastSecondsMin = params.nPowTargetTimespan * 0.025;
    uint64_t pastSecondsMax = params.nPowTargetTimespan * 7;
    uint64_t PastBlocksMin = pastSecondsMin / params.nPowTargetSpacing;
    uint64_t PastBlocksMax = pastSecondsMax / params.nPowTargetSpacing;

    if (BlockLastSolved == nullptr || BlockLastSolved->nHeight == 0 || (uint64_t)BlockLastSolved->nHeight < PastBlocksMin) {
        return GetProofOfWorkLimit(testnet).GetCompact();
    }

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) {
            break;
        }
        PastBlocksMass++;

        PastDifficultyAverage.SetCompact(BlockReading->nBits);
        if (i > 1) {
            // handle negative uint256
            if (PastDifficultyAverage >= PastDifficultyAveragePrev)
                PastDifficultyAverage = ((PastDifficultyAverage - PastDifficultyAveragePrev) / i) + PastDifficultyAveragePrev;
            else
                PastDifficultyAverage = PastDifficultyAveragePrev - ((PastDifficultyAveragePrev - PastDifficultyAverage) / i);
        }
        PastDifficultyAveragePrev = PastDifficultyAverage;

        PastRateActualSeconds = BlockLastSolved->GetBlockTime() - BlockReading->GetBlockTime();
        PastRateTargetSeconds = params.nPowTargetSpacing * PastBlocksMass;
        PastRateAdjustmentRatio = double(1);
        if (PastRateActualSeconds < 0) {
            PastRateActualSeconds = 0;
        }
        if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
            PastRateAdjustmentRatio = double(PastRateTargetSeconds) / double(PastRateActualSeconds);
        }
        EventHorizonDeviation = 1 + (0.7084 * pow((double(PastBlocksMass) / double(28.2)), -1.228));
        EventHorizonDeviationFast = EventHorizonDeviation;
        EventHorizonDeviationSlow = 1 / EventHorizonDeviation;

        if (PastBlocksMass >= PastBlocksMin) {
            if ((PastRateAdjustmentRatio <= EventHorizonDeviationSlow) || (PastRateAdjustmentRatio >= EventHorizonDeviationFast)) {
                assert(BlockReading);
                break;
            }
        }
        if (BlockReading->pprev == nullptr) {
            assert(BlockReading);
            break;
        }
        BlockReading = BlockReading->pprev;
    }

    arith_uint256 bnNew(PastDifficultyAverage);
    if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
        bnNew *= PastRateActualSeconds;
        bnNew /= PastRateTargetSeconds;
    }

    if (bnNew > GetProofOfWorkLimit(testnet)) {
        bnNew = GetProofOfWorkLimit(testnet);
    }

    return bnNew.GetCompact();
}

unsigned int static DarkGravityWave(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    const CBlockIndex* BlockLastSolved = pindexLast;
    const CBlockIndex* BlockReading = pindexLast;
    int64_t nActualTimespan = 0;
    int64_t LastBlockTime = 0;
    int64_t PastBlocksMin = 24;
    int64_t PastBlocksMax = 24;
    int64_t CountBlocks = 0;
    arith_uint256 PastDifficultyAverage;
    arith_uint256 PastDifficultyAveragePrev;

    bool isAdjustmentPeriod = BlockLastSolved->nHeight >= Params().PoSStartHeight() - 1 && BlockLastSolved->nHeight < Params().PoSStartHeight() + PastBlocksMax;
    if (BlockLastSolved == nullptr || BlockLastSolved->nHeight == 0 || BlockLastSolved->nHeight < PastBlocksMin || isAdjustmentPeriod) {
        return GetProofOfWorkLimit().GetCompact();
    }

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) {
            break;
        }
        CountBlocks++;

        if (CountBlocks <= PastBlocksMin) {
            if (CountBlocks == 1) {
                PastDifficultyAverage.SetCompact(BlockReading->nBits);
            } else {
                PastDifficultyAverage = ((PastDifficultyAveragePrev * CountBlocks) + (arith_uint256().SetCompact(BlockReading->nBits))) / (CountBlocks + 1);
            }
            PastDifficultyAveragePrev = PastDifficultyAverage;
        }

        if (LastBlockTime > 0) {
            int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
            nActualTimespan += Diff;
        }
        LastBlockTime = BlockReading->GetBlockTime();

        if (BlockReading->pprev == nullptr) {
            assert(BlockReading);
            break;
        }
        BlockReading = BlockReading->pprev;
    }

    arith_uint256 bnNew(PastDifficultyAverage);

    int64_t _nTargetTimespan = CountBlocks * params.nPowTargetSpacing;

    if (nActualTimespan < _nTargetTimespan / 3)
        nActualTimespan = _nTargetTimespan / 3;
    if (nActualTimespan > _nTargetTimespan * 3)
        nActualTimespan = _nTargetTimespan * 3;

    // Retarget
    bnNew *= nActualTimespan;
    bnNew /= _nTargetTimespan;

    if (bnNew > GetProofOfWorkLimit()) {
        bnNew = GetProofOfWorkLimit();
    }

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::Params& params)
{
    unsigned int retarget = DIFF_DGW;

    if (pindexLast->nHeight + 1 >= 1059780) {
        retarget = DIFF_DGW;
    } else {
        retarget = DIFF_BTC;
    }

    if (Params().NetworkIDString() == CBaseChainParams::TESTNET) {
        if (pindexLast->nHeight + 1 < Params().PoSStartHeight()) {
            return GetProofOfWorkLimit(true).GetCompact();
        }
    }

    if (Params().NetworkIDString() == CBaseChainParams::MAIN && pindexLast->nHeight >= Params().PoSStartHeight() - 1) {
        SetProofOfWorkLimit(uint256S("000003ffff000000000000000000000000000000000000000000000000000000"));
    }

    // Default Crown style retargeting
    if (retarget == DIFF_BTC) {
        unsigned int nProofOfWorkLimit = GetProofOfWorkLimit().GetCompact();

        // Genesis block
        if (pindexLast == nullptr)
            return nProofOfWorkLimit;

        // Only change once per interval
        if ((pindexLast->nHeight + 1) % params.DifficultyAdjustmentInterval() != 0) {
            if (params.fPowAllowMinDifficultyBlocks) {
                // Special difficulty rule for testnet:
                // If the new block's timestamp is more than 2* 2.5 minutes
                // then allow mining of a min-difficulty block.
                if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing * 2)
                    return nProofOfWorkLimit;
                else {
                    // Return the last non-special-min-difficulty-rules-block
                    const CBlockIndex* pindex = pindexLast;
                    while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                        pindex = pindex->pprev;
                    return pindex->nBits;
                }
            }
            return pindexLast->nBits;
        }

        // Go back by what we want to be 1 day worth of blocks
        const CBlockIndex* pindexFirst = pindexLast;
        for (int i = 0; pindexFirst && i < params.DifficultyAdjustmentInterval() - 1; i++)
            pindexFirst = pindexFirst->pprev;
        assert(pindexFirst);

        // Limit adjustment step
        int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
        LogPrintf("  nActualTimespan = %d  before bounds\n", nActualTimespan);
        if (nActualTimespan < params.nPowTargetTimespan / 4)
            nActualTimespan = params.nPowTargetTimespan / 4;
        if (nActualTimespan > params.nPowTargetTimespan * 4)
            nActualTimespan = params.nPowTargetTimespan * 4;

        // Retarget
        arith_uint256 bnNew;
        arith_uint256 bnOld;
        bnNew.SetCompact(pindexLast->nBits);
        bnOld = bnNew;
        bnNew *= nActualTimespan;
        bnNew /= params.nPowTargetTimespan;

        if (bnNew > GetProofOfWorkLimit())
            bnNew = GetProofOfWorkLimit();

        /// debug print
        LogPrintf("GetNextWorkRequired RETARGET at %d\n", pindexLast->nHeight + 1);
        LogPrintf("params.nPowTargetTimespan = %d    nActualTimespan = %d\n", params.nPowTargetTimespan, nActualTimespan);
        LogPrintf("Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
        LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

        return bnNew.GetCompact();

    }

    else if (retarget == DIFF_KGW) {
        return KimotoGravityWell(pindexLast, params);
    }

    else if (retarget == DIFF_DGW) {
        return DarkGravityWave(pindexLast, params);
    }

    return DarkGravityWave(pindexLast, params);
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    if (Params().NetworkIDString() == CBaseChainParams::MAIN && ::ChainActive().Height() >= Params().PoSStartHeight() - 1) {
        SetProofOfWorkLimit(uint256S("000003ffff000000000000000000000000000000000000000000000000000000"));
    }

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow)
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

bool CheckProofOfWork(const CBlockHeader& block, const Consensus::Params& params)
{
    if (!block.nVersion.IsLegacy() && Params().StrictChainId() && block.nVersion.GetChainId() != Params().AuxpowChainId()) {
        return error("%s : block does not have our chain ID (got %d, expected %d, full nVersion %d)",
                     __func__, block.nVersion.GetChainId(), Params().AuxpowChainId(), block.nVersion.GetFullVersion());
    }

    if (!block.auxpow) {
        if (block.nVersion.IsAuxpow())
            return error("%s : no AuxPow on block with AuxPow version", __func__);
        if (!CheckProofOfWork(block.GetHash(), block.nBits, params))
            return error("%s : non-AUX proof of work failed", __func__);
        return true;
    }

    if (!block.nVersion.IsAuxpow())
        return error("%s : AuxPow on block with non-AuxPow version", __func__);
    if (block.auxpow->getParentBlock().nVersion.IsAuxpow())
        return error("%s : AuxPow parent block has AuxPow version", __func__);
    if (!block.auxpow->check(block.GetHash(), block.nVersion.GetChainId(), params))
        return error("%s : AuxPow is not valid", __func__);
    if (!CheckProofOfWork(block.auxpow->getParentBlockPoWHash(), block.nBits, params))
        return error("%s : AuxPow work failed", __func__);

    return true;
}
