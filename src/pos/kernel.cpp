#include <arith_uint256.h>
#include <hash.h>
#include <pos/kernel.h>
#include <pos/stakepointer.h>
#include <streams.h>
#include <tinyformat.h>
#include <util/strencodings.h>

#include <iostream>

/*
 * A 'proof hash' (also referred to as a 'kernel') is comprised of the following:
 * Masternode's outpoint: the outpoint of the masternode is the collateral transaction that the masternode is created by. This is unique to each masternode.
 * Stake Modifier: entropy collected from points in the f0uture of the blockchain, far enough after the outpoint was created to prevent any 'grinding'
 * Block Time: The time that the block containing the outpoint was added to the blockchain
 * Stake Time: The time that the stake hash is being created
 */
Kernel::Kernel(const COutPoint& outpoint, const uint64_t nAmount, const uint256& nStakeModifier,
    const uint64_t& nTimeBlockFrom, const uint64_t& nTimeStake)
{
    m_outpoint = outpoint;
    m_nAmount = nAmount;
    m_nStakeModifier = nStakeModifier;
    m_nTimeBlockFrom = nTimeBlockFrom;
    m_nTimeStake = nTimeStake;
}

uint256 Kernel::GetStakeHash()
{
    CDataStream ss(SER_GETHASH, 0);
    ss << m_outpoint.hash << m_outpoint.n << m_nStakeModifier << m_nTimeBlockFrom << m_nTimeStake;
    return Hash(ss);
}

uint64_t Kernel::GetTime() const
{
    return m_nTimeStake;
}

bool Kernel::CheckProof(const arith_uint256& target, const arith_uint256& hash, const uint64_t nAmount)
{
    return hash < nAmount * target;
}

bool Kernel::IsValidProof(const uint256& nTarget)
{
    arith_uint256 target = UintToArith256(nTarget);
    arith_uint256 hashProof = UintToArith256(GetStakeHash());
    return CheckProof(target, hashProof, m_nAmount);
}

void Kernel::SetStakeTime(uint64_t nTime)
{
    m_nTimeStake = nTime;
}

std::string Kernel::ToString()
{
    return strprintf("OutPoint: %s:%d Modifier=%s timeblockfrom=%d time=%d", m_outpoint.hash.GetHex(),
        m_outpoint.n, m_nStakeModifier.GetHex(), m_nTimeBlockFrom, m_nTimeStake);
}
