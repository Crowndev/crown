#include "nf-token-multiindex-utils.h"

unsigned int hash_value(const uint256 & val)
{
    //TODO: place for possible optimization: boost::hash vs std::hash vs uint256::GetHash vs SipHashUint256
    return boost::hash_range(val.begin(), val.end());
}

unsigned int hash_value(const CKeyID & val)
{
    //TODO: place for possible optimization: boost::hash vs std::hash vs uint256::GetHash vs SipHashUint256
    return boost::hash_range(val.begin(), val.end());
}
