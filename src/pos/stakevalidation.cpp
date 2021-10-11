#include <pos/kernel.h>
#include <pos/stakevalidation.h>

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <primitives/block.h>
#include <pubkey.h>
#include <util/system.h>
#include <wallet/wallet.h>

bool CheckBlockSignature(const CBlock& block, const CPubKey& pubkeyMasternode)
{
    uint256 hashBlock = block.GetHash();

    return pubkeyMasternode.Verify(hashBlock, block.vchBlockSig);
}

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(const CBlock& block, const CBlockIndex* prevBlock, const COutPoint& outpointStakePointer, uint256& hashProofOfStake, std::string& errormsg)
{
    const CTransactionRef tx = block.vtx[1];
    if (!tx->IsCoinStake()){
        errormsg = strprintf("CheckProofOfStake() : called on non-coinstake %s", tx->GetHash().ToString().c_str());
        return false;
    }

    // Get the stake modifier
    auto pindexModifier = prevBlock->GetAncestor(prevBlock->nHeight - Params().KernelModifierOffset());
    if (!pindexModifier){
        errormsg = "CheckProofOfStake() : could not find modifier index for stake";
        return false;
    }
    uint256 nStakeModifier = pindexModifier->GetBlockHash();

    // Get the correct amount for the collateral
    CAmount nAmountCollateral = 0;
    if (outpointStakePointer.n == 1) {
        nAmountCollateral = Params().MasternodeCollateral() / COIN;
    } else if (outpointStakePointer.n == 2) {
        nAmountCollateral = Params().SystemnodeCollateral() / COIN;
    } else {
        errormsg = "Stake pointer is neither pos 1 or 2";
        return false;
    }

    //LogPrintf("outpointStakePointer %s %d Amount : %d modifier : %s\n", outpointStakePointer.hash.ToString(), outpointStakePointer.n, nAmountCollateral, nStakeModifier.ToString());

    // Reconstruct the kernel that created the stake
    Kernel kernel(outpointStakePointer, nAmountCollateral, nStakeModifier, prevBlock->GetBlockTime(), block.nTime);

    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget = arith_uint256().SetCompact(block.nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow){
        errormsg = "CheckProofOfStake() : nBits below minimum stake";
        return false;
    }

    LogPrintf("%s : %s\n", __func__, kernel.ToString());

    hashProofOfStake = kernel.GetStakeHash();
    bool ret = kernel.IsValidProof(ArithToUint256(bnTarget));
    if(!ret)
        errormsg = "CheckProofOfStake() : Invalid proof";

    return ret;
}
