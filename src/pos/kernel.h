#ifndef CROWN_CORE_KERNEL_H
#define CROWN_CORE_KERNEL_H

#include <uint256.h>
#include <primitives/transaction.h>

class arith_uint256;
class StakePointer;

class Kernel {
public:
    Kernel(const COutPoint& outpoint, const uint64_t nAmount, const uint256& nStakeModifier,
        const uint64_t& nTimeBlockFrom, const uint64_t& nTimeStake);
    uint256 GetStakeHash();
    uint64_t GetTime() const;
    bool IsValidProof(const uint256& nTarget);
    void SetStakeTime(uint64_t nTime);
    std::string ToString();

    static bool CheckProof(const arith_uint256& target, const arith_uint256& hash, const uint64_t nAmount);

private:
    COutPoint m_outpoint;
    uint256 m_nStakeModifier{};
    uint64_t m_nTimeBlockFrom {0};
    uint64_t m_nTimeStake {0};
    uint64_t m_nAmount {0};
};

#endif //CROWN_CORE_KERNEL_H
