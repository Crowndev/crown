#ifndef CROWNCORE_STAKEVALIDATION_H
#define CROWNCORE_STAKEVALIDATION_H

#include <string>

class CBlock;
class CPubKey;
class CBlockIndex;
class COutPoint;
class CTransaction;
class uint256;

bool CheckBlockSignature(const CBlock& block, const CPubKey& pubkeyMasternode);

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(const CBlock& block, const CBlockIndex* prevBlock, const COutPoint& outpointStakePointer, uint256& hashProofOfStake, std::string& errormsg);

#endif //CROWNCORE_STAKEVALIDATION_H
